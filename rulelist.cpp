#include "rulelist.h"
#include "rule.h"
#include "bttrader.h"
#include "indicator.h"
#include "autooptimizer.h"
#include "rule_evaluation.h"

class RuleList::PrivateData
{
    public:
        QVector<Rule *> ruleList;
        Indicator *indicator;
        PriceChart *priceChart;
        TransactionType type;
        QCache<QByteArray, SignalCacheOject> ruleResolvementCache;
        SignalCacheOject *currentCache;
};

RuleList::RuleList(PriceChart *priceChart, Indicator *parent, TransactionType type)
{
    privateData = new PrivateData;
    privateData->priceChart = priceChart;
    privateData->indicator = parent;
    privateData->type = type;
    privateData->currentCache = NULL;
    privateData->ruleResolvementCache.setMaxCost(10000);
}

RuleList::~RuleList()
{
    delete privateData;
}

void RuleList::read(const QString &configKey)
{
    clear();

    while (!settings.group().isEmpty())
        settings.endGroup();

    QVector<Rule *> ruleList;
    QVariantList variantRuleList = settings.value(configKey).toList();
    foreach(QVariant variant, variantRuleList)
    ruleList << variant.value<Rule *>();

    initRuleList(&ruleList);
}

void RuleList::write(const QString &configKey)
{
    while (!settings.group().isEmpty())
        settings.endGroup();

    QVariantList variantRuleList;
    foreach(Rule * rule, privateData->ruleList)
    variantRuleList << qVariantFromValue(rule);

    settings.setValue(configKey, variantRuleList);
    settings.sync();
}

Rule *RuleList::addRule(const QString &ruleString, Rule *parent)
{
    QStringList list = ruleString.split(";", QString::SkipEmptyParts);

    if (list.size() != 10)
    {
        qWarning() << "RuleList::addRule(" << ruleString << ", " << parent << "): Invalid rule string, incorrect parameter count: " << list.size() << ", parameters=" << list;
        return NULL;
    }

    for (int i = 0; i < list.size(); i++)
    {
        if (list.at(i).split("=", QString::SkipEmptyParts).size() == 2)
            continue;

        qWarning() << "RuleList::addRule(" << ruleString << ", " << parent << "): Invalid rule string.";
        return NULL;
    }

    Rule *rule = new Rule;
    rule->indicator = privateData->indicator;
    rule->sourceIndicator = findIndicator(list.at(0).split("=").at(1));
    rule->sourceCurve = findCurve(list.at(1).split("=").at(1), rule->sourceIndicator);
    rule->referenceIndicator = findIndicator(list.at(2).split("=").at(1));
    rule->referenceCurve = findCurve(list.at(3).split("=").at(1), rule->referenceIndicator);
    rule->referenceValue = list.at(4).split("=").at(1).toFloat();
    rule->offset = list.at(5).split("=").at(1).toInt();
    rule->compareType = (Rule::CompareType) Rule::compareTypeFromString(list.at(6).split("=").at(1));
    rule->logicalOperator = (Rule::LogicalOperator) Rule::logicalOperatorFromString(list.at(7).split("=").at(1));
    rule->previousTransactionType = (TransactionType) Rule::previousTransactionTypeFromString(list.at(8).split("=").at(1));
    rule->enabled = list.at(9).split("=").at(1).toInt();
    rule->filter.type = RuleFilter::NoFilter;
    rule->filter.periods = 1;
    rule->filter.relativeDifference = 0.0;
    rule->filter.absoluteDifference = 0.0;
    rule->updateTitle();

    if (!rule->isValid())
    {
        qWarning() << "RuleList::addRule(" << ruleString << ", " << parent << "): One ore more parameters are incorrect";
        return NULL;
    }

    return addRule(rule, parent);
}

