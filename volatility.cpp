#include "volatility.h"
#include <qdialog.h>
#include <qcolordialog.h>

Volatility::Volatility(Plot *plot, PriceChart *priceChart, bool setupDialog, const QString &configGroup, Indicator::ProcessMode processMode) :
    Indicator(plot, priceChart, setupDialog, configGroup, processMode)
{
    readSettingsPrivate();

    volatilityCurve = new PlotCurve("Volatility", indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    volatilityCurve->setBrush(QBrush(Qt::SolidPattern));
    curveList.append(volatilityCurve);

    indicatorPlot->setAxisAutoScale(QwtPlot::yLeft);

    if (configGroup.isEmpty())
        loadDefaultRules();

    options.registerWithAutoOptimizer(this, "", true);
}

Volatility::~Volatility(void)
{
}


void Volatility::readSettingsPrivate(void)
{
    options = settings.value("PeriodOptions", qVariantFromValue(PeriodOptions::defaultOptions(10))).value<PeriodOptions>();

    Indicator::readSettingsPrivate();
}

void Volatility::writeSettingsPrivate(void)
{
    settings.setValue("Type", VolatilityIndicator);
    settings.setValue("PeriodOptions", qVariantFromValue(options));

    Indicator::writeSettingsPrivate();
}

void Volatility::loadDefaultRules(void)
{
    clearRuleLists();

    Indicator::loadDefaultRules();
}

void Volatility::plotCurve(void)
{
    calculateVolatilityCurve(*indicatorSourceCurve, *volatilityCurve, options);

    setXAxisScaleToCurve(*indicatorSourceCurve);

    Indicator::plotCurve();
}

void Volatility::updateGUI(void)
{
    bool block = blockSettingsGUISignals();
    setBlockSettingsGUISignals(true);

    ciWidget->setOptions(options);

    setBlockSettingsGUISignals(block);

    Indicator::updateGUI();
}


QWidget *Volatility::customIndicatorWidget(void)
{
    ciWidget = new PeriodsUI;
    ciWidget->setOptions(options);
    ciWidget->setUseHalfCycle(false);
    connect(ciWidget, SIGNAL(propertiesChanged()), this, SLOT(setProperties()));

    volatilityCurve->setTitle(tr("Volatility ") + QString::number(options.periods));
    setTitle(tr("Volatility ") + QString::number(options.periods));

    return ciWidget;
}

void Volatility::setProperties(void)
{
    if (blockSettingsGUISignals())
        return;

    ciWidget->options(options);

    volatilityCurve->setTitle(tr("Volatility ") + QString::number(options.periods));
    setTitle(tr("Volatility ") + QString::number(options.periods));

    Indicator::setProperties();
}

