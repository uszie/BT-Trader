#include "indicator.h"
#include "ui_periods.h"

QDataStream &operator<<(QDataStream &stream, const PeriodOptions &options)
{
    stream << options.periods << options.fastSmoothing << options.slowSmoothing << options.minCyclePeriods << options.maxCyclePeriods << options.amplificationFactor << options.cycleAdativeEnabled << options.trendAdaptiveEnabled << options.useHalfCycle;

    return stream;
}


QDataStream &operator>>(QDataStream &stream, PeriodOptions &options)
{
    stream >> options.periods >> options.fastSmoothing >> options.slowSmoothing >> options.minCyclePeriods >> options.maxCyclePeriods >> options.amplificationFactor >> options.cycleAdativeEnabled >> options.trendAdaptiveEnabled >> options.useHalfCycle;

    return stream;
}

void PeriodOptions::registerWithAutoOptimizer(Indicator *indicator, const QString &title, bool cycleAdative, int aoMinPE, int aoMaxPE)
{
    AutoOptimizerOption *periodsOption = indicator->addAutoOptimizerOption(QString(title + indicator->tr(" perioden")), &periods, 1, 100, 1, true);
    periodsOption->setMin(aoMinPE);
    if (!debugEnabled)
        periodsOption->setMax(aoMaxPE);
    else
        periodsOption->setMax(2);

    if (cycleAdative == false)
        return;

    if (!debugEnabled)
    {
        AutoOptimizerOption *cycleAdativeEnabledOption = indicator->addAutoOptimizerOption(QString(title + indicator->tr(" perioden instellen op de cyclus lengte")), &cycleAdativeEnabled, 0, 1);
        AutoOptimizerOption *trendAdaptiveEnabledOption = indicator->addAutoOptimizerOption(QString(title + indicator->tr(" ook bij aanwezigheid van een trend")), &trendAdaptiveEnabled, 0, 0);
        trendAdaptiveEnabledOption->addDependency(cycleAdativeEnabledOption, true);
        AutoOptimizerOption *minCyclePeriodsOption;
        AutoOptimizerOption *maxCyclePeriodsOption;
        if (useHalfCycle)
        {
            minCyclePeriodsOption = indicator->addAutoOptimizerOption(QString(title + indicator->tr(" minimum perioden")), &minCyclePeriods, 3, 3, 1, true);
            maxCyclePeriodsOption = indicator->addAutoOptimizerOption(QString(title + indicator->tr(" maximum perioden")), &maxCyclePeriods, 15, 15, 1, true);
        }
        else
        {
            minCyclePeriodsOption = indicator->addAutoOptimizerOption(QString(title + indicator->tr(" minimum perioden")), &minCyclePeriods, 6, 6, 1, true);
            maxCyclePeriodsOption = indicator->addAutoOptimizerOption(QString(title + indicator->tr(" maximum perioden")), &maxCyclePeriods, 30, 30, 1, true);
        }

        minCyclePeriodsOption->addDependency(cycleAdativeEnabledOption, true);
        maxCyclePeriodsOption->addDependency(cycleAdativeEnabledOption, true);
        maxCyclePeriodsOption->addDependency(minCyclePeriodsOption, Dependency::IsLower);
    }
};

QString PeriodOptions::createConfigKey(void) const
{
    return QString("Periods=" + QString::number(periods) + ",CycleAdaptiveEnable=" + QString::number(cycleAdativeEnabled) + ",TrendAdaptiveEnabled=" + QString::number(trendAdaptiveEnabled) + ",MinCyclePeriods=" + QString::number(minCyclePeriods) + ",MaxCyclePeriods=" + QString::number(maxCyclePeriods));
}

class PeriodsUI::PrivateData
{
    public:

        bool useHalfCycle;
        bool cycleAdaptiveButtonEnabled;
        Ui::PeriodsUI form;
};

PeriodsUI::PeriodsUI(QWidget *parent, const QString &title, Qt::WindowFlags f):
    QWidget(parent, f)
{
    (void)title;

    privateData = new PrivateData;
    privateData->useHalfCycle = false;
    privateData->cycleAdaptiveButtonEnabled = true;
    privateData->form.setupUi(this);
    privateData->form.peSpinBox->setValue(20);
    privateData->form.cadCheckBox->setChecked(false);
    privateData->form.taCheckBox->setChecked(false);
    privateData->form.micpSpinBox->setValue(6);
    privateData->form.micpSpinBox->setMinimum(6);
    privateData->form.micpSpinBox->setMaximum(30);
    privateData->form.macpSpinBox->setValue(30);
    privateData->form.macpSpinBox->setMinimum(6);
    privateData->form.macpSpinBox->setMaximum(30);
    privateData->form.caSpinBox->setValue(1.0);

    setCycleAdaptive(false);
    setPeriodsSpinboxEnabled(true);

    connect(privateData->form.peSpinBox, SIGNAL(valueChanged(int)), this, SLOT(slotPropertiesChanged()));
    connect(privateData->form.cadCheckBox, SIGNAL(clicked(bool)), this, SLOT(slotPropertiesChanged()));
    connect(privateData->form.taCheckBox, SIGNAL(clicked(bool)), this, SLOT(slotPropertiesChanged()));
    connect(privateData->form.micpSpinBox, SIGNAL(valueChanged(int)), this, SLOT(slotPropertiesChanged()));
    connect(privateData->form.macpSpinBox, SIGNAL(valueChanged(int)), this, SLOT(slotPropertiesChanged()));
    connect(privateData->form.caSpinBox, SIGNAL(valueChanged(int)), this, SLOT(slotPropertiesChanged()));
}

