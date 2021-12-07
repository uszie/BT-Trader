#include "bollingerbands.h"

BollingerBands::BollingerBands(Plot *plot, PriceChart *priceChart, bool setupDialog, const QString &configGroup, Indicator::ProcessMode processMode):
    Indicator(plot, priceChart, setupDialog, configGroup, processMode)
{
    tbCurve = new PlotCurve(tr("BB: Top band"), indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    curveList.append(tbCurve);

    bbCurve = new PlotCurve(tr("BB: Bodem band"), indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    curveList.append(bbCurve);

    maCurve = new PlotCurve(QString(tr("BB: ") + QString::number(20)), indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    QPen pen = maCurve->pen();
    pen.setStyle(Qt::DotLine);
    maCurve->setPen(pen);
    curveList.append(maCurve);

    addAutoOptimizerOption(tr("Standaad Deviatie"), &stdDeviation, 1, 4, 1, false);
    options.registerWithAutoOptimizer(this, tr("BB:"), true);
}

BollingerBands::~BollingerBands()
{
}


void BollingerBands::readSettingsPrivate(void)
{
    Indicator::readSettingsPrivate();

    stdDeviation = settings.value("StandardDeviation", 1).toInt();
    options = settings.value("BBMovingAverageOptions", qVariantFromValue(MovingAverageOptions::defaultOptions(20))).value<MovingAverageOptions>();
}

void BollingerBands::writeSettingsPrivate(void)
{
    Indicator::writeSettingsPrivate();

    settings.setValue("Type", BollingerBandsIndicator);
    settings.setValue("StandardDeviation", stdDeviation);
    settings.setValue("BBMovingAverageOptions", qVariantFromValue(options));
}

void BollingerBands::loadDefaultRules(void)
{
    clearRuleLists();

    //     addRule( tr( "AND<>Price Close<>Hoger dan<>EMA 20->EMA 20<>2<>0" ), longRuleList, true );
    //     addRule( tr( "AND<>Price Close<>Lager dan<>EMA 20->EMA 20<>2<>0" ), shortRuleList, true );
    //     addRule( tr( "OR<>Price Close<>Lager dan<>EMA 20->EMA 20<>1<>0" ), exitRuleList, true );
    //     addRule( tr( "OR<>Price Close<>Hoger dan<>EMA 20->EMA 20<>0<>0" ), exitRuleList, true );

    Indicator::loadDefaultRules();
}

void BollingerBands::plotCurve(void)
{
    if (options.type == Simple || options.type == Exponential || options.type == LinearWeighted || options.type == Adaptive)
        calculateMACurve(*indicatorSourceCurve, *maCurve, options);
    else
        calculateMACurve(*indicatorSourceCurve, *maCurve, options.periods, options.type, -1, options.slowSmoothing, options.fastSmoothing);


    double tmp = 0;
    QPointF point;

    tbCurve->clearData();
    bbCurve->clearData();

    int offset = indicatorSourceCurve->dataSize() - maCurve->dataSize();
    for (int i = 0; i < maCurve->dataSize(); ++i)
    {
        tmp += pow(indicatorSourceCurve->y(i + offset) -  maCurve->y(i), 2);
        if (i < (options.periods - 1))
        {
            ;
        }
        else
        {
            point.setX(maCurve->x(i));
            point.setY(maCurve->y(i) + (stdDeviation * sqrt(tmp / options.periods)));
            tbCurve->appendData(point);
            point.setY(maCurve->y(i) - (stdDeviation * sqrt(tmp / options.periods)));
            bbCurve->appendData(point);
            tmp -= pow(indicatorSourceCurve->y(i + offset - (options.periods - 1)) - maCurve->y(i - (options.periods - 1)), 2);
        }
    }

    Indicator::plotCurve();
}

void BollingerBands::updateGUI(void)
{
    //    bool block = blockSettingsGUISignals();
    //    setBlockSettingsGUISignals(true);

    //    form->sdSpinBox->setValue(stdDeviation);
    //    maForm->setOptions(options);

    //    setBlockSettingsGUISignals(block);

    Indicator::updateGUI();
}

QWidget *BollingerBands::customIndicatorWidget(void)
{
    QWidget *deviationWidget = new QWidget;
    //    form = new Ui::BollingerBandsUI;
    //    form->setupUi(deviationWidget);
    //    form->sdSpinBox->setValue(stdDeviation);
    //    connect(form->sdSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setProperties()));

    //    maForm = new MovingAverageUI;
    //    maForm->setOptions(options);
    //    maForm->setUseHalfCycle(false);
    //    connect(maForm, SIGNAL(propertiesChanged()), this, SLOT(setProperties()));

    //    QWidget *ciWidget = new QWidget;
    //    ciWidget->setLayout(new QVBoxLayout);
    //    ciWidget->layout()->addWidget(deviationWidget);
    //    ciWidget->layout()->addWidget(maForm);

    //    maCurve->setTitle(QString(tr("BB: ") + maForm->maTitle()));

    //    setTitle(QString(tr("Bollinger Bands ") + maForm->maTitle()));

    //    return ciWidget;

    return deviationWidget;
}

void BollingerBands::setProperties(void)
{
    //    if (blockSettingsGUISignals())
    //        return;

    //    stdDeviation = form->sdSpinBox->value();
    //    maForm->options(options);

    //    maCurve->setTitle(QString(tr("BB: ") + maForm->maTitle()));

    //    setTitle(QString(tr("Bollinger Bands ") + maForm->maTitle()));

    Indicator::setProperties();
}

