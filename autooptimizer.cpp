#include "autooptimizer.h"
#include "autooptimizerworker.h"
#include "ui_autooptimizer.h"
#include "indicator.h"
#include <qdialogbuttonbox.h>
#include <qtconcurrentrun.h>
#include <qscrollbar.h>
#include <unistd.h>

class TableWidgetItem : public QTableWidgetItem
{
    public:
        virtual bool operator< (const QTableWidgetItem &other) const;
        TradeResults results;
};

bool TableWidgetItem::operator< (const QTableWidgetItem &other) const
{
    bool ok;
    QString valueString = text().replace(" %", "");
    QString otherValueString = other.text().replace(" %", "");

    float value = valueString.toFloat(&ok);
    if (!ok)
    {
        qDebug() << "TableWidgetItem::operator <(): could not convert value string " << valueString << " to valid float";
        return QTableWidgetItem::operator< (other);
    }

    float otherValue = otherValueString.toFloat(&ok);
    if (!ok)
    {
        qDebug() << "TableWidgetItem::operator <(): could not convert other value string" << otherValueString << " to valid float";
        return QTableWidgetItem::operator< (other);
    }

    return value < otherValue;
}

class AutoOptimizer::PrivateData
{
    public:
        Indicator *indicator;
        QList<AutoOptimizerOption *> optionsList;
        QList<IndicatorReference *> indicatorList;
        QList<AutoOptimizerWorker *> workers;
        QList<QThread *> workerThreads;
        QTimer priorityTimer;
        QVector<TradeResults> tradeResultsList;
        QReadWriteLock tradeResultsTableLock;
        SessionInfo sessionInfo;
        Ui::AutoOptimizerUI form;
        QPushButton *startButton;
        QPushButton *stopButton;
        mutable QMutex progressMutex;
        QVariant taskMin;
        QVariant taskMax;
        QTimer updateResultsWidgetTimer;
        bool running;
        mutable bool pause;
        bool stopProcessing;
        qulonglong totalAvailableOptions;
        qulonglong lastProgressValueUpdate;
        qulonglong progressValue;
        QTime progressTimer;
        int progressFactor;
};

AutoOptimizer::AutoOptimizer(Indicator *indicator, Qt::WindowFlags f) : QDialog(indicator, f)
{
    //    privateData = new PrivateData;
    privateData = (PrivateData *) _mm_malloc(sizeof(PrivateData), 16);
    new(privateData) PrivateData();
    privateData->indicatorList << new IndicatorReference(indicator);
    privateData->indicator = indicator;
    privateData->running = false;
    privateData->pause = false;
    privateData->totalAvailableOptions = 0;
    privateData->progressFactor = 1;
    privateData->form.setupUi(this);
    privateData->form.optionsScrollArea->setVisible(false);
    privateData->form.showOptionsPushButton->setIcon(qApp->style()->standardIcon(QStyle::SP_ArrowDown));
    connect(privateData->form.showOptionsPushButton, SIGNAL(toggled(bool)), privateData->form.optionsScrollArea, SLOT(setVisible(bool)));
    privateData->form.textLabel->setStyleSheet("QLabel { background-color: " + indicator->color().name() +  "; }");

    QMovie *labelMovie = new QMovie(":/images/Gears-02-june.gif");
    labelMovie->setCacheMode(QMovie::CacheAll);
    labelMovie->setScaledSize(QSize(55, 45));
    labelMovie->start();
    privateData->form.iconLabel->setMovie(labelMovie);
    privateData->form.iconLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    privateData->form.iconLabel->setStyleSheet("QLabel { background-color: qlineargradient(x1: 0, y1: 0, x2: 0.8, y2: 0, stop: 0 " + indicator->color().name() + ", stop: 1 white ); }");
    privateData->form.spaceLabel->setStyleSheet("QLabel { background-color: white; }");

    privateData->updateResultsWidgetTimer.setInterval(100);
    privateData->updateResultsWidgetTimer.setSingleShot(true);
    connect(&privateData->updateResultsWidgetTimer, SIGNAL(timeout()), this, SLOT(fillResultsWidget()));
    connect(this, SIGNAL(rejected()), this, SLOT(stop()));

    privateData->priorityTimer.setInterval(10);
    connect(&privateData->priorityTimer, SIGNAL(timeout()), this, SLOT(updateThreadPriorities()));

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);
    layout()->addWidget(buttonBox);
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(quit()));
    privateData->startButton = buttonBox->addButton("Start", QDialogButtonBox::ActionRole);
    connect(privateData->startButton, SIGNAL(clicked()), this, SLOT(start()));
    privateData->stopButton = buttonBox->addButton("Stop", QDialogButtonBox::ActionRole);
    privateData->stopButton->setDisabled(true);
    connect(privateData->stopButton, SIGNAL(clicked()), this, SLOT(stop()));

    setMaximumResultsVisible(20);
    setShowDoubleResults(false);
    setShowNegativeResults(true);
    setDeadStrategyCutOffRate(20.0);
    setMaximumAllowedCapitalLoss(20.0);
    setMinimumRequieredTrades(0);
    setMinimumRequieredSuccessRate(0.0);

    qRegisterMetaType<TradeResults> ("TradeResults&");
    qRegisterMetaType<TradeResults> ("TradeResults");

    connect(privateData->form.resultsTableWidget, SIGNAL(cellActivated(int, int)), this, SLOT(plotSelectedResultEntry(int, int)));
}

AutoOptimizer::~AutoOptimizer(void)
{
    //    delete privateData;
    privateData->~PrivateData();
    _mm_free(privateData);
}

void AutoOptimizer::testSlot(void)
{
}

void AutoOptimizer::addIndicator(Indicator *indicator)
{
    if (!indicator)
        return;

    for (int i = 0; i < privateData->indicatorList.size(); ++i)
    {
        if (indicator == privateData->indicatorList[ i ]->indicator)
        {
            privateData->indicatorList[ i ]->referenceCount++;
            return;
        }
    }

    privateData->indicatorList << new IndicatorReference(indicator);

    const QList<AutoOptimizerOption *> list = indicator->autoOptimizer().listOptions();
    for (int i = 0; i < list.size(); ++i)
        addOptionToInterface(list.at(i));
}

void AutoOptimizer::removeIndicator(Indicator *indicator)
{
    if (!indicator)
        return;

    for (int i = 0; i < privateData->indicatorList.size(); ++i)
    {
        if (indicator == privateData->indicatorList.at(i)->indicator)
        {
            privateData->indicatorList.at(i)->referenceCount--;
            if (privateData->indicatorList.at(i)->referenceCount <= 0 && privateData->indicatorList.at(i)->indicator != privateData->indicator)
            {
                QList<AutoOptimizerOption *> list = indicator->autoOptimizer().listOptions();
                for (int j = 0; j < list.size(); ++j)
                    removeOptionFromInterface(list.at(j));

                delete privateData->indicatorList.takeAt(i);
            }

            return;
        }
    }
}

