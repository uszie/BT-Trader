#include "autooptimizerworker.h"
#include "autooptimizer.h"
#include "indicator.h"
#include "bttrader.h"
#include "rule.h"
#include <unistd.h>

class AutoOptimizerWorker::PrivateData
{
    public:
        QVector<Indicator *> indicatorList;
        Indicator *workIndicator;
        AutoOptimizer *autoOptimizer;
        QList<AutoOptimizerOption *> optionsList;
        QVariant taskMin;
        QVariant taskMax;
        qulonglong progressCount;
        qulonglong totalAvailableOptions;
        bool stopProcessing;
        bool pause;
        bool newTradeResultsAvailable;
};

AutoOptimizerWorker::AutoOptimizerWorker(AutoOptimizer *autoOptimizer)
{
    privateData = new PrivateData;
    privateData->autoOptimizer = autoOptimizer;
    privateData->progressCount = 0;
    privateData->totalAvailableOptions = 0;

    QStringList indicatorNumberList;
    for (int i = privateData->autoOptimizer->indicators().size() - 1; i >= 0; i--)
        indicatorNumberList << privateData->autoOptimizer->indicators().at(i)->configGroupKey();
    privateData->indicatorList = privateData->autoOptimizer->indicator()->priceChart->restoreIndicators(indicatorNumberList, PriceChart::Hidden).toVector();

    QString title = privateData->autoOptimizer->indicator()->title();
    for (int i = 0; i < privateData->indicatorList.size(); i++)
    {
        Indicator *indicator = privateData->indicatorList.at(i);
        indicator->setAutoOptimizerMode(true);
        indicator->setAutoRestore(false);
        indicator->setConfigGroupKey("Indicator-1");

        if (indicator->title() == title)
            privateData->workIndicator = indicator;
    }

    if (!privateData->workIndicator)
    {
        qWarning() << "AutoOptimizerWorker::AutoOptimizerWorker(): Could not find work leader";
        return;
    }

    QList<AutoOptimizerOption *> optionsList = privateData->autoOptimizer->listOptions(AutoOptimizerOption::AllOptions);
    privateData->optionsList = privateData->workIndicator->autoOptimizer().listOptions(AutoOptimizerOption::AllOptions);
    for (int i = 0; i < privateData->optionsList.size(); i++)
    {
        privateData->optionsList.at(i)->setMin(optionsList.at(i)->min());
        privateData->optionsList.at(i)->setMax(optionsList.at(i)->max());
        privateData->optionsList.at(i)->setStepSize(optionsList.at(i)->stepSize());
        privateData->optionsList.at(i)->setUseAcceleration(optionsList.at(i)->useAcceleration());
    }
}

AutoOptimizerWorker::~AutoOptimizerWorker(void)
{
    for (int i = 0; i < privateData->indicatorList.size(); i++)
    {
        Indicator *indicator = privateData->indicatorList.at(i);
        while (indicator->externalReferenceCount())
            indicator->decreaseExternalReferenceCount();
        indicator->removeIndicator();
    }

    delete privateData;
}

void AutoOptimizerWorker::setTaskStartPoint(const QVariant &start)
{
    switch (privateData->optionsList.at(0)->varType())
    {
        case AutoOptimizerOption::Integer:
            privateData->taskMin = start.toInt();
            break;

        case AutoOptimizerOption::Double:
            privateData->taskMin = start.toFloat();
            break;

        case AutoOptimizerOption::Boolean:
            privateData->taskMin = start.toBool();
            break;

        default:
            privateData->taskMin = start.toInt();
            break;
    }
}

void AutoOptimizerWorker::setTaskEndPoint(const QVariant &end)
{
    switch (privateData->optionsList.at(0)->varType())
    {
        case AutoOptimizerOption::Integer:
            privateData->taskMax = end.toInt();
            break;

        case AutoOptimizerOption::Double:
            privateData->taskMax = end.toFloat();
            break;

        case AutoOptimizerOption::Boolean:
            privateData->taskMax = end.toBool();
            break;

        default:
            privateData->taskMax = end.toInt();
            break;
    }
}