Rule *RuleList::addRule(Rule *rule, Rule *parent)
{
    if (!rule)
        return NULL;

    if (parent)
    {
        if (rule->logicalOperator == Rule::AND) // AND voorwaarden moeten aan het einde van de lijst worden toegevoegd.
            parent->children.append(rule);
        else
        {
            int firstAndRuleIndex = 0;
            for (int i = 0; i < parent->children.size(); i++)
            {
                firstAndRuleIndex = i + 1;
                if (parent->children.at(i)->logicalOperator == Rule::AND)
                    break;
            }
            parent->children.insert(firstAndRuleIndex, rule);
        }
    }
    else
    {
        if (rule->logicalOperator == Rule::AND)
            privateData->ruleList.append(rule);
        else
        {
            int firstAndRuleIndex = 0;
            for (int i = 0; i < privateData->ruleList.size(); i++)
            {
                firstAndRuleIndex = i + 1;
                if (privateData->ruleList.at(i)->logicalOperator == Rule::AND)
                    break;
            }
            privateData->ruleList.insert(firstAndRuleIndex, rule);
        }
    }


    rule->parent = parent;

    if (rule->sourceIndicator)
    {
        QApplication::connect(rule->sourceIndicator, SIGNAL(titleChanged(Indicator *, QString)), rule->indicator, SLOT(renameRuleToNewTitle(Indicator *, QString))/*, Qt::UniqueConnection*/);
        if (rule->sourceIndicator != rule->indicator)
        {
            rule->sourceIndicator->increaseExternalReferenceCount();
            QApplication::connect(rule->sourceIndicator, SIGNAL(propertiesChanged()), rule->indicator, SLOT(executeTransactionRules()));
            rule->indicator->autoOptimizer().addIndicator(rule->sourceIndicator);
        }
    }

    if (rule->referenceIndicator)
    {
        QApplication::connect(rule->referenceIndicator, SIGNAL(titleChanged(Indicator *, QString)), rule->indicator, SLOT(renameRuleToNewTitle(Indicator *, QString))/*, Qt::UniqueConnection*/);
        if (rule->referenceIndicator != rule->indicator)
        {
            rule->referenceIndicator->increaseExternalReferenceCount();
            QApplication::connect(rule->referenceIndicator, SIGNAL(propertiesChanged()), rule->indicator, SLOT(executeTransactionRules()));
            rule->indicator->autoOptimizer().addIndicator(rule->referenceIndicator);
        }
    }

    int min = 1;
    int max = 0;
    AutoOptimizerOption::optionTypes optionType;
    if (privateData->type == Exit)
    {
        min = 0;
        max = 1;
        optionType = AutoOptimizerOption::ExitRuleOption;
    }
    else if (privateData->type == Long)
        optionType = AutoOptimizerOption::LongRuleOption;
    else if (privateData->type == Short)
        optionType = AutoOptimizerOption::ShortRuleOption;
    else
        optionType = AutoOptimizerOption::RuleOption;

    AutoOptimizerOption *ruleOption = rule->indicator->autoOptimizer().addOption(rule->indicator, rule->humanReadableTitle, &rule->enabled, min, max, optionType);
    ruleOption->setValueCheckBoxEnabled(true, false);
    rule->autoOptimizerOptionList << ruleOption;
    if (!rule->referenceCurve && (rule->compareType == Rule::IsHigher || rule->compareType == Rule::IsLower || rule->compareType == Rule::IsCrossingDown || rule->compareType == Rule::IsCrossingUp))
    {
        AutoOptimizerOption *ruleReferenceOption = rule->indicator->autoOptimizer().addOption(rule->indicator, QString(rule->title + QApplication::tr(" referentie waarde")), &rule->referenceValue, rule->referenceValue, rule->referenceValue, 1.0, true, false, optionType);
        rule->autoOptimizerOptionList << ruleReferenceOption;
        ruleReferenceOption->addDependency(ruleOption, true);
    }

    return rule;
}

void RuleList::deleteRule(Rule *rule)
{
    disconnectRuleIndicators(rule);

    if (rule->parent)
    {
        int index = rule->parent->children.indexOf(rule);
        if (index != - 1)
            rule->parent->children.remove(index);
    }
    else
    {
        int index = privateData->ruleList.indexOf(rule);
        if (index != -1)
            privateData->ruleList.remove(index);
    }

    while (rule->children.size() != 0)
        deleteRule(rule->children.at(0));

    delete rule;
}

void RuleList::clear(void)
{
    while (privateData->ruleList.size() != 0)
        deleteRule(privateData->ruleList.at(0));
}

const QVector<Rule *> &RuleList::items() const
{
    return privateData->ruleList;
}