Indicator *AutoOptimizer::indicator() const
{
    return privateData->indicator;
}

QList<Indicator *> AutoOptimizer::indicators(void) const
{
    QList<Indicator *> indicatorList;
    for (int i = 0; i < privateData->indicatorList.size(); ++i)
        indicatorList << privateData->indicatorList.at(i)->indicator;

    return indicatorList;
}

QList< AutoOptimizerOption * > AutoOptimizer::listOptions(AutoOptimizerOption::optionTypes type) const
{
    QList<AutoOptimizerOption *> list;

    for (int i = 0; i < privateData->optionsList.size(); ++i)
    {
        AutoOptimizerOption *option = privateData->optionsList[ i ];
        if (type & option->type())
            list << option;
    }

    return list;
}

AutoOptimizerOption *AutoOptimizer::addOption(Indicator *indicator, const QString &title, bool *option, int min, int max, AutoOptimizerOption::optionTypes type, const QStringList &valueTitles)
{
    AutoOptimizerOption *newOption = new AutoOptimizerOption(option, indicator, valueTitles);
    newOption->setType(type);
    newOption->setTitle(title);
    newOption->setMin(min);
    newOption->setMax(max);

    addOptionToInterface(newOption);
    connect(newOption, SIGNAL(guiTouched()), this, SLOT(updateGui()));

    return newOption;
}

AutoOptimizerOption *AutoOptimizer::addOption(Indicator *indicator, const QString &title, int *option, int min, int max, int stepSize, bool useAccelleration, AutoOptimizerOption::optionTypes type, const QStringList &valueTitles)
{
    AutoOptimizerOption *newOption = new AutoOptimizerOption(option, indicator, valueTitles);
    newOption->setTitle(title);
    newOption->setType(type);
    newOption->setMin(min);
    newOption->setMax(max);
    newOption->setStepSize(stepSize);
    newOption->setUseAcceleration(useAccelleration);

    addOptionToInterface(newOption);
    connect(newOption, SIGNAL(guiTouched()), this, SLOT(updateGui()));

    return newOption;
}

AutoOptimizerOption *AutoOptimizer::addOption(Indicator *indicator, const QString &title, float *option, float min, float max, float stepSize, bool useAccelleration, bool percentualNotation, AutoOptimizerOption::optionTypes type)
{
    AutoOptimizerOption *newOption = new AutoOptimizerOption(option, indicator, percentualNotation);
    newOption->setTitle(title);
    newOption->setType(type);
    newOption->setMin(min);
    newOption->setMax(max);
    newOption->setStepSize(stepSize);
    newOption->setUseAcceleration(useAccelleration);

    addOptionToInterface(newOption);
    connect(newOption, SIGNAL(guiTouched()), this, SLOT(updateGui()));

    return newOption;
}

AutoOptimizerOption *AutoOptimizer::addOption(Indicator *indicator, const QString &title, float *option, float min, int periods, bool useAccelleration, bool percentualNotation, AutoOptimizerOption::optionTypes type)
{
    AutoOptimizerOption *newOption = new AutoOptimizerOption(option, indicator, percentualNotation);
    newOption->setTitle(title);
    newOption->setType(type);
    newOption->setMin(min);
    newOption->setAutoAdjustPeriods(periods);
    newOption->setAutoAdjustEnabled(true);
    newOption->setUseAcceleration(useAccelleration);

    addOptionToInterface(newOption);
    connect(newOption, SIGNAL(guiTouched()), this, SLOT(updateGui()));

    return newOption;
}

void AutoOptimizer::removeOption(AutoOptimizerOption *option)
{
    removeOptionFromInterface(option);
    delete option;
}


