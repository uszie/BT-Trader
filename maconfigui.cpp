#include "indicator.h"
#include "ui_ma.h"

const char *movingAverageTitleList[ 11 ] = { "Simple", "Exponential", "LinearWeighted", "Adaptive", "Hull", "T3", "GDEMA", "ZLEMA", "MAMA", "FAMA", NULL };

QDataStream &operator<<(QDataStream &stream, const MovingAverageOptions &options)
{
    stream << options.type << options.periods << options.fastSmoothing << options.slowSmoothing << options.minCyclePeriods << options.maxCyclePeriods << options.amplificationFactor << options.cycleAdativeEnabled << options.trendAdaptiveEnabled << options.useHalfCycle;

    return stream;
}


QDataStream &operator>>(QDataStream &stream, MovingAverageOptions &options)
{
    stream >> options.type >> options.periods >> options.fastSmoothing >> options.slowSmoothing >> options.minCyclePeriods >> options.maxCyclePeriods >> options.amplificationFactor >> options.cycleAdativeEnabled >> options.trendAdaptiveEnabled >> options.useHalfCycle;

    return stream;
}

void  MovingAverageOptions::registerWithAutoOptimizer(Indicator *indicator, const QString &title, bool cycleAdativeEnabled, int aoMinPE, int aoMaxPE, int aoMinMA, int aoMaxMA, int aoMinFS, int aoMaxFS, int aoMinSS, int aoMaxSS/*, int aoMinMinCP, int aoMaxMinCP, int aoMinMaxCP, int aoMaxMaxCP*/)
{
    PeriodOptions::registerWithAutoOptimizer(indicator, title, cycleAdativeEnabled, aoMinPE, aoMaxPE);

    QStringList maTypeTitles;

    for (int i = 0; movingAverageTitleList[ i ]; ++i)
        maTypeTitles << movingAverageTitleList[ i ];

    AutoOptimizerOption *maTypeOption = indicator->addAutoOptimizerOption(QString(title + indicator->tr(" MA type")), &type, 0, 4, 1, false, AutoOptimizerOption::IndicatorOption, maTypeTitles);
    maTypeOption->setMin(aoMinMA);
    if (!debugEnabled)
        maTypeOption->setMax(aoMaxMA);
    else
        maTypeOption->setMax(0);

    AutoOptimizerOption *fastSmoothingOption = indicator->addAutoOptimizerOption(QString(title + indicator->tr(" fast smoothing")), &fastSmoothing, 2, 2, 1, true);
    fastSmoothingOption->setMin(aoMinFS);
    fastSmoothingOption->setMax(aoMaxFS);
    fastSmoothingOption->setSuffix(indicator->tr(" perioden"));
    fastSmoothingOption->addDependency(maTypeOption, Adaptive);

    AutoOptimizerOption *slowSmoothingOption = indicator->addAutoOptimizerOption(QString(title + indicator->tr(" slow smoothing")), &slowSmoothing, 30, 30, 1, true);
    slowSmoothingOption->setMin(aoMinSS);
    slowSmoothingOption->setMax(aoMaxSS);
    slowSmoothingOption->setSuffix(indicator->tr(" perioden"));
    slowSmoothingOption->addDependency(maTypeOption, Adaptive);
    slowSmoothingOption->addDependency(fastSmoothingOption, Dependency::IsLower);
}

QString MovingAverageOptions::createConfigKey(void) const
{
    return QString(PeriodOptions::createConfigKey() + ",MovingAverageType=" + QString::number(type) + ",FastSmoothing=" + QString::number(fastSmoothing) + ",SlowSmoothing=" + QString::number(slowSmoothing));
}

class MovingAverageUI::PrivateData
{
    public:
        QStringList maTypeList;
        Ui::MAUI form;
};

MovingAverageUI::MovingAverageUI(QWidget *parent, const QString &title, Qt::WindowFlags f) :
    PeriodsUI(parent, title, f)
{
    privateData = new PrivateData;

    QWidget tmp;
    tmp.setLayout(layout());
    delete layout();

    privateData->form.setupUi(this);
    QFormLayout *peFormLayout = qobject_cast<QFormLayout *>(tmp.layout());
    QFormLayout *maFormLayout = qobject_cast<QFormLayout *>(layout());

    for (int i = 0; i < peFormLayout->rowCount(); i++)
    {
        if (peFormLayout->itemAt(i, QFormLayout::SpanningRole))
            maFormLayout->insertRow(600, peFormLayout->itemAt(i, QFormLayout::SpanningRole)->widget());
        else if (peFormLayout->itemAt(i, QFormLayout::LabelRole) && peFormLayout->itemAt(i, QFormLayout::FieldRole))
            maFormLayout->insertRow(600, peFormLayout->itemAt(i, QFormLayout::LabelRole)->widget() , peFormLayout->itemAt(i, QFormLayout::FieldRole)->widget());
        else if (peFormLayout->itemAt(i, QFormLayout::FieldRole))
            maFormLayout->insertRow(600, "", peFormLayout->itemAt(i, QFormLayout::FieldRole)->widget());
    }

    privateData->form.titleLabel->setText(QString("<b>" + title + "</b>"));
    privateData->form.maComboBox->setCurrentIndex(0);
    privateData->maTypeList << tr("Simple") << tr("Exponential") << tr("Linear Weighted") << tr("Adaptive") << tr("Hull") /*<< tr( "Jurik")*/ << tr("T3") << tr("GDEMA") << tr("Zero Lag EMA") << tr("Mesa AMA") << tr("Mesa Following AMA");

    for (int i = 0; i < privateData->maTypeList.size(); ++i)
    {
        privateData->form.maComboBox->addItem(privateData->maTypeList.at(i));
    }

    setUseHalfCycle(true);
    setMaType(Simple);
    setSmoothingBoxesEnabled(false);
    setFastSmoothing(2);
    setSlowSmoothing(30);

    connect(privateData->form.maComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotPropertiesChanged()));
    connect(privateData->form.fsSpinBox, SIGNAL(valueChanged(int)), this, SLOT(slotPropertiesChanged()));
    connect(privateData->form.ssSpinBox, SIGNAL(valueChanged(int)), this, SLOT(slotPropertiesChanged()));
    connect(this, SIGNAL(overloadedPropertiesChanged()), this, SLOT(slotOverloadedPropertiesChanged()));
}

