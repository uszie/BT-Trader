#include "pricechannel.h"

PriceChannel::PriceChannel(Plot *plot, PriceChart *priceChart, bool setupDialog, const QString &configGroup, Indicator::ProcessMode processMode):
    Indicator(plot, priceChart, setupDialog, configGroup, processMode)
{
    tbCurve = new PlotCurve(tr("PC: Top band"), indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    curveList.append(tbCurve);

    bbCurve = new PlotCurve(tr("PC: Bodem band"), indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    curveList.append(bbCurve);

    options.registerWithAutoOptimizer(this, tr("PC:"), true);
}

PriceChannel::~PriceChannel()
{
}

void PriceChannel::readSettingsPrivate(void)
{
    Indicator::readSettingsPrivate();

    options = settings.value("PCPeriodOptions", qVariantFromValue(PeriodOptions::defaultOptions(10))).value<PeriodOptions>();
}

void PriceChannel::writeSettingsPrivate(void)
{
    Indicator::writeSettingsPrivate();

    settings.setValue("Type", PriceChannelIndicator);
    settings.setValue("PCPeriodOptions", qVariantFromValue(options));
}

void PriceChannel::loadDefaultRules(void)
{
    clearRuleLists();

    Indicator::loadDefaultRules();
}

void PriceChannel::plotCurve(void)
{
    QVector<QPointF> bottomBand;
    QVector<QPointF> topBand;
    QPointF point;
    double low;
    double high;

    for (int i = 0; i < indicatorSourceCurve->dataSize(); ++i)
    {
        if (i < (options.periods - 1))
            continue;

        high = 0;
        low = LONG_MAX;

        for (int j = 0; j < options.periods; ++j)
        {
            if (priceChart->averagePriceHigh->y(i - j) > high)
                high = priceChart->averagePriceHigh->y(i - j);

            if (priceChart->averagePriceLow->y(i - j) < low)
                low = priceChart->averagePriceLow->y(i - j);
        }

        point.setX(indicatorSourceCurve->x(i));
        point.setY(high);
        topBand.append(point);
        point.setY(low);
        bottomBand.append(point);

    }

    tbCurve->setData(topBand);
    bbCurve->setData(bottomBand);

    Indicator::plotCurve();
}

void PriceChannel::updateGUI(void)
{
    bool block = blockSettingsGUISignals();
    setBlockSettingsGUISignals(true);

    ciWidget->setOptions(options);

    setBlockSettingsGUISignals(block);

    Indicator::updateGUI();
}

QWidget *PriceChannel::customIndicatorWidget(void)
{
    ciWidget = new PeriodsUI;
    ciWidget->setOptions(options);
    ciWidget->setUseHalfCycle(false);
    connect(ciWidget, SIGNAL(propertiesChanged()), this, SLOT(setProperties()));

    setTitle(QString(tr("Price Channel ") + QString::number(options.periods)));

    return ciWidget;
}

void PriceChannel::setProperties()
{
    if (blockSettingsGUISignals())
        return;

    ciWidget->options(options);

    setTitle(QString(tr("Price Channel ") + QString::number(options.periods)));

    Indicator::setProperties();
}