float AutoOptimizerWorker::progressRate(void)
{
    return static_cast<float>((static_cast<long double>(privateData->progressCount) / static_cast<long double>(privateData->totalAvailableOptions)) * 100.0);
}

void AutoOptimizerWorker::setTotalAvailableOptions(qulonglong count)
{
    privateData->totalAvailableOptions = count;
}

bool AutoOptimizerWorker::isRunning(void) const
{
    return !privateData->stopProcessing;
}

void AutoOptimizerWorker:: start(void)
{
    privateData->stopProcessing = false;
    privateData->pause = false;
    privateData->progressCount = 0;

    if (!privateData->workIndicator)
    {
        emit finished();
        privateData->stopProcessing = true;
        return;
    }

    QList<Indicator *> indicatorTouchedList;
    for (int i = 0; i < privateData->indicatorList.size(); i++)
        indicatorTouchedList << privateData->indicatorList.at(i);


    for (int i = 0; i < privateData->optionsList.size(); ++i)
    {
        AutoOptimizerOption *option = privateData->optionsList.at(i);
        if (option->hasGuiItemsEnabled())
        {
            if (i == 0)
                option->setValue(privateData->taskMin);
            else
                option->setValue(option->min());
        }
    }

    privateData->workIndicator->clearStopLossCache();
    privateData->workIndicator->clearTakeProfitsCache();
    collectSessionInfo();

    privateData->workIndicator->firstRuns = 0;
    privateData->workIndicator->secondRuns = 0;

    bool done = false;
    bool reuseOldStopLossCache = false;
    bool reuseOldTakeProfitsCache = false;
    bool rulesTouched = true;
    bool exitRulesTouched = true;
    bool longRulesTouched = true;
    bool shortRulesTouched = true;
    bool stopLossTouched = true;
    bool takeProfitsTouched = true;
    int unfinshedStrategies = 0;
    int depth = 0;
    while (done == false)
    {
        if (privateData->stopProcessing)
        {
            emit finished();
            return;
        }

        if (privateData->pause)
        {
            usleep(1000);
            continue;
        }

        bool skip = false;
        QList<AutoOptimizerOption *>::const_iterator optionsIt = privateData->optionsList.constBegin();
        if (depth > 0)
            optionsIt = optionsIt + depth;
        const QList<AutoOptimizerOption *>::const_iterator optionsEnd = privateData->optionsList.constEnd();
        for (; optionsIt != optionsEnd; optionsIt++)
        {
            AutoOptimizerOption *option = *optionsIt;
            if (!option->enabled())
                continue;

            if (!option->dependenciesForfilled())
            {
                skip = true;
                break;
            }

            if (!option->valueCheckBoxEnabled(option->value()))
            {
                skip = true;
                break;
            }
        }

        if (skip)
            ;//qDebug() << "Skipping configuration " << optionsSettingsToString();
        else
        {
            for (int i = 0; i < indicatorTouchedList.size(); ++i)
                indicatorTouchedList.at(i)->plotCurve();

            if (!indicatorTouchedList.isEmpty() || rulesTouched)
            {
                privateData->workIndicator->ruleList(Long)->clearRuleResolvementCache();
                privateData->workIndicator->ruleList(Long)->createRuleResolvementCache();
                privateData->workIndicator->ruleList(Short)->clearRuleResolvementCache();
                privateData->workIndicator->ruleList(Short)->createRuleResolvementCache();
                privateData->workIndicator->ruleList(Exit)->clearRuleResolvementCache();
                privateData->workIndicator->ruleList(Exit)->createRuleResolvementCache();
            }
            else
            {
                if (longRulesTouched)
                    privateData->workIndicator->ruleList(Long)->createRuleResolvementCache();

                if (shortRulesTouched)
                    privateData->workIndicator->ruleList(Short)->createRuleResolvementCache();

                if (exitRulesTouched)
                    privateData->workIndicator->ruleList(Exit)->createRuleResolvementCache();
            }

            if (stopLossTouched)
                privateData->workIndicator->createStopLossCache(reuseOldStopLossCache);

            if (takeProfitsTouched)
                privateData->workIndicator->createTakeProfitsCache(reuseOldTakeProfitsCache);

            updateSessionInfo(!indicatorTouchedList.isEmpty());

            privateData->workIndicator->executeTransactionRules();
            TradeResults tradeResults = privateData->workIndicator->tradeResults();
            privateData->autoOptimizer->increaseProgressValue();
            privateData->progressCount++;

            if (tradeResults.ignore)
                unfinshedStrategies++;
            else
            {
                bool needsEmit = true;
                if (tradeResults.relativeProfits < privateData->workIndicator->autoOptimizer().sessionInfo().worstTradeResults.relativeProfits &&
                    privateData->autoOptimizer->tradeResultsCount() >= privateData->autoOptimizer->maximumResultsVisible())
                    needsEmit = false;

                if (!privateData->autoOptimizer->showNegativeResults() && tradeResults.relativeProfits < 0.0f)
                    needsEmit = false;

                if (tradeResults.totalTransactions <= 0)
                    needsEmit = false;

                float allowedCapitalLoss = privateData->autoOptimizer->maximumAllowedCapitalLoss();
                if (allowedCapitalLoss > 0.0f && tradeResults.relativeProfits < (allowedCapitalLoss * -1.0f))
                    needsEmit = false;

                float requieredSuccessRate = privateData->autoOptimizer->minimumRequieredSuccessRate();
                if (requieredSuccessRate > 0.0f && tradeResults.successRate < requieredSuccessRate)
                    needsEmit = false;

                int requieredTrades = privateData->autoOptimizer->minimumRequieredTrades();
                if (requieredTrades > 0 && (int) tradeResults.totalTrades < requieredTrades)
                    needsEmit = false;

                if (needsEmit)
                {
                    tradeResults.settings = settings();
                    tradeResults.configuration = optionsSettingsToString();
                    privateData->autoOptimizer->addResult(tradeResults);
                }
            }
        }

        indicatorTouchedList.clear();
        reuseOldStopLossCache = true;
        reuseOldTakeProfitsCache = true;
        rulesTouched = false;
        exitRulesTouched = false;
        longRulesTouched = false;
        shortRulesTouched = false;
        stopLossTouched = false;
        takeProfitsTouched = false;
        depth = privateData->optionsList.size() - 1;
        optionsIt = privateData->optionsList.constEnd();
        const QList<AutoOptimizerOption *>::const_iterator optionsBegin = privateData->optionsList.constBegin();
        for (optionsIt = optionsIt - 1; optionsIt >= optionsBegin; optionsIt--)
        {
            if (done)
                break;

            AutoOptimizerOption *option = *optionsIt;
            if (!option->enabled())
            {
                if (optionsIt == optionsBegin)
                    done = true;

                continue;
            }

            depth--;

            float min;
            float max;
            if (optionsIt == optionsBegin)
            {
                min = privateData->taskMin.toFloat();
                max = privateData->taskMax.toFloat();
            }
            else
            {
                min = option->min().toFloat();
                max = option->max().toFloat();
            }

            float stepSize = option->stepSize().toFloat();
            float accelleration;
            if (option->useAcceleration())
                accelleration = 1.1f;
            else
                accelleration = 1.0f;

            AutoOptimizerOption::optionTypes type = option->type();
            switch (type)
            {
                case AutoOptimizerOption::RuleOption:
                    rulesTouched = true;
                    break;

                case AutoOptimizerOption::ExitRuleOption:
                    exitRulesTouched = true;
                    break;

                case AutoOptimizerOption::LongRuleOption:
                    longRulesTouched = true;
                    break;

                case AutoOptimizerOption::ShortRuleOption:
                    shortRulesTouched = true;
                    break;

                case AutoOptimizerOption::StopLossOption:
                    stopLossTouched = true;
                    break;

                case AutoOptimizerOption::TakeProfitOption:
                    takeProfitsTouched = true;
                    break;
                default:
                    break;
            }

            if (type == AutoOptimizerOption::IndicatorOption && option->indicator())
            {
                if (!indicatorTouchedList.contains(option->indicator()))
                    indicatorTouchedList << option->indicator();
            }

            float value = option->value(0).toFloat();
            if (option->varType() == AutoOptimizerOption::Integer)
                value = static_cast<float>(static_cast<int>((value * accelleration) + stepSize));
            else
                value = (value * accelleration) + stepSize;


            if (value <= max)
            {
                option->setValue(value, 5);
                break;
            }
            else
            {
                if (optionsIt == optionsBegin)
                {
                    done = true;
                    continue;
                }

                if (stopLossTouched && (option->title() == tr("Stoploss (relatief)") || option->title() == tr("Stoploss (absoluut)")))
                    reuseOldStopLossCache = false;

                if (takeProfitsTouched && (option->title() == tr("Winst nemen (relatief)") || option->title() == tr("Winst nemen (absoluut)")))
                    reuseOldTakeProfitsCache = false;

                option->setValue(min, 5);
            }
        }
    }

    qDebug() << "DONE firstRuns=" << privateData->workIndicator->firstRuns << " secondRuns=" << privateData->workIndicator->secondRuns << " percentage=" << (/*1.0 - */(privateData->workIndicator->secondRuns / privateData->workIndicator->firstRuns)) * 100 << "%";
    qDebug() << "totalAvailableOptions=" << ((float) privateData->totalAvailableOptions);
    qDebug() << "private progressValue=" << privateData->progressCount;
    qDebug() << "killedStrategies=" << ((float) unfinshedStrategies);

    privateData->stopProcessing = true;
    emit finished();
}

