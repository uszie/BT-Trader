#include "adx.h"

ADX::ADX(Plot *plot, PriceChart *priceChart, bool setupDialog, const QString &configGroup, Indicator::ProcessMode processMode) :
    Indicator(plot, priceChart, setupDialog, configGroup, processMode)
{
    adxCurve = new PlotCurve(tr("ADX"), indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    adxCurve->setBrush(Qt::SolidPattern);
    curveList.append(adxCurve);

    plusDICurve = new PlotCurve(tr("+DI"), indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    QPen pen = plusDICurve->pen();
    pen.setStyle(Qt::DotLine);
    pen.setColor(Qt::green);
    plusDICurve->setPen(pen);
    curveList.append(plusDICurve);

    minDICurve = new PlotCurve(tr("-DI"), indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    pen = minDICurve->pen();
    pen.setStyle(Qt::DotLine);
    pen.setColor(Qt::red);
    minDICurve->setPen(pen);
    curveList.append(minDICurve);

    indicatorPlot->setAxisScale(QwtPlot::yLeft, 0, 100);
    setIndicatorSourceComboBoxHidden(true);

    QwtPlotCurve *trendBorder = new QwtPlotCurve;
    trendBorder->setItemAttribute(QwtPlotItem::Legend, false);
    trendBorder->attach(indicatorPlot);
    pen = QPen();
    pen.setColor(Qt::white);
    pen.setWidth(1);
    pen.setStyle(Qt::DashLine);
    trendBorder->setPen(pen);
    QVector<QPointF> vector;
    vector << QPointF(-1, 25.0) << QPointF(100000.0, 25.0);
    trendBorder->setSamples(vector);

    options.registerWithAutoOptimizer(this, tr("ADX:"), true);
}

ADX::~ADX(void) {}

void ADX::readSettingsPrivate(void)
{
    Indicator::readSettingsPrivate();

    options = settings.value("ADXMovingAverageOptions", qVariantFromValue(MovingAverageOptions::defaultOptions(14))).value<MovingAverageOptions>();
}

void ADX::writeSettingsPrivate(void)
{
    Indicator::writeSettingsPrivate();

    settings.setValue("Type", ADXIndicator);
    settings.setValue("ADXMovingAverageOptions", qVariantFromValue(options));
}


void ADX::loadDefaultRules(void)
{
    clearRuleLists();

    Rule *rule = ruleList(Long)->addRule("SourceIndicator=this;SourceCurve=" + adxCurve->title().text() + ";ReferenceIndicator=NULL;ReferenceCurve=NULL;ReferenceValue=25;Offset=0;CompareType=Kruist omhoog;LogicalOperator=En;PreviousTransactionType=Exit;Enabled=1");
    if (rule)
        ruleList(Long)->addRule("SourceIndicator=this;SourceCurve=" + plusDICurve->title().text() + ";ReferenceIndicator=this;ReferenceCurve=" + minDICurve->title().text() + ";ReferenceValue=0;Offset=0;CompareType=Kruist omhoog;LogicalOperator=En;PreviousTransactionType=Exit;Enabled=1", rule);

    rule = ruleList(Short)->addRule("SourceIndicator=this;SourceCurve=" + adxCurve->title().text() + ";ReferenceIndicator=NULL;ReferenceCurve=NULL;ReferenceValue=25;Offset=0;CompareType=Kruist omlaag;LogicalOperator=En;PreviousTransactionType=Exit;Enabled=1");
    if (rule)
        ruleList(Short)->addRule("SourceIndicator=this;SourceCurve=" + plusDICurve->title().text() + ";ReferenceIndicator=this;ReferenceCurve=" + minDICurve->title().text() + ";ReferenceValue=0;Offset=0;CompareType=Kruist omlaag;LogicalOperator=En;PreviousTransactionType=Exit;Enabled=1", rule);

    ruleList(Exit)->addRule("SourceIndicator=this;SourceCurve=" + plusDICurve->title().text() + ";ReferenceIndicator=this;ReferenceCurve=" + minDICurve->title().text() + ";ReferenceValue=0;Offset=0;CompareType=Kruist omlaag;LogicalOperator=Of;PreviousTransactionType=Short;Enabled=1");
    ruleList(Exit)->addRule("SourceIndicator=this;SourceCurve=" + plusDICurve->title().text() + ";ReferenceIndicator=this;ReferenceCurve=" + minDICurve->title().text() + ";ReferenceValue=0;Offset=0;CompareType=Kruist omhoog;LogicalOperator=Of;PreviousTransactionType=Long;Enabled=1");

    Indicator::loadDefaultRules();
}

void ADX::plotCurve(void)
{
    plusDM.clear();
    minDM.clear();
    plusADM.clear();
    minADM.clear();
    TR.clear();
    ATR.clear();
    DX.clear();
    _ADX.clear();
    plusDICurve->clearData();
    minDICurve->clearData();
    adxCurve->clearData();

    if (indicatorSourceCurve->dataSize() < 1)
    {
        adxCurve->setData(QVector<QPointF>());
        Indicator::plotCurve();
        return;
    }

    for (int i = 1; i < indicatorSourceCurve->dataSize(); ++i)
    {
        QPointF point;
        point.setX(indicatorSourceCurve->x(i));
        double deltaHigh = priceChart->averagePriceHigh->y(i) - priceChart->averagePriceHigh->y(i - 1);
        double deltaLow = priceChart->averagePriceLow->y(i - 1) - priceChart->averagePriceLow->y(i);

        if ((deltaHigh < 0 && deltaLow < 0) || (deltaHigh == deltaLow))
        {
            point.setY(0);
            plusDM.append(point);
            minDM.append(point);
        }
        else if (deltaHigh > deltaLow)
        {
            point.setY(0);
            minDM.append(point);
            point.setY(deltaHigh);
            plusDM.append(point);
        }
        else
        {
            point.setY(0);
            plusDM.append(point);
            point.setY(deltaLow);
            minDM.append(point);
        }
    }

    if (options.type == Simple || options.type == Exponential || options.type == LinearWeighted || options.type == Adaptive)
    {
        calculateMACurve(plusDM, plusADM, options);
        calculateMACurve(minDM, minADM, options);
    }
    else
    {
        calculateMACurve(plusDM, plusADM, options.periods, options.type, -1, options.slowSmoothing, options.fastSmoothing);
        calculateMACurve(minDM, minADM, options.periods, options.type, -1, options.slowSmoothing, options.fastSmoothing);
    }

    calculateTRCurve(priceChart->averagePriceHigh->data(), priceChart->averagePriceLow->data(), priceChart->averagePriceClose->data(), TR, priceChart->averagePriceClose->dataSize());

    if (options.type == Simple || options.type == Exponential || options.type == LinearWeighted || options.type == Adaptive)
    {
        calculateMACurve(TR, ATR, options);
        calculateMACurve(TR, ATR, options);
    }
    else
    {
        calculateMACurve(TR, ATR, options.periods, options.type, -1, options.slowSmoothing, options.fastSmoothing);
        calculateMACurve(TR, ATR, options.periods, options.type, -1, options.slowSmoothing, options.fastSmoothing);
    }

    int i;
    for (i = 0; i < plusADM.size(); ++ i)
    {
        QPointF point;
        point.setX(plusADM.at(i).x());
        point.setY((plusADM.at(i).y() / ATR.at(i).y()) * 100);
        plusDICurve->appendData(point);

        point.setY((minADM.at(i).y() / ATR.at(i).y()) * 100);
        minDICurve->appendData(point);
    }

    int delay = 0;
    if (options.type == Exponential)
        delay = options.periods - 1;

    for (int i = delay; i < plusDICurve->dataSize(); ++i)
    {
        double absoluteDifferenceIndices = plusDICurve->y(i) - minDICurve->y(i);
        if (absoluteDifferenceIndices < 0)
            absoluteDifferenceIndices = absoluteDifferenceIndices * -1;

        QPointF point;
        point.setX(plusDICurve->x(i));
        point.setY((absoluteDifferenceIndices / (plusDICurve->y(i) + minDICurve->y(i))) * 100);
        DX.append(point);
    }

    if (options.type == Simple || options.type == Exponential || options.type == LinearWeighted || options.type == Adaptive)
    {
        calculateMACurve(DX, _ADX, options);
        calculateMACurve(DX, _ADX, options);
    }
    else
    {
        calculateMACurve(DX, _ADX, options.periods, options.type, -1, options.slowSmoothing, options.fastSmoothing);
        calculateMACurve(DX, _ADX, options.periods, options.type, -1, options.slowSmoothing, options.fastSmoothing);
    }

    adxCurve->setData(_ADX);

    indicatorPlot->setXAxisScaleToCurve(*indicatorSourceCurve);

    Indicator::plotCurve();
}

void ADX::updateGUI(void)
{
    bool block = blockSettingsGUISignals();
    setBlockSettingsGUISignals(true);

    ciWidget->setOptions(options);

    setBlockSettingsGUISignals(block);

    Indicator::updateGUI();
}

QWidget *ADX::customIndicatorWidget(void)
{
    ciWidget = new MovingAverageUI;
    ciWidget->setOptions(options);
    ciWidget->setUseHalfCycle(false);
    ciWidget->setCycleAdaptiveButtonEnabled(false);
    connect(ciWidget, SIGNAL(propertiesChanged()), this, SLOT(setProperties()));
    adxCurve->setTitle(QString(tr("ADX ") + QString::number(options.periods)));

    setTitle(QString(tr("ADX ") + ciWidget->maTitle()));

    return ciWidget;
}

void ADX::setProperties()
{
    if (blockSettingsGUISignals())
        return;

    ciWidget->options(options);
    adxCurve->setTitle(QString(tr("ADX ") + QString::number(options.periods)));

    setTitle(QString(tr("ADX ") + ciWidget->maTitle()));

    Indicator::setProperties();
}
