#include "stochastic.h"

Stochastic::Stochastic(Plot *plot, PriceChart *priceChart, bool setupDialog, const QString &configGroup, Indicator::ProcessMode processMode) :
    Indicator(plot, priceChart, setupDialog, configGroup, processMode)
{
    KCurve = new PlotCurve("%K", indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    KCurve->setBrush(QBrush(Qt::SolidPattern));
    curveList.append(KCurve);

    DCurve = new PlotCurve("%D", indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    QPen pen = DCurve->pen();
    pen.setStyle(Qt::DotLine);
    pen.setColor(Qt::red);
    DCurve->setPen(pen);
    curveList.append(DCurve);

    QwtPlotCurve *buyLine = new QwtPlotCurve;
    buyLine->setItemAttribute(QwtPlotItem::Legend, false);
    buyLine->attach(indicatorPlot);
    pen = QPen();
    pen.setColor(Qt::green);
    pen.setWidth(1);
    pen.setStyle(Qt::DashLine);
    buyLine->setPen(pen);
    QVector<QPointF> vector;
    vector << QPointF(-1, 20.0) << QPointF(100000.0, 20.0);
    buyLine->setSamples(vector);

    QwtPlotCurve *sellLine = new QwtPlotCurve;
    sellLine->setItemAttribute(QwtPlotItem::Legend, false);
    sellLine->attach(indicatorPlot);
    pen = QPen();
    pen.setColor(Qt::red);
    pen.setWidth(1);
    pen.setStyle(Qt::DashLine);
    sellLine->setPen(pen);
    vector.clear();
    vector << QPointF(-1, 80.0) << QPointF(100000.0, 80.0);
    sellLine->setSamples(vector);

    indicatorPlot->setAxisScale(QwtPlot::yLeft, 0, 100);

    stochasticOptions.registerWithAutoOptimizer(this, tr("Stochastic"), true, 1, 30);
    KOptions.registerWithAutoOptimizer(this, tr("%K"), false, 1, 10, 0, 0, 2, 2, 30, 30);
    DOptions.registerWithAutoOptimizer(this, tr("%D"), false, 1, 10, 0, 0, 2, 2, 30, 30);
}

Stochastic::~Stochastic(void)
{
}

void Stochastic::readSettingsPrivate(void)
{
    Indicator::readSettingsPrivate();

    stochasticOptions = settings.value("StochasticPeriodOptions", qVariantFromValue(PeriodOptions::defaultOptions(14))).value<PeriodOptions>();
    KOptions = settings.value("StochasticKMovingAverageOptions", qVariantFromValue(MovingAverageOptions::defaultOptions(1))).value<MovingAverageOptions>();
    DOptions = settings.value("StochasticDMovingAverageOptions", qVariantFromValue(MovingAverageOptions::defaultOptions(3))).value<MovingAverageOptions>();
}

void Stochastic::writeSettingsPrivate(void)
{
    Indicator::writeSettingsPrivate();

    settings.setValue("Type", StochasticIndicator);
    settings.setValue("StochasticPeriodOptions", qVariantFromValue(stochasticOptions));
    settings.setValue("StochasticKMovingAverageOptions", qVariantFromValue(KOptions));
    settings.setValue("StochasticDMovingAverageOptions", qVariantFromValue(DOptions));
}

void Stochastic::loadDefaultRules(void)
{
    clearRuleLists();

    Indicator::loadDefaultRules();
}

void Stochastic::plotCurve(void)
{
    StochasticData.clear();

    QVector<QPointF>::const_iterator it = indicatorSourceCurve->data().constBegin();
    const QVector<QPointF>::const_iterator end = indicatorSourceCurve->data().constEnd();
    for (int i = 0; it != end; it++, ++i)
    {
        const QPointF &currentPoint = *it;
        double x = currentPoint.x();
        int periods = autoPeriodsForX(x, stochasticOptions);
        if (i < (periods - 1))
        {
            if (StochasticData.isEmpty())
                continue;

            periods = i + 1;
        }

        double high = -LONG_LONG_MAX;
        double low = LONG_LONG_MAX;
        for (int j = 0; j < periods; ++j)
        {
            double y = priceChart->averagePriceHigh->y(i - j);
            if (y > high)
                high = y;

            y = priceChart->averagePriceLow->y(i - j);
            if (y < low)
                low = y;
        }

        QPointF point(x, (currentPoint.y() - low) / (high - low) * 100);
        StochasticData.append(point);
    }

    if (KOptions.type == Simple || KOptions.type == Exponential || KOptions.type == LinearWeighted || KOptions.type == Adaptive)
        calculateMACurve(StochasticData, KData, KOptions);
    else
        calculateMACurve(StochasticData, KData, KOptions.periods, KOptions.type, -1, KOptions.slowSmoothing, KOptions.fastSmoothing);

    if (DOptions.type == Simple || DOptions.type == Exponential || DOptions.type == LinearWeighted || DOptions.type == Adaptive)
        calculateMACurve(KData, DData, DOptions);
    else
        calculateMACurve(KData, DData, DOptions.periods, DOptions.type, -1, DOptions.slowSmoothing, DOptions.fastSmoothing);

    KCurve->setData(KData);
    DCurve->setData(DData);

    setXAxisScaleToCurve(*indicatorSourceCurve);

    Indicator::plotCurve();
}

void Stochastic::updateGUI(void)
{
    bool block = blockSettingsGUISignals();
    setBlockSettingsGUISignals(true);

    stochasticPeriodsUI->setOptions(stochasticOptions);
    KUI->setOptions(KOptions);
    DUI->setOptions(DOptions);

    setBlockSettingsGUISignals(block);

    Indicator::updateGUI();
}

QWidget *Stochastic::customIndicatorWidget(void)
{
    stochasticPeriodsUI = new PeriodsUI(NULL, tr("Stochastic"));
    stochasticPeriodsUI->setOptions(stochasticOptions);
    connect(stochasticPeriodsUI, SIGNAL(propertiesChanged()), this, SLOT(setProperties()));

    KUI = new MovingAverageUI(NULL, tr("%K"));
    KUI->setCycleAdaptiveButtonEnabled(false);
    KUI->setOptions(KOptions);
    connect(KUI, SIGNAL(propertiesChanged()), this, SLOT(setProperties()));

    DUI = new MovingAverageUI(NULL, tr("%D"));
    DUI->setCycleAdaptiveButtonEnabled(false);
    DUI->setOptions(DOptions);
    connect(DUI, SIGNAL(propertiesChanged()), this, SLOT(setProperties()));

    QWidget *ciWidget = new QWidget;
    ciWidget->setLayout(new QVBoxLayout);
    ciWidget->layout()->addWidget(stochasticPeriodsUI);
    ciWidget->layout()->addWidget(KUI);
    ciWidget->layout()->addWidget(DUI);

    setTitle(tr("Stochastic ") + QString::number(stochasticOptions.periods) + tr(",%K") + QString::number(KOptions.periods) + tr(",%D") + QString::number(DOptions.periods));

    return ciWidget;
}

void Stochastic::setProperties(void)
{
    if (blockSettingsGUISignals())
        return;

    stochasticPeriodsUI->options(stochasticOptions);

    KUI->options(KOptions);

    DUI->options(DOptions);

    setTitle(tr("Stochastic ") + QString::number(stochasticOptions.periods) + tr(",%K") + QString::number(KOptions.periods) + tr(",%D") + QString::number(DOptions.periods));

    Indicator::setProperties();
}