void AutoOptimizer::addOptionToInterface(AutoOptimizerOption *option)
{
    privateData->optionsList << option;
    resortOptionList();

    QFormLayout *groupLayout;
    int preferedWidth;

    switch (option->type())
    {
        case AutoOptimizerOption::IndicatorOption:
        {
            bool found = false;
            QGroupBox *indicatorGroupBox;

            for (int i = 0; i < privateData->form.indicatorGroupBox->layout()->count(); ++i)
            {
                indicatorGroupBox = dynamic_cast<QGroupBox *>(privateData->form.indicatorGroupBox->layout()->itemAt(i)->widget());
                if (!indicatorGroupBox)
                    continue;

                if (indicatorGroupBox->title() != option->indicator()->title())
                    continue;

                found = true;

                groupLayout = dynamic_cast<QFormLayout *>(indicatorGroupBox->layout());
                groupLayout->addRow(option->title(), option);
                preferedWidth = privateData->form.indicatorGroupBox->sizeHint().width();
                preferedWidth = indicatorGroupBox->sizeHint().width();
                break;
            }

            if (found)
                break;

            indicatorGroupBox = new QGroupBox(privateData->form.indicatorGroupBox);
            indicatorGroupBox->setTitle(option->indicator()->title());
            groupLayout = new QFormLayout(indicatorGroupBox);
            indicatorGroupBox->setLayout(groupLayout);
            groupLayout->addRow(option->title(), option);
            groupLayout = dynamic_cast<QFormLayout *>(privateData->form.indicatorGroupBox->layout());
            groupLayout->addRow(indicatorGroupBox);
            preferedWidth = privateData->form.indicatorGroupBox->sizeHint().width();
            preferedWidth = indicatorGroupBox->sizeHint().width();
            break;
        }

        case AutoOptimizerOption::TradeOption:
            groupLayout = dynamic_cast<QFormLayout *>(privateData->form.tradeGroupBox->layout());
            groupLayout->addRow(option->title(), option);
            preferedWidth = privateData->form.tradeGroupBox->sizeHint().width();
            break;

        case AutoOptimizerOption::StopLossOption:
            groupLayout = dynamic_cast<QFormLayout *>(privateData->form.stopLossGroupBox->layout());
            groupLayout->addRow(option->title(), option);
            preferedWidth = privateData->form.stopLossGroupBox->sizeHint().width();
            break;

        case AutoOptimizerOption::TakeProfitOption:
            groupLayout = dynamic_cast<QFormLayout *>(privateData->form.takeProfitGroupBox->layout());
            groupLayout->addRow(option->title(), option);
            preferedWidth = privateData->form.takeProfitGroupBox->sizeHint().width();
            break;

        case AutoOptimizerOption::RuleOption:
        case AutoOptimizerOption::ExitRuleOption:
        case AutoOptimizerOption::LongRuleOption:
        case AutoOptimizerOption::ShortRuleOption:
            groupLayout = dynamic_cast<QFormLayout *>(privateData->form.rulesGroupBox->layout());
            groupLayout->addRow(option->title(), option);
            preferedWidth = privateData->form.rulesGroupBox->sizeHint().width();
            break;

        default:
            groupLayout = dynamic_cast<QFormLayout *>(privateData->form.indicatorGroupBox->layout());
            groupLayout->addRow(option->title(), option);
            preferedWidth = privateData->form.indicatorGroupBox->sizeHint().width();
            break;
    }

    if ((preferedWidth + (layout()->margin() * 3)) > privateData->form.optionsScrollArea->sizeHint().width())
        resize(preferedWidth + (layout()->margin() * 3), height());

    int minimumWidth = 0;
    int indicatorLayoutIt = 0;
    for (int i = 0; i < 5; i++)
    {
        groupLayout = NULL;

        if (i == 0)
        {
            if (indicatorLayoutIt < privateData->form.indicatorGroupBox->layout()->count())
            {
                QGroupBox *indicatorGroupBox = dynamic_cast<QGroupBox *>(privateData->form.indicatorGroupBox->layout()->itemAt(indicatorLayoutIt)->widget());
                if (!indicatorGroupBox)
                {
                    i--;
                    indicatorLayoutIt++;
                    continue;
                }

                groupLayout = dynamic_cast<QFormLayout *>(indicatorGroupBox->layout());
                i--;
                indicatorLayoutIt++;
            }
        }
        else if (i == 1)
            groupLayout = dynamic_cast<QFormLayout *>(privateData->form.tradeGroupBox->layout());
        else if (i == 2)
            groupLayout = dynamic_cast<QFormLayout *>(privateData->form.stopLossGroupBox->layout());
        else if (i == 3)
            groupLayout = dynamic_cast<QFormLayout *>(privateData->form.takeProfitGroupBox->layout());
        else if (i == 4)
            groupLayout = dynamic_cast<QFormLayout *>(privateData->form.rulesGroupBox->layout());
        else
            continue;

        if (!groupLayout)
            continue;

        for (int j = 0; j < groupLayout->rowCount(); j++)
        {
            QLayoutItem *layoutItem = groupLayout->itemAt(j, QFormLayout::LabelRole);
            if (!layoutItem)
                continue;

            QLabel *label = dynamic_cast<QLabel *>(layoutItem->widget());
            if (!label)
                continue;

            if (label->sizeHint().width() > minimumWidth)
                minimumWidth = label->sizeHint().width();
        }
    }

    if (minimumWidth > 0)
    {
        indicatorLayoutIt = 0;
        for (int i = 0; i < 5; i++)
        {
            groupLayout = NULL;

            if (i == 0)
            {
                if (indicatorLayoutIt < privateData->form.indicatorGroupBox->layout()->count())
                {
                    QGroupBox *indicatorGroupBox = dynamic_cast<QGroupBox *>(privateData->form.indicatorGroupBox->layout()->itemAt(indicatorLayoutIt)->widget());
                    if (!indicatorGroupBox)
                    {
                        i--;
                        indicatorLayoutIt++;
                        continue;
                    }

                    groupLayout = dynamic_cast<QFormLayout *>(indicatorGroupBox->layout());
                    i--;
                    indicatorLayoutIt++;
                }
            }
            else if (i == 1)
                groupLayout = dynamic_cast<QFormLayout *>(privateData->form.tradeGroupBox->layout());
            else if (i == 2)
                groupLayout = dynamic_cast<QFormLayout *>(privateData->form.stopLossGroupBox->layout());
            else if (i == 3)
                groupLayout = dynamic_cast<QFormLayout *>(privateData->form.takeProfitGroupBox->layout());
            else if (i == 4)
                groupLayout = dynamic_cast<QFormLayout *>(privateData->form.rulesGroupBox->layout());
            else
                continue;

            if (!groupLayout)
                continue;

            for (int j = 0; j < groupLayout->rowCount(); j++)
            {
                QLayoutItem *layoutItem = groupLayout->itemAt(j, QFormLayout::LabelRole);
                if (!layoutItem)
                    continue;

                QLabel *label = dynamic_cast<QLabel *>(layoutItem->widget());
                if (!label)
                    continue;

                label->setMinimumWidth(minimumWidth);
            }
        }
    }
}

void AutoOptimizer::removeOptionFromInterface(AutoOptimizerOption *option)
{
    for (int i = 0; i < privateData->optionsList.size(); ++i)
    {
        if (privateData->optionsList.at(i) == option)
            privateData->optionsList.takeAt(i);
    }

    resortOptionList();

    switch (option->type())
    {
        case AutoOptimizerOption::IndicatorOption:
            privateData->form.indicatorGroupBox->layout()->removeWidget(option);
            break;

        case AutoOptimizerOption::TradeOption:
            privateData->form.tradeGroupBox->layout()->removeWidget(option);
            break;

        case AutoOptimizerOption::StopLossOption:
            privateData->form.stopLossGroupBox->layout()->removeWidget(option);
            break;

        case AutoOptimizerOption::TakeProfitOption:
            privateData->form.takeProfitGroupBox->layout()->removeWidget(option);
            break;

        case AutoOptimizerOption::RuleOption:
        case AutoOptimizerOption::ExitRuleOption:
        case AutoOptimizerOption::LongRuleOption:
        case AutoOptimizerOption::ShortRuleOption:
        {
            int index = privateData->form.rulesGroupBox->layout()->indexOf(option);
            if (index != - 1)
            {
                QLayoutItem *layoutItem = privateData->form.rulesGroupBox->layout()->itemAt(index - 1);
                privateData->form.rulesGroupBox->layout()->removeItem(layoutItem);
                delete layoutItem->widget();
                delete layoutItem;
            }
            privateData->form.rulesGroupBox->layout()->removeWidget(option);
            break;
        }
        default:
            privateData->form.indicatorGroupBox->layout()->removeWidget(option);
            break;
    }
}

bool AutoOptimizer::moveDependenciesForward(const AutoOptimizerOption *option)
{
    int optionIndex = privateData->optionsList.indexOf(const_cast<AutoOptimizerOption *>(option));
    if (optionIndex < 0 || optionIndex >= privateData->optionsList.size())
        return false;

    if (!option->hasDependencies())
        return false;

    bool listChanged = false;
    QVector<Dependency *> dependencyList = option->dependencies();
    for (int i = 0; i < dependencyList.size(); ++i)
    {
        Dependency *dependency = dependencyList.at(i);
        int dependencyIndex = privateData->optionsList.indexOf(dependency->dependencyOption);
        if (dependencyIndex < 0 || dependencyIndex >= privateData->optionsList.size())
            continue;

        if (dependencyIndex < optionIndex)
            continue;

        privateData->optionsList.move(dependencyIndex, optionIndex);
        moveDependenciesForward(dependency->dependencyOption);
        int optionIndex = privateData->optionsList.indexOf(const_cast<AutoOptimizerOption *>(option));
        if (optionIndex < 0 || optionIndex >= privateData->optionsList.size())
            return true;

        listChanged = true;
    }

    return listChanged;
}