MovingAverageUI::~MovingAverageUI()
{

}

int MovingAverageUI::maType(void)
{
    return privateData->form.maComboBox->currentIndex();
}

void MovingAverageUI::setMaType(int type)
{
    if (type < 0 || type > (privateData->maTypeList.size() - 1))
        type = 0;

    privateData->form.maComboBox->setCurrentIndex(type);
}

int MovingAverageUI::slowSmoothing(void)
{
    return privateData->form.ssSpinBox->value();
}

void MovingAverageUI::setSlowSmoothing(int slowSmoothing)
{
    privateData->form.ssSpinBox->setValue(slowSmoothing);
}


int MovingAverageUI::fastSmoothing(void)
{
    return privateData->form.fsSpinBox->value();
}

void MovingAverageUI::setFastSmoothing(int fastSmoothing)
{
    privateData->form.fsSpinBox->setValue(fastSmoothing);
}


void MovingAverageUI::setSmoothingBoxesEnabled(bool enabled)
{
    if (enabled)
    {
        privateData->form.fsLabel->show();
        privateData->form.fsSpinBox->show();
        privateData->form.ssLabel->show();
        privateData->form.ssSpinBox->show();
    }
    else
    {
        privateData->form.fsLabel->hide();
        privateData->form.fsSpinBox->hide();
        privateData->form.ssLabel->hide();
        privateData->form.ssSpinBox->hide();
    }
}

QString MovingAverageUI::maTitle(bool shortTitle)
{
    if (shortTitle)
    {
        if (privateData->form.maComboBox->currentIndex() < 0 || privateData->form.maComboBox->currentIndex() > (privateData->maTypeList.size() - 1))
            return QString(privateData->maTypeList.at(0).at(0).toUpper() + tr("MA ") + QString::number(periods()));

        return QString(privateData->maTypeList.at(privateData->form.maComboBox->currentIndex()).at(0).toUpper() + tr("MA ") + QString::number(periods()));
    }

    if (privateData->form.maComboBox->currentIndex() < 0 || privateData->form.maComboBox->currentIndex() > (privateData->maTypeList.size() - 1))
        return QString(privateData->maTypeList.at(0) + tr(" Moving Average ") + QString::number(periods()));

    return QString(privateData->maTypeList.at(privateData->form.maComboBox->currentIndex()) + tr(" Moving Average ") + QString::number(periods()));
}


void MovingAverageUI::options(MovingAverageOptions &options)
{
    PeriodsUI::options(options);
    options.type = maType();
    options.fastSmoothing = fastSmoothing();
    options.slowSmoothing = slowSmoothing();
}

void MovingAverageUI::setOptions(MovingAverageOptions &options)
{
    bool block = signalsBlocked();
    blockSignals(true);

    PeriodsUI::setOptions(options);
    setMaType(options.type);
    setFastSmoothing(options.fastSmoothing);
    setSlowSmoothing(options.slowSmoothing);

    if (maType() == Simple || maType() == Exponential || maType() == LinearWeighted || maType() == Adaptive)
        showCycleAdaptiveButton(true);
    else
        showCycleAdaptiveButton(false);

    blockSignals(block);

    emit propertiesChanged();
}

void MovingAverageUI::slotPropertiesChanged(void)
{
    if (maType() == Adaptive || maType() == MAMA || maType() == FAMA)
    {
        setSmoothingBoxesEnabled(true);
    }
    else
    {
        setSmoothingBoxesEnabled(false);
    }

    if (maType() == MAMA || maType() == FAMA)
        setPeriodsSpinboxEnabled(false);
    else
        setPeriodsSpinboxEnabled(true);

    if (maType() == Simple || maType() == Exponential || maType() == LinearWeighted || maType() == Adaptive)
        showCycleAdaptiveButton(true);
    else
        showCycleAdaptiveButton(false);

    emit propertiesChanged();
}

void MovingAverageUI::slotOverloadedPropertiesChanged(void)
{
    slotPropertiesChanged();
}