inline bool RuleList::resolveRule(int currentBar, const Rule *rule, TransactionType transactionType) const
{
    bool result = false;

    if ((rule->previousTransactionType != transactionType) && (rule->previousTransactionType != Undefined))
        return false;

    currentBar = currentBar - rule->offset;
    if (currentBar < 0)
        return false;

    switch (rule->compareType)
    {
        case IsAbove:
            if (rule->referenceCurve)
                result = isAbove(currentBar, rule->sourceCurve->fastData(), rule->referenceCurve->fastData(), rule->filter);
            else
                result = isAbove(currentBar, rule->sourceCurve->fastData(), rule->referenceValue, rule->filter);
            break;

        case IsUnder:
            if (rule->referenceCurve)
                result = isUnder(currentBar, rule->sourceCurve->fastData(), rule->referenceCurve->fastData(), rule->filter);
            else
                result = isUnder(currentBar, rule->sourceCurve->fastData(), rule->referenceValue, rule->filter);
            break;

        case IsCrossingUp:
            if (rule->referenceCurve)
                result = isCrossingUp(currentBar, rule->sourceCurve->fastData(), rule->referenceCurve->fastData(), rule->filter);
            else
                result = isCrossingUp(currentBar, rule->sourceCurve->fastData(), rule->referenceValue, rule->filter);
            break;

        case IsCrossingDown:
            if (rule->referenceCurve)
                result = isCrossingDown(currentBar, rule->sourceCurve->fastData(), rule->referenceCurve->fastData(), rule->filter);
            else
                result = isCrossingDown(currentBar, rule->sourceCurve->fastData(), rule->referenceValue, rule->filter);
            break;

        case IsRising:
            result = isRising(currentBar, rule->sourceCurve->fastData(), rule->filter);
            break;

        case IsFalling:
            result = isFalling(currentBar, rule->sourceCurve->fastData(), rule->filter);
            break;

        default:
            return false;
    }

    if (rule->children.isEmpty())
        return result;

    //    if (result && ((rule->children.at(0).logicalOperator & OR) >> 1))         // Dit moet nog gecontrolleerd worden.
    if (result && (rule->children.at(0)->logicalOperator == Rule::OR))
        return result;

    const struct RuleListResult &listResult = doResolveRuleList(currentBar, transactionType, &rule->children);
    if (listResult.onlyContainsDisabledRules)
        return result;

    if (!listResult.result)
        return false;

    if (listResult.positiveAND && result)
        return true;

    if (listResult.positiveOR)
        return true;

    return false;
}

struct RuleListResult RuleList::doResolveRuleList(int currentBar, TransactionType transactionType, const QVector<Rule *> *ruleList) const
{
    struct RuleListResult result;
    result.result = result.positiveOR = false;
    result.onlyContainsDisabledRules = result.positiveAND = true;

    QVector<Rule *>::const_iterator it;
    const QVector<Rule *>::const_iterator &ruleListEnd = ruleList->constEnd();
    for (it = ruleList->constBegin(); it != ruleListEnd; it++)
    {
        const Rule *rule = *it;
        if (!rule->enabled)
            continue;

        result.onlyContainsDisabledRules = false;
        result.result = resolveRule(currentBar, rule , transactionType);

        if ((rule->logicalOperator == Rule::AND) && !result.result)
        {
            result.positiveAND = false;
            return result;
        }

        if (result.result && (rule->logicalOperator == Rule::OR))
        {
            result.positiveOR = true;
            return result;
        }
    }

    return result;
}

RuleListResult RuleList::resolveRuleList(int currentBar, TransactionType transactionType, int transactionStart) const
{
    struct RuleListResult result;
    if (privateData->currentCache->resultsCache()[transactionType][currentBar] != -1)
    {
        result.result = privateData->currentCache->resultsCache()[transactionType][currentBar] ? true : false;
        return result;
    }

    result = doResolveRuleList(currentBar, transactionType, &privateData->ruleList);
    privateData->currentCache->resultsCache()[transactionType][currentBar] = result.result;
    if (currentBar > privateData->currentCache->signalCache()[transactionType][transactionStart])
        privateData->currentCache->signalCache()[transactionType][transactionStart] = currentBar;

    return result;
}

PlotCurve *RuleList::findCurve(const QString &title, const Indicator *indicator) const
{
    if (!indicator)
    {
        if (title.contains("Price Open") || title.contains(QApplication::tr("Prijs Open")))
            return privateData->priceChart->averagePriceOpen;
        else if (title.contains("Price Close") || title.contains(QApplication::tr("Prijs Close")))
            return privateData->priceChart->averagePriceClose;
        else if (title.contains("Price High") || title.contains(QApplication::tr("Prijs High")))
            return privateData->priceChart->averagePriceHigh;
        else if (title.contains("Price Low") || title.contains(QApplication::tr("Prijs Low")))
            return privateData->priceChart->averagePriceLow;
        else
            return NULL;
    }

    for (int i = 0; i < indicator->curveList.size(); ++i)
    {
        if (indicator->curveList[ i ]->title() == title)
            return indicator->curveList[ i ];
    }

    return NULL;
}

Indicator *RuleList::findIndicator(const QString &title) const
{
    if (title.isEmpty())
        return NULL;

    if (title.compare(QApplication::tr("NULL"), Qt::CaseInsensitive) == 0)
        return NULL;

    if (title.compare(QApplication::tr("this"), Qt::CaseInsensitive) == 0)
        return privateData->indicator;

    for (int i = privateData->priceChart->indicatorList.size() - 1; i >= 0; i--)
    {
        if (privateData->priceChart->indicatorList[ i ]->windowTitle() == title)
            return privateData->priceChart->indicatorList[ i ];
    }

    return NULL;
}