void AutoOptimizer::resortOptionList(void)
{
    QList<AutoOptimizerOption *> newList;
    QList<AutoOptimizerOption *> indicatorList;
    QList<AutoOptimizerOption *> ruleList;
    AutoOptimizerOption *option;

    for (int i = 0; i < privateData->optionsList.size(); ++i)
    {
        option = privateData->optionsList.at(i);
        if (option->type() != AutoOptimizerOption::IndicatorOption)
            continue;

        indicatorList << option;
    }

    for (int i = 0; i < privateData->optionsList.size(); ++i)
    {
        option = privateData->optionsList.at(i);
        if (option->type() != AutoOptimizerOption::RuleOption)
            continue;

        ruleList << option;
    }

    for (int i = 0; i < privateData->optionsList.size(); ++i)
    {
        option = privateData->optionsList.at(i);
        if (option->type() != AutoOptimizerOption::ExitRuleOption)
            continue;

        ruleList << option;
    }

    for (int i = 0; i < privateData->optionsList.size(); ++i)
    {
        option = privateData->optionsList.at(i);
        if (option->type() != AutoOptimizerOption::LongRuleOption)
            continue;

        ruleList << option;
    }

    for (int i = 0; i < privateData->optionsList.size(); ++i)
    {
        option = privateData->optionsList.at(i);
        if (option->type() != AutoOptimizerOption::ShortRuleOption)
            continue;

        ruleList << option;
    }

    newList << indicatorList;
    newList << ruleList;

    for (int i = 0; i < privateData->optionsList.size(); ++i)
    {
        option = privateData->optionsList.at(i);
        if (option->type() == AutoOptimizerOption::IndicatorOption  ||
            option->type() == AutoOptimizerOption::RuleOption ||
            option->type() == AutoOptimizerOption::ExitRuleOption ||
            option->type() == AutoOptimizerOption::LongRuleOption ||
            option->type() == AutoOptimizerOption::ShortRuleOption)
        {
            continue;
        }

        newList << option;
    }

    privateData->optionsList = newList;

    for (int i = 0; i < privateData->optionsList.size(); ++i)
    {
        bool listChanged = moveDependenciesForward(privateData->optionsList.at(i));
        if (listChanged)
            i = 0;
    }
}

void AutoOptimizer::launchWorkerThreads(void)
{
    int threadCount = QThread::idealThreadCount() + 2;
    if (threadCount < 1)
        threadCount = 1;

    if (USE_AUTOOPTIMIZER_THREADS <= 0)
        threadCount = 1;

    //    threadCount = 1;

    int threadOptionCount = privateData->optionsList.at(0)->possibleOptionsCount();
    int cnt;
    for (cnt = threadCount; cnt > 0; cnt--)
    {
        if (privateData->optionsList.isEmpty())
        {
            cnt = 1;
            break;
        }

        threadOptionCount = privateData->optionsList.at(0)->possibleOptionsCount() / cnt;
        if (threadOptionCount >= 1)
            break;
    }
    threadCount = cnt;

    qDebug() << "AutoOptimizer::launchWorkerThreads(): launching " << threadCount << " threads";

    float border = privateData->optionsList.at(0)->min().toFloat();
    float stepSize = privateData->optionsList.at(0)->stepSize().toFloat();
    float accelleration = privateData->optionsList.at(0)->useAcceleration() ? 1.1f : 1.0f;
    AutoOptimizerOption::variableTypes varType = privateData->optionsList.at(0)->varType();

    qDebug() << "\n111";
    privateData->indicator->writeSettings();

    for (int i = 0; i < threadCount; i++)
    {
        AutoOptimizerWorker *worker = new AutoOptimizerWorker(this);
        QThread *workerThread = new QThread(this);
        connect(workerThread, SIGNAL(started()), worker, SLOT(start()));
        connect(workerThread, SIGNAL(terminated()), this, SLOT(finished()));
        connect(worker, SIGNAL(result(TradeResults)), this, SLOT(addResult(TradeResults)));
        connect(worker, SIGNAL(finished()), this, SLOT(finished()));
        worker->moveToThread(workerThread);
        privateData->workerThreads << workerThread;
        privateData->workers << worker;

        worker->setTaskStartPoint(border);
        if (i == (threadCount - 1))
        {
            worker->setTaskEndPoint(privateData->optionsList.at(0)->max().toFloat());
            worker->setTotalAvailableOptions((totalAvailableOptions() / privateData->optionsList.at(0)->possibleOptionsCount()) * (privateData->optionsList.at(0)->possibleOptionsCount() - (threadOptionCount * (threadCount - 1))));
        }
        else
        {
            for (int j = 1; j < threadOptionCount; j++)
            {
                if (varType == AutoOptimizerOption::Double)
                    border = (border * accelleration) + stepSize;
                else
                    border = static_cast<int>((border * accelleration) + stepSize);
            }

            worker->setTaskEndPoint(border);
            worker->setTotalAvailableOptions((totalAvailableOptions() / privateData->optionsList.at(0)->possibleOptionsCount()) * threadOptionCount);
        }

        if (varType == AutoOptimizerOption::Double)
            border = (border * accelleration) + stepSize;
        else
            border = static_cast<int>((border * accelleration) + stepSize);

    }

    for (int i = privateData->workerThreads.size() - 1; i >= 0; i--)
        privateData->workerThreads.at(i)->start();
}

