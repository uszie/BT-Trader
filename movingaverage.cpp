#include "movingaverage.h"

MovingAverageCurve::MovingAverageCurve(Plot *plot, PriceChart *priceChart, bool setupDialog, const QString &configGroup, Indicator::ProcessMode processMode) :
    Indicator(plot, priceChart, setupDialog, configGroup, processMode)
{

    curve = new PlotCurve(tr("SMA 20"), indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    curveList.append(curve);

    options.registerWithAutoOptimizer(this, "MA:", true);
}

MovingAverageCurve::~MovingAverageCurve(void)
{
}

void MovingAverageCurve::readSettingsPrivate(void)
{
    Indicator::readSettingsPrivate();

    options = settings.value("MovingAverageOptions", qVariantFromValue(MovingAverageOptions::defaultOptions())).value<MovingAverageOptions>();
}

void MovingAverageCurve::writeSettingsPrivate(void)
{
    Indicator::writeSettingsPrivate();

    settings.setValue("Type", MovingAverageIndicator);
    settings.setValue("MovingAverageOptions", qVariantFromValue(options));
}

void MovingAverageCurve::loadDefaultRules(void)
{
    clearRuleLists();

    ruleList(Long)->addRule("SourceIndicator=NULL;SourceCurve=Price Close;ReferenceIndicator=this;ReferenceCurve=" + curve->title().text() + ";ReferenceValue=0;Offset=0;CompareType=Kruist omhoog;LogicalOperator=En;PreviousTransactionType=Exit;Enabled=1");
    ruleList(Short)->addRule("SourceIndicator=NULL;SourceCurve=Price Close;ReferenceIndicator=this;ReferenceCurve=" + curve->title().text() + ";ReferenceValue=0;Offset=0;CompareType=Kruist omlaag;LogicalOperator=En;PreviousTransactionType=Exit;Enabled=1");
    ruleList(Exit)->addRule("SourceIndicator=NULL;SourceCurve=Price Close;ReferenceIndicator=this;ReferenceCurve=" + curve->title().text() + ";ReferenceValue=0;Offset=0;CompareType=Kruist omhoog;LogicalOperator=Of;PreviousTransactionType=Short;Enabled=1");
    ruleList(Exit)->addRule("SourceIndicator=NULL;SourceCurve=Price Close;ReferenceIndicator=this;ReferenceCurve=" + curve->title().text() + ";ReferenceValue=0;Offset=0;CompareType=Kruist omlaag;LogicalOperator=Of;PreviousTransactionType=Long;Enabled=1");

    Indicator::loadDefaultRules();
}

void MovingAverageCurve::plotCurve(void)
{
    if (options.type == Simple || options.type == Exponential || options.type == LinearWeighted || options.type == Adaptive)
        calculateMACurve(*indicatorSourceCurve, *curve, options);
    else
        calculateMACurve(*indicatorSourceCurve, *curve, options.periods, options.type, -1, options.slowSmoothing, options.fastSmoothing);

    Indicator::plotCurve();
}

void MovingAverageCurve::updateGUI(void)
{
    bool block = blockSettingsGUISignals();
    setBlockSettingsGUISignals(true);

    ciWidget->setOptions(options);

    setBlockSettingsGUISignals(block);

    Indicator::updateGUI();
}

QWidget *MovingAverageCurve::customIndicatorWidget(void)
{
    ciWidget = new MovingAverageUI;
    ciWidget->setOptions(options);
    ciWidget->setUseHalfCycle(true);
    connect(ciWidget, SIGNAL(propertiesChanged()), this, SLOT(setProperties()));
    curve->setTitle(ciWidget->maTitle());

    setTitle(ciWidget->maTitle(false));

    return ciWidget;
}

void MovingAverageCurve::setProperties(void)
{
    if (blockSettingsGUISignals())
        return;

    ciWidget->options(options);
    curve->setTitle(ciWidget->maTitle());

    setTitle(ciWidget->maTitle(false));

    Indicator::setProperties();
}
