#include "instantaneoustrend.h"

InstantaneousTrend::InstantaneousTrend(Plot *plot, PriceChart *priceChart, bool setupDialog, const QString &configGroup, Indicator::ProcessMode processMode):
    Indicator(plot, priceChart, setupDialog, configGroup, processMode)
{
    iTrend = new PlotCurve(tr("ITrend"), indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    curveList.append(iTrend);

    iTrendTrigger = new PlotCurve(tr("ITrend Trigger"), indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    QPen pen = iTrendTrigger->pen();
    pen.setStyle(Qt::DotLine);
    iTrendTrigger->setPen(pen);
    curveList.append(iTrendTrigger);

    setIndicatorSourceComboBoxHidden(true);
    setTitle(tr("Instantaneous Trend"));
}

InstantaneousTrend::~InstantaneousTrend()
{
}

void InstantaneousTrend::writeSettingsPrivate(void)
{
    Indicator::writeSettingsPrivate();

    settings.setValue("Type", ITrendIndicator);
}

void InstantaneousTrend::loadDefaultRules(void)
{
    clearRuleLists();

    ruleList(Long)->addRule("SourceIndicator=this;SourceCurve=" + iTrendTrigger->title().text() + ";ReferenceIndicator=this;ReferenceCurve=" + iTrend->title().text() + ";ReferenceValue=0;Offset=0;CompareType=Kruist omhoog;LogicalOperator=En;PreviousTransactionType=Exit;Enabled=1");
    ruleList(Short)->addRule("SourceIndicator=this;SourceCurve=" + iTrendTrigger->title().text() + ";ReferenceIndicator=this;ReferenceCurve=" + iTrend->title().text() + ";ReferenceValue=0;Offset=0;CompareType=Kruist omlaag;LogicalOperator=En;PreviousTransactionType=Exit;Enabled=1");
    ruleList(Exit)->addRule("SourceIndicator=this;SourceCurve=" + iTrendTrigger->title().text() + ";ReferenceIndicator=this;ReferenceCurve=" + iTrend->title().text() + ";ReferenceValue=0;Offset=0;CompareType=Kruist omlaag;LogicalOperator=Of;PreviousTransactionType=Long;Enabled=1");
    ruleList(Exit)->addRule("SourceIndicator=this;SourceCurve=" + iTrendTrigger->title().text() + ";ReferenceIndicator=this;ReferenceCurve=" + iTrend->title().text() + ";ReferenceValue=0;Offset=0;CompareType=Kruist omhoog;LogicalOperator=Of;PreviousTransactionType=Short;Enabled=1");

    Indicator::loadDefaultRules();
}

void InstantaneousTrend::plotCurve(void)
{
    double alpha = 0.07;

    iTrend->clearData();
    iTrendTrigger->clearData();
    median.clear();
    it.clear();
    trigger.clear();

    calculateMedianCurve(priceChart->averagePriceLow->data(), priceChart->averagePriceHigh->data(), median);
    it = trigger = median;

    for (int i = 2; i < median.size() && i < 6; ++i)
    {
        it[ i ].setY((median[i].y() + 2 * median[ i - 1].y() + median[i - 2].y()) / 4);
        if (i > 3)
            trigger[ i ].setY(2 * it[i].y() - it[i - 2].y());
    }

    for (int i = 6; i < median.size(); ++i)
    {
        it[ i ].setY((alpha - alpha * alpha / 4)*median[i].y() +
                     0.5 * alpha * alpha * median[i - 1].y() -
                     (alpha - 0.75 * alpha * alpha)*median[i - 2].y() +
                     2 * (1 - alpha)*it[i - 1].y() -
                     (1 - alpha) * (1 - alpha)*it[i - 2].y());

        trigger[ i ].setY(2 * it[i].y() - it[i - 2].y());
    }

    for (int i = 0; i < 4; ++i)
    {
        if (i < 2)
            it.remove(0);
        trigger.remove(0);
    }

    iTrend->setData(it);
    iTrendTrigger->setData(trigger);

    Indicator::plotCurve();
}
