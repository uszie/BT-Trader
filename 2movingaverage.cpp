#include "2movingaverage.h"

TwoMovingAverageCurve::TwoMovingAverageCurve(Plot *plot, PriceChart *priceChart, bool setupDialog, const QString &configGroup, Indicator::ProcessMode processMode) :
    Indicator(plot, priceChart, setupDialog, configGroup, processMode)
{
    fastCurve = new PlotCurve("snel", indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    curveList.append(fastCurve);

    mediumCurve = new PlotCurve("medium", indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    curveList.append(mediumCurve);

    fastOptions.registerWithAutoOptimizer(this, tr("fast:"), true, 1, 50, 0, 4, 2, 2, 30, 30);
    mediumOptions.registerWithAutoOptimizer(this, tr("medium:"), true, 40, 100, 0, 4, 2, 2, 30, 30);
}

TwoMovingAverageCurve::~TwoMovingAverageCurve(void)
{
}

void TwoMovingAverageCurve::readSettingsPrivate(void)
{
    Indicator::readSettingsPrivate();

    fastOptions = settings.value("FastMovingAverageOptions", qVariantFromValue(MovingAverageOptions::defaultOptions(5))).value<MovingAverageOptions>();
    mediumOptions = settings.value("MediumMovingAverageOptions", qVariantFromValue(MovingAverageOptions::defaultOptions(20))).value<MovingAverageOptions>();
}

void TwoMovingAverageCurve::writeSettingsPrivate(void)
{
    Indicator::writeSettingsPrivate();

    settings.setValue("Type", TwoMovingAveragesIndicator);
    settings.setValue("FastMovingAverageOptions", qVariantFromValue(fastOptions));
    settings.setValue("MediumMovingAverageOptions", qVariantFromValue(mediumOptions));
}

void TwoMovingAverageCurve::loadDefaultRules(void)
{
    clearRuleLists();

    ruleList(Long)->addRule("SourceIndicator=this;SourceCurve=" + fastCurve->title().text() + ";ReferenceIndicator=this;ReferenceCurve=" + mediumCurve->title().text() + ";ReferenceValue=0;Offset=0;CompareType=Kruist omhoog;LogicalOperator=En;PreviousTransactionType=Exit;Enabled=1");
    ruleList(Short)->addRule("SourceIndicator=this;SourceCurve=" + fastCurve->title().text() + ";ReferenceIndicator=this;ReferenceCurve=" + mediumCurve->title().text() + ";ReferenceValue=0;Offset=0;CompareType=Kruist omlaag;LogicalOperator=En;PreviousTransactionType=Exit;Enabled=1");
    ruleList(Exit)->addRule("SourceIndicator=this;SourceCurve=" + fastCurve->title().text() + ";ReferenceIndicator=this;ReferenceCurve=" + mediumCurve->title().text() + ";ReferenceValue=0;Offset=0;CompareType=Kruist omhoog;LogicalOperator=Of;PreviousTransactionType=Short;Enabled=1");
    ruleList(Exit)->addRule("SourceIndicator=this;SourceCurve=" + fastCurve->title().text() + ";ReferenceIndicator=this;ReferenceCurve=" + mediumCurve->title().text() + ";ReferenceValue=0;Offset=0;CompareType=Kruist omlaag;LogicalOperator=Of;PreviousTransactionType=Long;Enabled=1");

    Indicator::loadDefaultRules();
}

void TwoMovingAverageCurve::plotCurve(void)
{
    if (fastOptions.type == Simple || fastOptions.type == Exponential || fastOptions.type == LinearWeighted || fastOptions.type == Adaptive)
        calculateMACurve(*indicatorSourceCurve, *fastCurve, fastOptions);
    else
        calculateMACurve(*indicatorSourceCurve, *fastCurve, fastOptions.periods, fastOptions.type, -1, fastOptions.slowSmoothing, fastOptions.fastSmoothing);

    if (mediumOptions.type == Simple || mediumOptions.type == Exponential || mediumOptions.type == LinearWeighted || mediumOptions.type == Adaptive)
        calculateMACurve(*indicatorSourceCurve, *mediumCurve, mediumOptions);
    else
        calculateMACurve(*indicatorSourceCurve, *mediumCurve, mediumOptions.periods, mediumOptions.type, -1, mediumOptions.slowSmoothing, mediumOptions.fastSmoothing);

    Indicator::plotCurve();
}

void TwoMovingAverageCurve::updateGUI(void)
{
    bool block = blockSettingsGUISignals();
    setBlockSettingsGUISignals(true);

    fastForm->setOptions(fastOptions);
    mediumForm->setOptions(mediumOptions);

    setBlockSettingsGUISignals(block);

    Indicator::updateGUI();
}

QWidget *TwoMovingAverageCurve::customIndicatorWidget(void)
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

    QWidget *ciWidget = new QWidget;
    ciWidget->setLayout(new QVBoxLayout);
    ciWidget->layout()->addWidget(fastForm);
    ciWidget->layout()->addWidget(mediumForm);

    setTitle(QString(tr("2 Moving Averages ") + QString::number(fastOptions.periods) + "," + QString::number(mediumOptions.periods)));

    return ciWidget;
}

void TwoMovingAverageCurve::setProperties(void)
{
    if (blockSettingsGUISignals())
        return;

    fastForm->options(fastOptions);
    fastCurve->setTitle(QString(tr("Fast ") + fastForm->maTitle()));

    mediumForm->options(mediumOptions);
    mediumCurve->setTitle(QString(tr("Medium ") + mediumForm->maTitle()));

    setTitle(QString(tr("2 Moving Averages ") + QString::number(fastOptions.periods) + "," + QString::number(mediumOptions.periods)));

    Indicator::setProperties();
}
