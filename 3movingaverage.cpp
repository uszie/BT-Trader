#include "3movingaverage.h"

ThreeMovingAverageCurve::ThreeMovingAverageCurve(Plot *plot, PriceChart *priceChart, bool setupDialog, const QString &configGroup, Indicator::ProcessMode processMode) :
    Indicator(plot, priceChart, setupDialog, configGroup, processMode)
{
    fastCurve = new PlotCurve("snel", indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    curveList.append(fastCurve);

    mediumCurve = new PlotCurve("medium", indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    curveList.append(mediumCurve);

    slowCurve = new PlotCurve("langzaam", indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    curveList.append(slowCurve);

    fastOptions.registerWithAutoOptimizer(this, tr("fast:"), true, 1, 40, 0, 4, 2, 2, 30, 30);
    mediumOptions.registerWithAutoOptimizer(this, tr("medium:"), true, 30, 70, 0, 4, 2, 2, 30, 30);
    slowOptions.registerWithAutoOptimizer(this, tr("slow:"), true, 60, 100, 0, 4, 2, 2, 30, 30);
}

ThreeMovingAverageCurve::~ThreeMovingAverageCurve(void)
{
}

void ThreeMovingAverageCurve::readSettingsPrivate(void)
{
    Indicator::readSettingsPrivate();

    fastOptions = settings.value("FastMovingAverageOptions", qVariantFromValue(MovingAverageOptions::defaultOptions(5))).value<MovingAverageOptions>();
    mediumOptions = settings.value("MediumMovingAverageOptions", qVariantFromValue(MovingAverageOptions::defaultOptions(20))).value<MovingAverageOptions>();
    slowOptions = settings.value("SlowMovingAverageOptions", qVariantFromValue(MovingAverageOptions::defaultOptions(60))).value<MovingAverageOptions>();
}

void ThreeMovingAverageCurve::writeSettingsPrivate(void)
{
    Indicator::writeSettingsPrivate();

    settings.setValue("Type", ThreeMovingAveragesIndicator);
    settings.setValue("FastMovingAverageOptions", qVariantFromValue(fastOptions));
    settings.setValue("MediumMovingAverageOptions", qVariantFromValue(mediumOptions));
    settings.setValue("SlowMovingAverageOptions", qVariantFromValue(slowOptions));
}

void ThreeMovingAverageCurve::loadDefaultRules(void)
{
    clearRuleLists();

    ruleList(Long)->addRule("SourceIndicator=this;SourceCurve=" + fastCurve->title().text() + ";ReferenceIndicator=this;ReferenceCurve=" + mediumCurve->title().text() + ";ReferenceValue=0;Offset=0;CompareType=Kruist omhoog;LogicalOperator=En;PreviousTransactionType=Exit;Enabled=1");
    ruleList(Long)->addRule("SourceIndicator=this;SourceCurve=" + mediumCurve->title().text() + ";ReferenceIndicator=this;ReferenceCurve=" + slowCurve->title().text() + ";ReferenceValue=0;Offset=0;CompareType=Hoger dan;LogicalOperator=En;PreviousTransactionType=Exit;Enabled=1");
    ruleList(Short)->addRule("SourceIndicator=this;SourceCurve=" + fastCurve->title().text() + ";ReferenceIndicator=this;ReferenceCurve=" + mediumCurve->title().text() + ";ReferenceValue=0;Offset=0;CompareType=Kruist omlaag;LogicalOperator=En;PreviousTransactionType=Exit;Enabled=1");
    ruleList(Short)->addRule("SourceIndicator=this;SourceCurve=" + mediumCurve->title().text() + ";ReferenceIndicator=this;ReferenceCurve=" + slowCurve->title().text() + ";ReferenceValue=0;Offset=0;CompareType=Lager dan;LogicalOperator=En;PreviousTransactionType=Exit;Enabled=1");
    ruleList(Exit)->addRule("SourceIndicator=this;SourceCurve=" + fastCurve->title().text() + ";ReferenceIndicator=this;ReferenceCurve=" + mediumCurve->title().text() + ";ReferenceValue=0;Offset=0;CompareType=Kruist omhoog;LogicalOperator=Of;PreviousTransactionType=Short;Enabled=1");
    ruleList(Exit)->addRule("SourceIndicator=this;SourceCurve=" + fastCurve->title().text() + ";ReferenceIndicator=this;ReferenceCurve=" + mediumCurve->title().text() + ";ReferenceValue=0;Offset=0;CompareType=Kruist omlaag;LogicalOperator=Of;PreviousTransactionType=Long;Enabled=1");

    Indicator::loadDefaultRules();
}

void ThreeMovingAverageCurve::plotCurve(void)
{
    if (fastOptions.type == Simple || fastOptions.type == Exponential || fastOptions.type == LinearWeighted || fastOptions.type == Adaptive)
        calculateMACurve(*indicatorSourceCurve, *fastCurve, fastOptions);
    else
        calculateMACurve(*indicatorSourceCurve, *fastCurve, fastOptions.periods, fastOptions.type, -1, fastOptions.slowSmoothing, fastOptions.fastSmoothing);

    if (mediumOptions.type == Simple || mediumOptions.type == Exponential || mediumOptions.type == LinearWeighted || mediumOptions.type == Adaptive)
        calculateMACurve(*indicatorSourceCurve, *mediumCurve, mediumOptions);
    else
        calculateMACurve(*indicatorSourceCurve, *mediumCurve, mediumOptions.periods, mediumOptions.type, -1, mediumOptions.slowSmoothing, mediumOptions.fastSmoothing);

    if (slowOptions.type == Simple || slowOptions.type == Exponential || slowOptions.type == LinearWeighted || slowOptions.type == Adaptive)
        calculateMACurve(*indicatorSourceCurve, *slowCurve, slowOptions);
    else
        calculateMACurve(*indicatorSourceCurve, *slowCurve, slowOptions.periods, slowOptions.type, -1, slowOptions.slowSmoothing, slowOptions.fastSmoothing);

    Indicator::plotCurve();
}

void ThreeMovingAverageCurve::updateGUI(void)
{
    bool block = blockSettingsGUISignals();
    setBlockSettingsGUISignals(true);

    fastForm->setOptions(fastOptions);
    mediumForm->setOptions(mediumOptions);
    slowForm->setOptions(slowOptions);

    setBlockSettingsGUISignals(block);

    Indicator::updateGUI();
}

QWidget *ThreeMovingAverageCurve::customIndicatorWidget(void)
{
    fastForm = new MovingAverageUI(NULL, "Fast");
    fastForm->setOptions(fastOptions);
    fastForm->setUseHalfCycle(true);
    connect(fastForm, SIGNAL(propertiesChanged()), this, SLOT(setProperties()));
    fastCurve->setTitle(QString(tr("Fast ") + fastForm->maTitle()));

    mediumForm = new MovingAverageUI(NULL, "Medium");
    mediumForm->setOptions(mediumOptions);
    mediumForm->setUseHalfCycle(true);
    connect(mediumForm, SIGNAL(propertiesChanged()), this, SLOT(setProperties()));
    mediumCurve->setTitle(QString(tr("Medium ") + mediumForm->maTitle()));

    slowForm = new MovingAverageUI(NULL, "Slow");
    slowForm->setOptions(slowOptions);
    slowForm->setUseHalfCycle(true);
    connect(slowForm, SIGNAL(propertiesChanged()), this, SLOT(setProperties()));
    slowCurve->setTitle(QString(tr("Slow ") + slowForm->maTitle()));

    QWidget *ciWidget = new QWidget;
    ciWidget->setLayout(new QVBoxLayout);
    ciWidget->layout()->addWidget(fastForm);
    ciWidget->layout()->addWidget(mediumForm);
    ciWidget->layout()->addWidget(slowForm);

    setTitle(QString(tr("3 Moving Averages ") + QString::number(fastOptions.periods) + "," + QString::number(mediumOptions.periods) + "," + QString::number(slowOptions.periods)));

    return ciWidget;
}

void ThreeMovingAverageCurve::setProperties(void)
{
    if (blockSettingsGUISignals())
        return;

    fastForm->options(fastOptions);
    fastCurve->setTitle(QString(tr("Fast ") + fastForm->maTitle()));

    mediumForm->options(mediumOptions);
    mediumCurve->setTitle(QString(tr("Medium ") + mediumForm->maTitle()));

    slowForm->options(slowOptions);
    slowCurve->setTitle(QString(tr("Slow ") + slowForm->maTitle()));

    setTitle(QString(tr("3 Moving Averages ") + QString::number(fastOptions.periods) + "," + QString::number(mediumOptions.periods) + "," + QString::number(slowOptions.periods)));

    Indicator::setProperties();
}