void RuleList::initRuleList(const QVector<Rule *> *ruleList, Rule *parent)
{
    for (int i = 0; i < ruleList->size(); i++)
    {
        Rule *rule = addRule(ruleList->at(i)->title, parent);
        if (!rule)
            continue;

        initRuleList(&ruleList->at(i)->children, rule);
    }
}

void RuleList::disconnectRuleList(const QVector<Rule *> *ruleList) const
{
    const QVector<Rule *> *list = ruleList ? ruleList : &privateData->ruleList;
    for (int i = 0; i < list->size(); i++)
    {
        Rule *rule = const_cast<Rule *>(list->at(i));
        disconnectRuleIndicators(rule);
    }
}

void RuleList::disconnectRuleIndicators(Rule *rule) const
{
    if (rule->sourceIndicator)
    {
        QApplication::disconnect(rule->sourceIndicator, SIGNAL(titleChanged(Indicator *, QString)), rule->indicator, SLOT(renameRuleToNewTitle(Indicator *, QString)));
        if (rule->sourceIndicator != rule->indicator)
        {
            rule->sourceIndicator->decreaseExternalReferenceCount();
            QApplication::disconnect(rule->sourceIndicator, SIGNAL(propertiesChanged()), rule->indicator, SLOT(executeTransactionRules()));
            rule->indicator->autoOptimizer().removeIndicator(rule->sourceIndicator);
        }
    }

    if (rule->referenceIndicator)
    {
        QApplication::disconnect(rule->referenceIndicator, SIGNAL(titleChanged(Indicator *, QString)), rule->indicator, SLOT(renameRuleToNewTitle(Indicator *, QString)));
        if (rule->referenceIndicator != rule->indicator)
        {
            rule->referenceIndicator->decreaseExternalReferenceCount();
            QApplication::disconnect(rule->referenceIndicator, SIGNAL(propertiesChanged()), rule->indicator, SLOT(executeTransactionRules()));
            rule->indicator->autoOptimizer().removeIndicator(rule->referenceIndicator);
        }
    }

    while (!rule->autoOptimizerOptionList.isEmpty())
        rule->indicator->autoOptimizer().removeOption(rule->autoOptimizerOptionList.takeFirst());

    disconnectRuleList(&rule->children);
}

QString RuleList::writeRulesToString(QVector<Rule *> *ruleList)
{
    if (!ruleList)
        return QString();

    QString ruleString;
    for (int i = 0; i < ruleList->size(); ++i)
    {
        Rule *rule = ruleList->at(i);
        rule->updateTitle();
        ruleString.append(rule->title);
        QString subList = writeRulesToString(&rule->children);
        if (!subList.isEmpty())
            ruleString.append(" <LIST " + subList + "/LIST>");

        ruleString.append("; ");
    }

    return ruleString;
}

QByteArray RuleList::createCacheKey(QVector<Rule *> *ruleList)
{
    if (!ruleList)
        return QByteArray();

    QByteArray ruleArray;
    for (int i = 0; i < ruleList->size(); ++i)
    {
        Rule *rule = ruleList->at(i);
        ruleArray.append(rule->title);
        ruleArray.append(';');
        ruleArray.append((const char *)&rule->enabled, sizeof(rule->enabled));
        ruleArray.append(';');
        ruleArray.append((const char *)&rule->offset, sizeof(rule->offset));
        ruleArray.append(';');
        ruleArray.append((const char *)&rule->referenceValue, sizeof(rule->referenceValue));
        ruleArray.append(';');
        QByteArray subList = createCacheKey(&rule->children);
        if (!subList.isEmpty())
            ruleArray.append(subList);
        ruleArray.append(';');
    }

    return ruleArray;
}

void RuleList::createRuleResolvementCache(void)
{
    QByteArray key = createCacheKey(&privateData->ruleList);

    if (privateData->ruleResolvementCache.contains(key))
    {
        privateData->currentCache = privateData->ruleResolvementCache.object(key);
        return;
    }

    SignalCacheOject *object = new SignalCacheOject(privateData->priceChart->indexPlot->periods);
    privateData->ruleResolvementCache.insert(key, object);
    privateData->currentCache = object;
}

void RuleList::clearRuleResolvementCache(void)
{
    privateData->ruleResolvementCache.clear();
    privateData->currentCache = NULL;
}

void RuleList::updateSignalCacheForTransaction(int currentBar, TransactionType transactionType, int transactionStart)
{
    if (currentBar > privateData->currentCache->signalCache()[transactionType][transactionStart])
        privateData->currentCache->signalCache()[transactionType][transactionStart] = currentBar;
}

SignalCacheOject *RuleList::currentCacheObject() const
{
    return privateData->currentCache;
}