void AutoOptimizer::resortRuleLists(void)
{
    if (USE_RULE_RESORTING <= 0)
        return;

    QList<IndicatorRule *> bestList;
    QList<IndicatorRule *> list;
    long double totalEvaluations;
    int size;
    bool firstRun;
    Indicator *indicator = privateData->indicator;

    list = indicator->longRuleList;
    bestList = list;
    size = list.size();
    totalEvaluations = LDBL_MAX;
    firstRun = true;
    for (int i = 0; i < size; i++)
    {
        int element = i;
        while (true)
        {
            if (element != i || firstRun)
            {
                indicator->totalEvaluations = 0;
                if (indicator->totalEvaluations < totalEvaluations)
                {
                    totalEvaluations = indicator->totalEvaluations;
                    bestList = list;
                }
            }

            firstRun = false;
            if (element == (size - 1))
            {
                IndicatorRule *rule = list.takeAt(element);
                list.insert(i, rule);
                indicator->longRuleList = list;
                break;
            }

            element++;
            list.swap(element, element - 1);
            indicator->longRuleList = list;
        }
    }
    indicator->longRuleList = bestList;

    list = indicator->shortRuleList;
    bestList = list;
    size = list.size();
    totalEvaluations = LDBL_MAX;
    firstRun = true;
    for (int i = 0; i < size; i++)
    {
        int element = i;
        while (true)
        {
            if (element != i || firstRun)
            {
                indicator->totalEvaluations = 0;
                //                indicator->createRuleResolvementCache();
                if (indicator->totalEvaluations < totalEvaluations)
                {
                    totalEvaluations = indicator->totalEvaluations;
                    bestList = list;
                }
            }

            firstRun = false;
            if (element == (size - 1))
            {
                IndicatorRule *rule = list.takeAt(element);
                list.insert(i, rule);
                indicator->shortRuleList = list;
                break;
            }

            element++;
            list.swap(element, element - 1);
            indicator->shortRuleList = list;
        }
    }
    indicator->shortRuleList = bestList;

    list = indicator->exitRuleList;
    bestList = list;
    size = list.size();
    totalEvaluations = LDBL_MAX;
    firstRun = true;
    for (int i = 0; i < size; i++)
    {
        int element = i;
        while (true)
        {
            if (element != i || firstRun)
            {
                indicator->totalEvaluations = 0;
                if (indicator->totalEvaluations < totalEvaluations)
                {
                    totalEvaluations = indicator->totalEvaluations;
                    bestList = list;
                }
            }

            firstRun = false;
            if (element == (size - 1))
            {
                IndicatorRule *rule = list.takeAt(element);
                list.insert(i, rule);
                indicator->exitRuleList = list;
                break;
            }

            element++;
            list.swap(element, element - 1);
            indicator->exitRuleList = list;
        }
    }
    indicator->exitRuleList = bestList;
}

bool AutoOptimizer::isRunning(void) const
{
    return privateData->running;
}

void AutoOptimizer::pause(void) const
{
    privateData->pause = true;
}

void AutoOptimizer::resume(void) const
{
    privateData->pause = false;
}

void AutoOptimizer::start(void)
{
    privateData->pause = false;
    privateData->startButton->setDisabled(true);
    privateData->form.optionsScrollAreaWidgetContents->setEnabled(false);
    privateData->form.resultOptionsWidget->setEnabled(false);
    strategie->setDataSearchWidgetEnabledForPriceChart(false, privateData->indicator->priceChart);
    for (int i = 0; i < privateData->indicatorList.size(); ++i)
        privateData->indicatorList.at(i)->indicator->setSettingsGUIEnabled(false);
    privateData->form.resultsTableWidget->setSortingEnabled(false);

    qulonglong optionCount = totalAvailableOptions();
    privateData->totalAvailableOptions = optionCount;
    privateData->lastProgressValueUpdate = 0;
    privateData->progressValue = 0;
    privateData->progressFactor = 1;
    privateData->form.progressBar->setMinimum(0);
    if (optionCount >= INT_MAX)
    {
        for (int i = 2; i < INT_MAX; i++)
        {
            if ((optionCount / i) < INT_MAX)
            {
                privateData->progressFactor = i;
                break;
            }
        }
    }
    privateData->form.progressBar->setMaximum(optionCount / privateData->progressFactor);
    privateData->form.progressBar->setValue(0);
    privateData->form.progressBar->setFormat("%p%");
    updateProgressBar();

    qDebug() << "total calculations=" << optionCount << " progressFactor=" << privateData->progressFactor;

    clearResults();
    launchWorkerThreads();
    privateData->priorityTimer.start();

    privateData->stopButton->setDisabled(false);
    privateData->running = true;

    if (privateData->workerThreads.isEmpty())
        finished();
}

void AutoOptimizer::stop(void)
{
    qDebug() << "AutoOptimizer::stop()";
    privateData->stopButton->setDisabled(true);

    for (int i = 0; i < privateData->workers.size(); i++)
        privateData->workers.at(i)->stop();
}

void AutoOptimizer::finished(void)
{
    qDebug() << "AutoOptimizer::finished()";
    privateData->indicator->writeSettings();

    for (int i = 0; i < privateData->workers.size(); i++)
    {
        if (privateData->workers.at(i)->isRunning())
        {
            qDebug() << "not all threads exited, returning";
            QTimer::singleShot(1000, this, SLOT(finished()));
            return;
        }
    }

    privateData->stopButton->setDisabled(true);
    privateData->priorityTimer.stop();
    updateProgressBar();
    fillResultsWidget();

    while (privateData->workers.size())
    {
        AutoOptimizerWorker *worker = privateData->workers.takeFirst();
        delete worker;
    }

    while (privateData->workerThreads.size())
    {
        QThread *workerThread = privateData->workerThreads.takeFirst();
        workerThread->exit();
        workerThread->deleteLater();
    }

    privateData->form.resultsTableWidget->setSortingEnabled(true);
    for (int i = 0; i < privateData->indicatorList.size(); ++i)
        privateData->indicatorList.at(i)->indicator->setSettingsGUIEnabled(true);
    privateData->indicator->priceChart->strategie->setDataSearchWidgetEnabledForPriceChart(true, privateData->indicator->priceChart);
    privateData->form.resultOptionsWidget->setEnabled(true);
    privateData->form.optionsScrollAreaWidgetContents->setEnabled(true);
    privateData->startButton->setDisabled(false);
    privateData->running = false;

    for (int i = 0; i < privateData->tradeResultsList.size(); i++)
    {
        qDebug() << "trade results at " << i << "=" << privateData->tradeResultsList.at(i).relativeProfits;
    }
}

void AutoOptimizer::quit(void)
{
    stop();
    reject();
}

void AutoOptimizer::updateGui(void)
{
    return;

    for (int i = 0; i < privateData->optionsList.size(); ++i)
    {
        AutoOptimizerOption *option = privateData->optionsList.at(i);
        QVector<Dependency *> dependencies = option->dependencies();
        bool show = true;
        for (int j = 0; j < dependencies.size(); ++j)
        {
            if (!dependencies.at(j)->dependencyOption->valueCheckBoxEnabled(dependencies.at(j)->value))
            {
                show = false;
            }
        }

        QFormLayout *groupLayout = NULL;
        if (option->type() == AutoOptimizerOption::IndicatorOption)
        {
            for (int i = 0; i < privateData->form.indicatorGroupBox->layout()->count(); ++i)
            {
                QGroupBox *indicatorGroupBox = dynamic_cast<QGroupBox *>(privateData->form.indicatorGroupBox->layout()->itemAt(i)->widget());
                if (!indicatorGroupBox)
                    continue;


                groupLayout = dynamic_cast<QFormLayout *>(indicatorGroupBox->layout());
                int index = groupLayout->indexOf(option);
                if (index != -1)
                    break;
            }
        }
        else if (option->type() == AutoOptimizerOption::TradeOption)
            groupLayout = dynamic_cast<QFormLayout *>(privateData->form.tradeGroupBox->layout());
        else if (option->type() == AutoOptimizerOption::StopLossOption)
            groupLayout = dynamic_cast<QFormLayout *>(privateData->form.stopLossGroupBox->layout());
        else if (option->type() == AutoOptimizerOption::TakeProfitOption)
            groupLayout = dynamic_cast<QFormLayout *>(privateData->form.takeProfitGroupBox->layout());

        if (!groupLayout)
            continue;

        int index = groupLayout->indexOf(option);
        if (index != -1)
        {
            QLayoutItem *layoutItem = groupLayout->itemAt(index - 1);
            if (layoutItem)
            {
                if (show)
                {
                    layoutItem->widget()->show();
                    option->show();
                }
                else
                {
                    layoutItem->widget()->hide();
                    option->hide();
                }
            }
        }
    }
}

