#include "er.h"
#include <qdialog.h>
#include <qcolordialog.h>

ER::ER(Plot *plot, PriceChart *priceChart, bool setupDialog, const QString &configGroup, Indicator::ProcessMode processMode) :
    Indicator(plot, priceChart, setupDialog, configGroup, processMode)
{
    erCurve = new PlotCurve(tr("ER"), indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    erCurve->setBrush(QBrush(Qt::SolidPattern));
    curveList.append(erCurve);

    maCurve = new PlotCurve(tr("EMA"), indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    QPen pen = maCurve->pen();
    pen.setStyle(Qt::DotLine);
    pen.setColor(Qt::red);
    maCurve->setPen(pen);
    curveList.append(erCurve);

    QwtPlotCurve *trendBorder = new QwtPlotCurve;
    trendBorder->setItemAttribute(QwtPlotItem::Legend, false);
    trendBorder->attach(indicatorPlot);
    pen = QPen();
    pen.setColor(Qt::white);
    pen.setWidth(1);
    pen.setStyle(Qt::DashLine);
    trendBorder->setPen(pen);
    QVector<QPointF> vector;
    vector << QPointF(-1, 0.0) << QPointF(100000.0, 0.0) << QPointF(1000000.0, 0.0);
    trendBorder->setSamples(vector);


    indicatorPlot->setAxisScale(QwtPlot::yLeft, -1, 1);

    erOptions.registerWithAutoOptimizer(this, tr("ER:"), true);
    maOptions.registerWithAutoOptimizer(this, tr("MA:"), true);
}

ER::~ER(void)
{
}

void ER::readSettingsPrivate(void)
{
    Indicator::readSettingsPrivate();

    erOptions = settings.value("ERPeriodOptions", qVariantFromValue(PeriodOptions::defaultOptions(10))).value<PeriodOptions>();
    maOptions = settings.value("MovingAverageOptions", qVariantFromValue(MovingAverageOptions::defaultOptions(5))).value<MovingAverageOptions>();
}

void ER::writeSettingsPrivate(void)
{
    Indicator::writeSettingsPrivate();

    settings.setValue("Type", ERIndicator);
    settings.setValue("ERPeriodOptions", qVariantFromValue(erOptions));
    settings.setValue("MovingAverageOptions", qVariantFromValue(maOptions));
}

void ER::loadDefaultRules(void)
{
    clearRuleLists();

    Indicator::loadDefaultRules();
}

void ER::plotCurve(void)
{
    calculateERCurve(*indicatorSourceCurve, *erCurve, erOptions);
    calculateMACurve(*erCurve, *maCurve, maOptions);

    setXAxisScaleToCurve(*indicatorSourceCurve);

    Indicator::plotCurve();
}

void ER::updateGUI(void)
{
    bool block = blockSettingsGUISignals();
    setBlockSettingsGUISignals(true);

    erForm->setOptions(erOptions);
    maForm->setOptions(maOptions);

    setBlockSettingsGUISignals(block);

    Indicator::updateGUI();
}

QWidget *ER::customIndicatorWidget(void)
{
    erForm = new PeriodsUI(NULL, tr("ER"));
    erForm->setOptions(erOptions);
    erForm->setUseHalfCycle(false);
    connect(erForm, SIGNAL(propertiesChanged()), this, SLOT(setProperties()));
    erCurve->setTitle(QString(tr("ER ") + QString::number(erForm->periods())));

    maForm = new MovingAverageUI(NULL, tr("MA"));
    maForm->setOptions(maOptions);
    maForm->setCycleAdaptiveButtonEnabled(false);
    maForm->setUseHalfCycle(true);
    connect(maForm, SIGNAL(propertiesChanged()), this, SLOT(setProperties()));
    maCurve->setTitle(maForm->maTitle());

    QWidget *ciWidget = new QWidget;
    ciWidget->setLayout(new QVBoxLayout(ciWidget));
    ciWidget->layout()->addWidget(erForm);
    ciWidget->layout()->addWidget(maForm);

    setTitle(QString(tr("ER ") + QString::number(erForm->periods()) + "," + maForm->maTitle()));

    return ciWidget;
}

void ER::setProperties(void)
{
    if (blockSettingsGUISignals())
        return;

    erForm->options(erOptions);
    erCurve->setTitle(QString(tr("ER ") + QString::number(erForm->periods())));

    maForm->options(maOptions);
    maCurve->setTitle(maForm->maTitle());

    setTitle(QString(tr("ER ") + QString::number(erForm->periods()) + "," + maForm->maTitle()));

    Indicator::setProperties();
}