PeriodsUI::~PeriodsUI()
{

}

int PeriodsUI::periods(void)
{
    return privateData->form.peSpinBox->value();
}

void PeriodsUI::setPeriods(int periods)
{
    privateData->form.peSpinBox->setValue(periods);
}

bool PeriodsUI::cycleAdative(void)
{
    return privateData->form.cadCheckBox->isChecked();
}

void PeriodsUI::setCycleAdaptive(bool enabled)
{
    privateData->form.cadCheckBox->setChecked(enabled);
    if (enabled)
        privateData->form.cadWidget->setVisible(true);
    else
        privateData->form.cadWidget->setVisible(false);
}

int PeriodsUI::minCycleAdaptive(void)
{
    return privateData->form.micpSpinBox->value();
}

void PeriodsUI::setMinCycleAdaptive(int periods)
{
    privateData->form.micpSpinBox->setValue(periods);
}

int PeriodsUI::maxCycleAdaptive(void)
{
    return privateData->form.macpSpinBox->value();
}

void PeriodsUI::setMaxCycleAdaptive(int periods)
{
    privateData->form.macpSpinBox->setValue(periods);
}

bool PeriodsUI::adaptiveInTrend(void)
{
    return privateData->form.taCheckBox->isChecked();
}

void PeriodsUI::setAdaptiveInTrend(bool enabled)
{
    privateData->form.taCheckBox->setChecked(enabled);
}

double PeriodsUI::amplificationFactor(void)
{
    return privateData->form.caSpinBox->value();
}

void PeriodsUI::setAmplificationFactor(double amplificationFactor)
{
    privateData->form.caSpinBox->setValue(amplificationFactor);
}

void PeriodsUI::setCycleAdaptiveButtonEnabled(bool enabled)
{
    if (enabled == false)
    {
        privateData->cycleAdaptiveButtonEnabled = false;
        privateData->form.cadLabel->hide();
        privateData->form.cadCheckBox->hide();
    }
    else
    {
        privateData->cycleAdaptiveButtonEnabled = true;
        privateData->form.cadLabel->show();
        privateData->form.cadCheckBox->show();
    }
}

void PeriodsUI::setPeriodsSpinboxEnabled(bool enabled)
{
    if (enabled)
    {
        privateData->form.peLabel->show();
        privateData->form.peSpinBox->show();
    }
    else
    {
        privateData->form.peLabel->hide();
        privateData->form.peSpinBox->hide();
    }
}

void PeriodsUI::showCycleAdaptiveButton(bool show)
{
    if (show == false)
    {
        privateData->form.cadLabel->hide();
        privateData->form.cadCheckBox->hide();
    }
    else if (privateData->cycleAdaptiveButtonEnabled)
    {
        privateData->form.cadLabel->show();
        privateData->form.cadCheckBox->show();
    }
}

void PeriodsUI::setUseHalfCycle(bool enabled)
{
    privateData->useHalfCycle = enabled;
    if (enabled)
    {
        privateData->form.micpSpinBox->setMinimum(3);
        privateData->form.macpSpinBox->setMaximum(15);
        privateData->form.micpSpinBox->setMaximum(privateData->form.macpSpinBox->value());
        privateData->form.macpSpinBox->setMinimum(privateData->form.micpSpinBox->value());
    }
    else
    {
        privateData->form.micpSpinBox->setMinimum(6);
        privateData->form.macpSpinBox->setMaximum(30);
        privateData->form.micpSpinBox->setMaximum(privateData->form.macpSpinBox->value());
        privateData->form.macpSpinBox->setMinimum(privateData->form.micpSpinBox->value());
    }
}

void PeriodsUI::options(PeriodOptions &options)
{
    options.periods = privateData->form.peSpinBox->value();
    options.minCyclePeriods = privateData->form.micpSpinBox->value();
    options.maxCyclePeriods = privateData->form.macpSpinBox->value();
    options.amplificationFactor = privateData->form.caSpinBox->value();
    options.cycleAdativeEnabled = privateData->form.cadCheckBox->isChecked();
    options.trendAdaptiveEnabled = privateData->form.taCheckBox->isChecked();
}

void PeriodsUI::setOptions(PeriodOptions &options)
{
    bool block = signalsBlocked();
    blockSignals(true);

    setPeriods(options.periods);
    setMinCycleAdaptive(options.minCyclePeriods);
    setMaxCycleAdaptive(options.maxCyclePeriods);
    setAmplificationFactor(options.amplificationFactor);
    setCycleAdaptive(options.cycleAdativeEnabled);
    setAdaptiveInTrend(options.trendAdaptiveEnabled);

    blockSignals(block);

    emit propertiesChanged();
}

void PeriodsUI::slotPropertiesChanged(void)
{
    privateData->form.micpSpinBox->setMaximum(privateData->form.macpSpinBox->value());
    privateData->form.macpSpinBox->setMinimum(privateData->form.micpSpinBox->value());

    if (privateData->form.cadCheckBox->isChecked())
        privateData->form.cadWidget->setVisible(true);
    else
        privateData->form.cadWidget->setVisible(false);

    emit propertiesChanged();
    emit overloadedPropertiesChanged();
}