class ThreadProgress
{
    public:
        QThread *thread;
        qreal progressRate;
        const AutoOptimizer *ao;
};

void AutoOptimizer::updateThreadPriorities(void)
{
    QVector<AutoOptimizerWorker *> sortedByProgressList;

    for (int i = 0; i < privateData->workers.size(); i++)
    {
        AutoOptimizerWorker *worker = privateData->workers.at(i);
        worker->pause(false);
        worker->thread()->setPriority(QThread::NormalPriority);
        if (!worker->isRunning())
            continue;

        int index;
        for (index = 0; index < sortedByProgressList.size(); index++)
        {
            if (worker->progressRate() > sortedByProgressList.at(index)->progressRate())
                break;
        }
        sortedByProgressList.insert(index, worker);
    }

    if (sortedByProgressList.isEmpty())
        return;

    float difference = sortedByProgressList.at(0)->progressRate() - sortedByProgressList.at(sortedByProgressList.size() - 1)->progressRate();
    int itemsToPause = 0;

    if (difference > 1.0f)
    {
        itemsToPause = itemsToPause + static_cast<int>(difference);
        if (itemsToPause > (sortedByProgressList.size() - 1))
            itemsToPause = sortedByProgressList.size() - 1;
    }

    if (itemsToPause > 0)
    {
        for (int i = 0; i < itemsToPause; i++)
        {
            sortedByProgressList.at(i)->pause(true);
            sortedByProgressList.at(i)->thread()->setPriority(QThread::IdlePriority);
        }
    }
}

int AutoOptimizer::maximumResultsVisible(void) const
{
    return privateData->form.maximumResultsSpinBox->value();
}

void AutoOptimizer::setMaximumResultsVisible(int count)
{
    privateData->form.maximumResultsSpinBox->setValue(count);
}

bool AutoOptimizer::showDoubleResults(void) const
{
    return privateData->form.showDoubleResultsCheckBox->isChecked();
}

void AutoOptimizer::setShowDoubleResults(bool enabled)
{
    privateData->form.showDoubleResultsCheckBox->setChecked(enabled);
}

bool AutoOptimizer::showNegativeResults(void) const
{
    return privateData->form.showNegativeResultsCheckBox->isChecked();
}

void AutoOptimizer::setShowNegativeResults(bool enabled)
{
    privateData->form.showNegativeResultsCheckBox->setChecked(enabled);
}

float AutoOptimizer::deadStrategyCutOffRate(void) const
{
    return static_cast<float>(privateData->form.deadStrategyCutOffRateDoubleSpinBox->value());
}

void AutoOptimizer::setDeadStrategyCutOffRate(float rate)
{
    privateData->form.deadStrategyCutOffRateDoubleSpinBox->setValue(static_cast<double>(rate));
}

float AutoOptimizer::maximumAllowedCapitalLoss(void) const
{
    return static_cast<float>(privateData->form.maximumAllowedCapitalLossDoubleSpinBox->value());
}

void AutoOptimizer::setMaximumAllowedCapitalLoss(float rate)
{
    privateData->form.maximumAllowedCapitalLossDoubleSpinBox->setValue(static_cast<double>(rate));
}

int AutoOptimizer::minimumRequieredTrades(void) const
{
    return privateData->form.minimumRequieredTradesSpinBox->value();
}

void AutoOptimizer::setMinimumRequieredTrades(int trades)
{
    privateData->form.minimumRequieredTradesSpinBox->setValue(trades);
}

float AutoOptimizer::minimumRequieredSuccessRate(void) const
{
    return static_cast<float>(privateData->form.minimumRequieredSuccessRateDoubleSpinBox->value());
}

void AutoOptimizer::setMinimumRequieredSuccessRate(float rate)
{
    privateData->form.minimumRequieredSuccessRateDoubleSpinBox->setValue(static_cast<double>(rate));
}

void AutoOptimizer::setVisible(bool visible)
{
    updateGui();
    QDialog::setVisible(visible);
}

void AutoOptimizer::setColor(const QColor &color)
{
    privateData->form.textLabel->setStyleSheet("QLabel { background-color: " + color.name() +  "; }");
    privateData->form.iconLabel->setStyleSheet("QLabel { background-color: qlineargradient(x1: 0, y1: 0, x2: 0.8, y2: 0, stop: 0 " + color.name() + ", stop: 1 white ); }");
    privateData->form.spaceLabel->setStyleSheet("QLabel { background-color: white; }");
}

void AutoOptimizer::plotSelectedResultEntry(int row, int column)
{
    TableWidgetItem *tableItem = (TableWidgetItem *) privateData->form.resultsTableWidget->item(row, column);
    if (!tableItem)
        return;


    QList<QVariant> selectedSettings = tableItem->results.settings;
    if (selectedSettings.isEmpty())
        return;

    loadSettings(selectedSettings);

    for (int i = 0; i < privateData->indicatorList.size(); ++i)
    {
        privateData->indicatorList.at(i)->indicator->updateGUI();
        privateData->indicatorList.at(i)->indicator->setProperties();
        privateData->indicatorList.at(i)->indicator->plotCurve();
        privateData->indicatorList.at(i)->indicator->writeSettings();
        privateData->indicatorList.at(i)->indicator->writeRules();
    }

    privateData->indicator->executeTransactionRules();
}

void AutoOptimizer::lockProgressMutex(void) const
{
    privateData->progressMutex.lock();
}

void AutoOptimizer::unlockProgressMutex(void) const
{
    privateData->progressMutex.unlock();
}

float AutoOptimizer::totalAvailableOptions(void) const
{
    return countTotalAvailableOptions();
}

void AutoOptimizer::increaseProgressValue(qulonglong value)
{
    lockProgressMutex();
    privateData->progressValue += value;

    if (privateData->progressValue >= (privateData->lastProgressValueUpdate + std::min(privateData->totalAvailableOptions / 200, static_cast<qulonglong>(100000))) || privateData->stopProcessing)
    {
        privateData->lastProgressValueUpdate = privateData->progressValue;
        QTimer::singleShot(0, this, SLOT(updateProgressBar()));
    }
    unlockProgressMutex();
}