void AutoOptimizerWorker::stop(void)
{
    privateData->stopProcessing = true;
}

void AutoOptimizerWorker::pause(bool value)
{
    privateData->pause = value;
}

QList<QVariant> AutoOptimizerWorker::settings(void) const
{
    QList<QVariant> list;
    for (int i = 0; i < privateData->optionsList.size(); ++i)
    {
        AutoOptimizerOption *option = privateData->optionsList.at(i);
        list << option->value();
    }

    return list;
}

QString AutoOptimizerWorker::optionsSettingsToString(void) const
{
    QString configuration;
    for (int i = 0; i < privateData->optionsList.size(); ++i)
    {
        const AutoOptimizerOption *option = privateData->optionsList.at(i);
        if (!option->enabled())
            continue;

        configuration.append(option->title());
        configuration.append("=");

        if (option->suffix() == " %")
            configuration.append(QString::number(option->value().toDouble() * 100, 'f', 2) + " %");
        else
            configuration.append(option->valueTitle(option->value()));
        configuration.append("; ");
    }

    return configuration;
}

void AutoOptimizerWorker::collectSessionInfo(void)
{
    SessionInfo &sessionInfo = privateData->workIndicator->autoOptimizer().sessionInfo();

    sessionInfo.maximumResultsVisible = privateData->autoOptimizer->maximumResultsVisible();
    sessionInfo.showDoubleResults = privateData->autoOptimizer->showDoubleResults();
    sessionInfo.showNegativeResults = privateData->autoOptimizer->showNegativeResults();
    sessionInfo.deadStrategyCutOffRate = privateData->autoOptimizer->deadStrategyCutOffRate();
    sessionInfo.deadStrategyCutOffPoint = 0;
    sessionInfo.maximumAllowedCapitalLoss = privateData->autoOptimizer->maximumAllowedCapitalLoss();
    sessionInfo.minimumRequieredTrades = privateData->autoOptimizer->minimumRequieredTrades();
    sessionInfo.minimumRequieredSuccessRate = privateData->autoOptimizer->minimumRequieredSuccessRate();
    sessionInfo.tradeResultsCount = 0;
    sessionInfo.worstTradeResults.startCapital = 0.0;
    sessionInfo.worstTradeResults.finalCapital = 0.0;
    sessionInfo.worstTradeResults.transactionCosts = 0.0;
    sessionInfo.worstTradeResults.profits = 0.0;
    sessionInfo.worstTradeResults.relativeProfits = -LLONG_MAX;
    sessionInfo.worstTradeResults.successRate = 0.0;
    sessionInfo.worstTradeResults.totalTransactions = 0;
    sessionInfo.worstTradeResults.totalTrades = 0;
    sessionInfo.worstTradeResults.totalProfitTrades = 0;
    sessionInfo.worstTradeResults.totalLossTrades = 0;
    sessionInfo.bestTradeResults = sessionInfo.worstTradeResults;
    sessionInfo.maximumProfitsMap.clear();

    if (!privateData->workIndicator->priceChart->averagePriceClose->dataSize() == 0)
    {
        double previousRate = 1.0;
        const QVector<QPointF> &priceDataClose = privateData->workIndicator->priceChart->averagePriceClose->data();
        QVector<QPointF>::const_iterator iterator = priceDataClose.constBegin();
        const QVector<QPointF>::const_iterator &end = priceDataClose.constEnd();
        QPointF newPoint(iterator[ 0 ].x(), 1.0);
        sessionInfo.maximumProfitsMap.append(newPoint);
        for (iterator = iterator + 1; iterator != end; ++iterator)
        {
            QPointF point = iterator[ 0 ];
            QPointF previousPoint = iterator[ -1 ];

            float valueDifference = point.y() - previousPoint.y();
            float rate;
            if (valueDifference < 0)
                rate = previousPoint.y() / point.y();
            else if (valueDifference > 0)
                rate = point.y() / previousPoint.y();
            else
                rate = 1.0;


            newPoint.setX(point.x());
            newPoint.setY(rate * previousRate);
            sessionInfo.maximumProfitsMap.append(newPoint);
            previousRate = rate * previousRate;
        }
    }

    if (sessionInfo.maximumProfitsMap.isEmpty())
        sessionInfo.maximumProfitsMap.append(QPointF(0.0, 1.0));

    double offset = 0;
    QVector<Indicator *>::const_iterator indicatorListIt = privateData->indicatorList.constBegin();
    const QVector<Indicator *>::const_iterator indicatorListEnd = privateData->indicatorList.constEnd();
    for (; indicatorListIt != indicatorListEnd; indicatorListIt++)
    {
        const Indicator *indicator = *indicatorListIt;
        for (int i = 0; i < 3; i++)
        {
            QVector<Rule *>::const_iterator ruleListIt = indicator->ruleList((TransactionType)i)->items().constBegin();
            QVector<Rule *>::const_iterator ruleListEnd = indicator->ruleList((TransactionType)i)->items().constEnd();
            for (; ruleListIt != ruleListEnd; ruleListIt++)
            {
                const Rule *rule = *ruleListIt;
                if (rule->sourceCurve && !rule->sourceCurve->dataSize() == 0 && !indicator->priceChart->averagePriceClose->dataSize() == 0)
                {
                    double tmp = rule->sourceCurve->x(0) - indicator->priceChart->averagePriceClose->x(0);
                    if (tmp > offset)
                        offset = tmp;
                }

                if (rule->referenceCurve && !rule->referenceCurve->dataSize() == 0 && !indicator->priceChart->averagePriceClose->dataSize() == 0)
                {
                    double tmp = rule->referenceCurve->x(0) - indicator->priceChart->averagePriceClose->x(0);
                    if (tmp > offset)
                        offset = tmp;
                }
            }
        }
    }

    if (!privateData->workIndicator->priceChart->averagePriceClose->dataSize() == 0)
    {
        double range = privateData->workIndicator->priceChart->averagePriceClose->x(privateData->workIndicator->priceChart->averagePriceClose->dataSize() - 1) - privateData->workIndicator->priceChart->averagePriceClose->x(0);
        range = privateData->workIndicator->priceChart->averagePriceClose->x(0) + ((range * sessionInfo.deadStrategyCutOffRate) / 100.0);
        sessionInfo.deadStrategyCutOffPoint = (int)(range + offset);
    }
    else
        sessionInfo.deadStrategyCutOffPoint = 0;
}