void AutoOptimizer::updateProgressBar(void)
{
    if (privateData->progressValue == 0)
    {
        privateData->progressTimer.restart();
        return;
    }

    double value = static_cast<double>(privateData->progressValue / privateData->progressFactor);
    double elapsed = static_cast<double>(privateData->progressTimer.elapsed()) / 1000.0;
    int left = static_cast<int>((elapsed / value) * (static_cast<double>(privateData->form.progressBar->maximum()) - value));
    QTime timeLeft(0, 0);
    QTime timeElapsed;
    QDateTime endDate = QDateTime::currentDateTime().addDays(left / 86400);
    int daysTo = QDateTime::currentDateTime().daysTo(endDate);
    left = left - (daysTo * 86400);
    if (daysTo == 0)
        privateData->form.progressBar->setFormat(QString("%p% resterende tijd " + timeLeft.addSecs(left).toString() + " verstreken tijd " + timeElapsed.addMSecs(privateData->progressTimer.elapsed()).toString()));
    else
        privateData->form.progressBar->setFormat(QString("%p% resterende tijd " + QString::number(daysTo) + " dagen " + timeLeft.addSecs(left).toString()));

    privateData->form.progressBar->setValue(static_cast<int>(value));
}

qulonglong AutoOptimizer::countTotalAvailableOptions(void) const
{
    QList<AutoOptimizerOption *> hasDependenciesList;
    QList<AutoOptimizerOption *> noDependenciesList;

    for (int i = 0; i < privateData->optionsList.size(); ++i)
    {
        AutoOptimizerOption *option = privateData->optionsList.at(i);
        const QVector<Dependency *> dependencyList = option->dependencies();
        if (dependencyList.isEmpty())
            noDependenciesList << option;
        else
        {
            if (!hasDependenciesList.contains(option))
                hasDependenciesList << option;

            Dependency *dependency;
            for (int j = 0; j < dependencyList.size(); ++j)
            {
                dependency = dependencyList.at(j);
                if (!hasDependenciesList.contains(dependency->dependencyOption))
                {
                    int index = hasDependenciesList.indexOf(option);
                    if (index < 0)
                        index = 0;
                    hasDependenciesList.insert(index, dependency->dependencyOption);
                }
            }
        }
    }

    for (int i = 0; i < hasDependenciesList.size(); ++i)
        noDependenciesList.removeAll(hasDependenciesList[ i ]);

    QList< QList<AutoOptimizerOption *> *> hasDependenciesLists;
    for (int i = 0; i < hasDependenciesList.size(); ++i)
    {
        if (!hasDependenciesList[ i ]->hasDependencies())
        {
            QList<AutoOptimizerOption *> *list = new QList<AutoOptimizerOption *>;
            hasDependenciesLists.append(list);
            list->append(hasDependenciesList[ i ]);
            hasDependenciesList.removeOne(hasDependenciesList[ i ]);
            i--;

            for (int j = 0; j < hasDependenciesList.size(); ++j)
            {
                for (int k = 0; k < list->size(); ++k)
                {
                    if (!hasDependenciesList[ j ]->dependsOn(*list->at(k)))
                        continue;

                    list->append(hasDependenciesList[ j ]);
                    hasDependenciesList.removeOne(hasDependenciesList[ j ]);
                    j--;
                    break;
                }
            }
        }
    }


    qulonglong tmp = 1;
    for (int i = 0; i < hasDependenciesLists.size(); i++)
        tmp = tmp * countTotalAvailableOptions(*hasDependenciesLists.at(i));

    qulonglong totalAvailableOptions = 0;
    for (int i = 0; i < noDependenciesList.size(); ++i)
    {
        qulonglong newTotalAvailableOptions = noDependenciesList.at(i)->possibleOptionsCount();
        if (newTotalAvailableOptions == 0)
            continue;

        if (totalAvailableOptions == 0)
            totalAvailableOptions =  newTotalAvailableOptions;
        else
            totalAvailableOptions = totalAvailableOptions * newTotalAvailableOptions;
    }

    if (tmp == 0)
        return totalAvailableOptions;
    else if (totalAvailableOptions == 0)
        return tmp;

    return totalAvailableOptions * tmp;
}

qulonglong AutoOptimizer::countTotalAvailableOptions(QList<AutoOptimizerOption *> &list) const
{
    const QList<QVariant> settings = saveSettings();
    setOptionsToMinimumValue(list);

    qulonglong options = 0;
    int depth = 0;
    bool done = false;
    while (done == false)
    {
        bool skip = false;
        QList<AutoOptimizerOption *>::const_iterator optionsIt = list.constBegin();
        if (depth > 0)
            optionsIt = optionsIt + depth;
        const QList<AutoOptimizerOption *>::const_iterator optionsEnd = list.constEnd();
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

        if (!skip)
            options++;

        depth = list.size() - 1;
        optionsIt = list.constEnd();
        const QList<AutoOptimizerOption *>::const_iterator optionsBegin = list.constBegin();
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

            float min = option->min().toFloat();
            float max = option->max().toFloat();
            float stepSize = option->stepSize().toFloat();
            float accelleration;
            if (option->useAcceleration())
                accelleration = 1.1f;
            else
                accelleration = 1.0f;

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

                option->setValue(min, 5);
            }
        }
    }

    loadSettings(settings);

    return options;
}

const QList<QVariant> AutoOptimizer::saveSettings(void) const
{
    QList<QVariant> list;
    for (int i = 0; i < privateData->optionsList.size(); ++i)
    {
        AutoOptimizerOption *option = privateData->optionsList.at(i);
        list << option->value();
    }

    return list;
}

void AutoOptimizer::loadSettings(const QList<QVariant> &settings) const
{
    for (int i = 0; i < privateData->optionsList.size(); ++i)
    {
        AutoOptimizerOption *option = privateData->optionsList.at(i);
        if (i < settings.size())
            option->setValue(settings.at(i));
    }
}

void AutoOptimizer::lockTradeResultsTable(bool write) const
{
    if (write)
        privateData->tradeResultsTableLock.lockForWrite();
    else
        privateData->tradeResultsTableLock.lockForRead();
}

void AutoOptimizer::unlockTradeResultsTable(void) const
{
    privateData->tradeResultsTableLock.unlock();
}

SessionInfo &AutoOptimizer::sessionInfo(void) const
{
    return privateData->sessionInfo;
}

TradeResults AutoOptimizer::tradeResultsItemAt(int i) const
{
    if (i < 0 || i >= privateData->tradeResultsList.size())
        return TradeResults();

    lockTradeResultsTable();
    const TradeResults &results = privateData->tradeResultsList.at(i);
    unlockTradeResultsTable();

    return results;
}

int AutoOptimizer::tradeResultsCount(void) const
{
    lockTradeResultsTable();
    int count = privateData->tradeResultsList.size();
    unlockTradeResultsTable();

    return count;
}

void AutoOptimizer::addResult(const TradeResults &tradeResults)
{
    lockTradeResultsTable(true);

    if (!showDoubleResults())
    {
        for (int i = 0; i < privateData->tradeResultsList.size(); i++)
        {
            const TradeResults &item = privateData->tradeResultsList.at(i);
            if (qFuzzyCompare(item.relativeProfits, tradeResults.relativeProfits))
            {
                float succesRateToTotalProfitTradesRate = item.successRate * item.totalProfitTrades;
                float newSuccesRateToTotalProfitTradesRate = tradeResults.successRate * tradeResults.totalProfitTrades;
                if (newSuccesRateToTotalProfitTradesRate < succesRateToTotalProfitTradesRate ||
                    qFuzzyCompare(newSuccesRateToTotalProfitTradesRate, succesRateToTotalProfitTradesRate))
                {
                    unlockTradeResultsTable();
                    return;
                }

                privateData->tradeResultsList.remove(i);
            }
        }
    }

    int index = privateData->tradeResultsList.size();
    for (int i = 0; i < privateData->tradeResultsList.size(); i++)
    {
        if (tradeResults.relativeProfits > privateData->tradeResultsList.at(i).relativeProfits)
        {
            index = i;
            break;
        }
    }
    privateData->tradeResultsList.insert(index, tradeResults);

    if (privateData->tradeResultsList.size() > maximumResultsVisible())
        privateData->tradeResultsList.remove(privateData->tradeResultsList.size() - 1);

    for (int i = 0; i < privateData->workers.size(); i++)
    {
        AutoOptimizerWorker *worker = privateData->workers.at(i);
        if (worker)
            worker->setNewTradeResultsAvailable(true);
    }

    unlockTradeResultsTable();

    if (!privateData->updateResultsWidgetTimer.isActive())
        privateData->updateResultsWidgetTimer.start();
}

void AutoOptimizer::clearResults(void)
{
    clearResultsWidget();
    privateData->tradeResultsList.clear();
}

void AutoOptimizer::clearResultsWidget(void)
{
    QTableWidgetItem *item;

    for (int i = 0; i < privateData->form.resultsTableWidget->rowCount(); ++i)
    {
        for (int j = 0; j < privateData->form.resultsTableWidget->columnCount(); ++j)
        {
            item = privateData->form.resultsTableWidget->takeItem(i, j);
            if (item)
                delete item;
        }
    }

    privateData->form.resultsTableWidget->setRowCount(0);
}

void AutoOptimizer::fillResultsWidget(void)
{
    qDebug() << "AutoOptimizer::fillResultsWidget()";

    bool updates = privateData->form.resultsTableWidget->updatesEnabled();
    privateData->form.resultsTableWidget->setUpdatesEnabled(false);

    clearResultsWidget();

    for (int i = 0; i < privateData->tradeResultsList.size(); i++)
    {
        TradeResults tradeResults = privateData->tradeResultsList.at(i);
        QString toolTip = QString(tr("Winst") + ":\t" + QString::number(tradeResults.relativeProfits) + " %\n" + tr("Transacties") + ":\t" + QString::number(tradeResults.totalTransactions) + "\n" + tr("Verlies trades") + ":\t" + QString::number(tradeResults.totalLossTrades) + "\n" + tr("Succes Ratio") + ":\t" + QString::number(tradeResults.successRate) + "\n\n" + tr("Configuratie") + ":\n" + QString(tradeResults.configuration).replace(";", "\n"));

        int rowCount = privateData->form.resultsTableWidget->rowCount() + 1;
        privateData->form.resultsTableWidget->setRowCount(rowCount);

        TableWidgetItem *profitItem = new TableWidgetItem;
        profitItem->results = tradeResults;
        profitItem->setText(QString::number(tradeResults.relativeProfits).append(" %"));
        profitItem->setToolTip(toolTip);
        profitItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        privateData->form.resultsTableWidget->setItem(rowCount - 1, 0, profitItem);

        TableWidgetItem *totalTransactionsItem = new TableWidgetItem;
        totalTransactionsItem->results = tradeResults;
        totalTransactionsItem->setText(QString::number(tradeResults.totalTransactions));
        totalTransactionsItem->setToolTip(toolTip);
        totalTransactionsItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        privateData->form.resultsTableWidget->setItem(rowCount - 1, 1, totalTransactionsItem);

        TableWidgetItem *totalTradesItem = new TableWidgetItem;
        totalTradesItem->results = tradeResults;
        totalTradesItem->setText(QString::number(tradeResults.totalTrades));
        totalTradesItem->setToolTip(toolTip);
        totalTradesItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        privateData->form.resultsTableWidget->setItem(rowCount - 1, 2, totalTradesItem);

        TableWidgetItem *totalProfitTradesItem = new TableWidgetItem;
        totalProfitTradesItem->results = tradeResults;
        totalProfitTradesItem->setText(QString::number(tradeResults.totalProfitTrades));
        totalProfitTradesItem->setToolTip(toolTip);
        totalProfitTradesItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        privateData->form.resultsTableWidget->setItem(rowCount - 1, 3, totalProfitTradesItem);

        TableWidgetItem *totalLossTradesItem = new TableWidgetItem;
        totalLossTradesItem->results = tradeResults;
        totalLossTradesItem->setText(QString::number(tradeResults.totalLossTrades));
        totalLossTradesItem->setToolTip(toolTip);
        totalLossTradesItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        privateData->form.resultsTableWidget->setItem(rowCount - 1, 4, totalLossTradesItem);

        TableWidgetItem *successRateItem = new TableWidgetItem;
        successRateItem->results = tradeResults;
        successRateItem->setText(QString::number(tradeResults.successRate).append(" %"));
        successRateItem->setToolTip(toolTip);
        successRateItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        privateData->form.resultsTableWidget->setItem(rowCount - 1, 5, successRateItem);

        TableWidgetItem *configItem = new TableWidgetItem;
        configItem->results = tradeResults;
        configItem->setText(tradeResults.configuration);
        configItem->setToolTip(toolTip);
        configItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        privateData->form.resultsTableWidget->setItem(rowCount - 1, 6, configItem);
    }

    privateData->form.resultsTableWidget->resizeColumnsToContents();
    privateData->form.resultsTableWidget->setUpdatesEnabled(updates);
}

void AutoOptimizer::setOptionsToMinimumValue(const QList< AutoOptimizerOption * > &list) const
{
    int size = list.size();
    for (int i = 0; i < size; ++i)
    {
        AutoOptimizerOption *option = list.at(i);
        float min = option->min().toFloat();
        if (option->hasGuiItemsEnabled())
            option->setValue(min);
    }
}