void AutoOptimizerWorker::updateSessionInfo(bool updateDeadStrategyCutOffPoint)
{
    bool newTradeResults = newTradeResultsAvailable();
    setNewTradeResultsAvailable(false);

    if (!updateDeadStrategyCutOffPoint && !newTradeResults)
        return;

    SessionInfo &sessionInfo = privateData->workIndicator->autoOptimizer().sessionInfo();

    if (updateDeadStrategyCutOffPoint)
    {
        double offset = 0;
        QVector<Indicator *>::const_iterator indicatorListIt = privateData->indicatorList.constBegin();
        const QVector<Indicator *>::const_iterator indicatorListEnd = privateData->indicatorList.constEnd();
        for (; indicatorListIt != indicatorListEnd; indicatorListIt++)
        {
            const Indicator *indicator = *indicatorListIt;
            for (int i = 0; i < 3; i++)
            {
                QVector<Rule *>::const_iterator ruleListIt = indicator->ruleList((TransactionType)i)->items().constBegin();
                QVector<Rule *>::const_iterator ruleListEnd = indicator->ruleList((TransactionType)i)->items().constEnd();
                for (; ruleListIt != ruleListEnd; ruleListIt++)
                {
                    const Rule *rule = *ruleListIt;
                    if (rule->sourceCurve && !rule->sourceCurve->dataSize() == 0 && !indicator->priceChart->averagePriceClose->dataSize() == 0)
                    {
                        double tmp = rule->sourceCurve->x(0) - indicator->priceChart->averagePriceClose->x(0);
                        if (tmp > offset)
                            offset = tmp;
                    }

                    if (rule->referenceCurve && !rule->referenceCurve->dataSize() == 0 && !indicator->priceChart->averagePriceClose->dataSize() == 0)
                    {
                        double tmp = rule->referenceCurve->x(0) - indicator->priceChart->averagePriceClose->x(0);
                        if (tmp > offset)
                            offset = tmp;
                    }
                }
            }
        }

        if (!privateData->workIndicator->priceChart->averagePriceClose->dataSize() == 0)
        {
            double range = privateData->workIndicator->priceChart->averagePriceClose->x(privateData->workIndicator->priceChart->averagePriceClose->dataSize() - 1) - privateData->workIndicator->priceChart->averagePriceClose->x(0);
            range = privateData->workIndicator->priceChart->averagePriceClose->x(0) + ((range * sessionInfo.deadStrategyCutOffRate) / 100.0);
            sessionInfo.deadStrategyCutOffPoint = static_cast<int>(range + offset);
        }
        else
            sessionInfo.deadStrategyCutOffPoint = 0;
    }

    if (newTradeResults)
    {
        sessionInfo.tradeResultsCount = privateData->autoOptimizer->tradeResultsCount();
        sessionInfo.bestTradeResults = privateData->autoOptimizer->tradeResultsItemAt(0);
        sessionInfo.worstTradeResults = privateData->autoOptimizer->tradeResultsItemAt(sessionInfo.tradeResultsCount - 1);
    }
}

void AutoOptimizerWorker::setNewTradeResultsAvailable(bool available)
{
    privateData->newTradeResultsAvailable = available;
}

inline bool AutoOptimizerWorker::newTradeResultsAvailable(void)
{
    return privateData->newTradeResultsAvailable;
}
