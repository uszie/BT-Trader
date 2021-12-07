#include "trend.h"
#include <qwt_raster_data.h>
#include <qwt_color_map.h>

class SpectrogramData: public QwtRasterData
{
    public:
        SpectrogramData(const PlotCurve *shortTrendCurve, const PlotCurve *mediumTrendCurve, const PlotCurve *longTrendCurve):
            QwtRasterData((longTrendCurve->dataSize() > 2) ? QwtDoubleRect(longTrendCurve->x(0), -40000, longTrendCurve->x(longTrendCurve->dataSize() - 1) - longTrendCurve->x(0) , 6000) : QwtDoubleRect(0, 0, 0, 0))
        {
            p_shortTrendCurve = shortTrendCurve;
            p_mediumTrendCurve = mediumTrendCurve;
            p_longTrendCurve = longTrendCurve;
        }

        virtual QwtRasterData *copy() const
        {
            return new SpectrogramData(p_shortTrendCurve, p_mediumTrendCurve, p_longTrendCurve);
        }

        virtual QwtDoubleInterval range() const
        {
            return QwtDoubleInterval(0, 1.0);
        }

        virtual QSize rasterHint(const QwtDoubleRect &rect) const
        {
            if (p_shortTrendCurve->dataSize() > 2000 && rect.width()  > 2000)
            {
                return QSize(2000, 3);
            }

            return QSize(p_shortTrendCurve->dataSize(), 3);
        }

        virtual double value(double x, double y) const
        {
            int address;
            if (y <= -38000)
            {
                address = p_longTrendCurve->indexOffX(x);
                if (address < 0)
                    return 0.0;

                return (1.0 / 20000.0) * ((p_longTrendCurve->y(address) + 10000.0));
            }
            else if (y <= -36000)
            {
                address = p_mediumTrendCurve->indexOffX(x);
                if (address < 0)
                    return 0.0;

                return (1.0 / 20000.0) * ((p_mediumTrendCurve->y(address) + 10000.0));
            }
            else
            {
                address = p_shortTrendCurve->indexOffX(x);
                if (address < 0)
                    return 0.0;

                return (1.0 / 20000.0) * ((p_shortTrendCurve->y(address) + 10000.0));
            }

            return 0.0;
        }

    private:
        const PlotCurve *p_shortTrendCurve;
        const PlotCurve *p_mediumTrendCurve;
        const PlotCurve *p_longTrendCurve;
};


Trend::Trend(Plot *plot, PriceChart *priceChart, bool setupDialog, const QString &configGroup, Indicator::ProcessMode processMode) :
    Indicator(plot, priceChart, setupDialog, configGroup, processMode)
{

    readSettingsPrivate();

    stTrend = new PlotCurve(tr("Korte Termijn Trend"), indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    stTrend->setYAxis(QwtPlot::yRight);
    curveList.append(stTrend);

    ma3 = new PlotCurve("3", indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    curveList.append(ma3);

    ma5 = new PlotCurve("5", indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    curveList.append(ma5);

    ma8 = new PlotCurve("8", indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    curveList.append(ma8);

    ma10 = new PlotCurve("10", indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    curveList.append(ma10);

    ma12 = new PlotCurve("12", indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    curveList.append(ma12);

    ma15 = new PlotCurve("15", indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    curveList.append(ma15);


    mtTrend = new PlotCurve(tr("Midden Termijn Trend") , indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    mtTrend->setYAxis(QwtPlot::yRight);
    curveList.append(mtTrend);

    ma30 = new PlotCurve("30", indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    curveList.append(ma30);

    ma35 = new PlotCurve("35", indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    curveList.append(ma35);

    ma40 = new PlotCurve("40", indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    curveList.append(ma40);

    ma45 = new PlotCurve("45", indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    curveList.append(ma45);

    ma50 = new PlotCurve("50", indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    curveList.append(ma50);

    ltTrend = new PlotCurve(tr("Lange Termijn Trend"), indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    ltTrend->setYAxis(QwtPlot::yRight);
    curveList.append(ltTrend);

    ma60 = new PlotCurve("60", indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    curveList.append(ma60);

    ma70 = new PlotCurve("70", indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    curveList.append(ma70);

    ma80 = new PlotCurve("80", indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    curveList.append(ma80);

    ma90 = new PlotCurve("90", indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    curveList.append(ma90);

    ma100 = new PlotCurve("100", indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    curveList.append(ma100);

    ma120 = new PlotCurve("120", indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    curveList.append(ma120);

    TR = new PlotCurve("TR", indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    curveList.append(TR);

    ATR = new PlotCurve("ATR", indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    curveList.append(ATR);

    d_spectrogram = new PlotSpectrogram();

    QwtLinearColorMap colorMap(QColor("#FF6666"),  QColor("#7EFC00"), QwtColorMap::RGB);
    colorMap.addColorStop(0.4, QColor("#FF6666"));
    colorMap.addColorStop(0.5, QColor("#FFFF99"));
    colorMap.addColorStop(0.6, QColor("#7EFC00"));
    d_spectrogram->setColorMap(colorMap);
    d_spectrogram->setYAxis(QwtPlot::yRight);
    d_spectrogram->attach(indicatorPlot);
    d_spectrogram->setVisible(false);

    setTitle("Trend");

    if (configGroup.isEmpty())
        loadDefaultRules();
}

Trend::~Trend(void)
{
    d_spectrogram->detach();
    delete d_spectrogram;
}

void Trend::readSettingsPrivate(void)
{
    maType = settings.value("MaType", Exponential).toInt();
    fastSmoothing = settings.value("FastSmoothing", 2).toInt();
    slowSmoothing = settings.value("SlowSmoothing", 30).toInt();

    Indicator::readSettingsPrivate();
}

void Trend::writeSettingsPrivate(void)
{
    settings.setValue("Type", TrendIndicator);
    settings.setValue("MaType", maType);
    settings.setValue("FastSmoothing", fastSmoothing);
    settings.setValue("SlowSmoothing", slowSmoothing);

    Indicator::writeSettingsPrivate();
}

void Trend::loadDefaultRules(void)
{
    clearRuleLists();

    Indicator::loadDefaultRules();
}

void Trend::plotCurve(void)
{
    stTrend->clearData();
    mtTrend->clearData();
    ltTrend->clearData();
    ma3->clearData();
    ma5->clearData();
    ma8->clearData();
    ma10->clearData();
    ma12->clearData();
    ma15->clearData();
    ma30->clearData();
    ma35->clearData();
    ma40->clearData();
    ma45->clearData();
    ma50->clearData();
    ma60->clearData();
    ma70->clearData();
    ma80->clearData();
    ma90->clearData();
    ma100->clearData();
    ma120->clearData();
    TR->clearData();
    ATR->clearData();

    if (indexPlot->periods <= 0)
    {
        d_spectrogram->setVisible(false);
        Indicator::plotCurve();
    }

    d_spectrogram->setVisible(true);

    calculateMACurve(*indicatorSourceCurve, *ma3, 3, maType, -1, slowSmoothing, fastSmoothing);
    calculateMACurve(*indicatorSourceCurve, *ma5, 5, maType, slowSmoothing, fastSmoothing);
    calculateMACurve(*indicatorSourceCurve, *ma8, 8, maType, slowSmoothing, fastSmoothing);
    calculateMACurve(*indicatorSourceCurve, *ma10, 10, maType, -1, slowSmoothing, fastSmoothing);
    calculateMACurve(*indicatorSourceCurve, *ma12, 12, maType, -1, slowSmoothing, fastSmoothing);
    calculateMACurve(*indicatorSourceCurve, *ma15, 15, maType, -1, slowSmoothing, fastSmoothing);

    calculateMACurve(*indicatorSourceCurve, *ma30, 30, maType, -1, slowSmoothing, fastSmoothing);
    calculateMACurve(*indicatorSourceCurve, *ma35, 35, maType, -1, slowSmoothing, fastSmoothing);
    calculateMACurve(*indicatorSourceCurve, *ma40, 40, maType, -1, slowSmoothing, fastSmoothing);
    calculateMACurve(*indicatorSourceCurve, *ma45, 45, maType, -1, slowSmoothing, fastSmoothing);
    calculateMACurve(*indicatorSourceCurve, *ma50, 50, maType, -1, slowSmoothing, fastSmoothing);

    calculateMACurve(*indicatorSourceCurve, *ma60, 60, maType, -1, slowSmoothing, fastSmoothing);
    calculateMACurve(*indicatorSourceCurve, *ma70, 70, maType, -1, slowSmoothing, fastSmoothing);
    calculateMACurve(*indicatorSourceCurve, *ma80, 80, maType, -1, slowSmoothing, fastSmoothing);
    calculateMACurve(*indicatorSourceCurve, *ma90, 90, maType, -1, slowSmoothing, fastSmoothing);
    calculateMACurve(*indicatorSourceCurve, *ma100, 100, maType, -1, slowSmoothing, fastSmoothing);
    calculateMACurve(*indicatorSourceCurve, *ma120, 120, maType, -1, slowSmoothing, fastSmoothing);

    calculateTRCurve(*priceChart->averagePriceHigh, *priceChart->averagePriceLow, *priceChart->averagePriceClose, *TR, priceChart->averagePriceClose->dataSize());
    calculateMACurve(*TR, *ATR, 100, Exponential);

    double SGN;
    double strength;
    QPointF point;
    QVector<QPointF> buffer;
    int ma3Address;
    int ma5Address;
    int ma8Address;
    int ma10Address;
    int ma12Address;

    int ma30Address;
    int ma35Address;
    int ma40Address;
    int ma45Address;
    int ma50Address;

    int ma60Address;
    int ma70Address;
    int ma80Address;
    int ma90Address;
    int ma100Address;
    int atrAddress;

    stTrend->clearData();
    buffer.clear();
    for (int i = 1; i < ma15->dataSize(); ++i)
    {
        ma3Address = ma3->indexOffX(ma15->x(i));
        if (ma3Address < 0)
            continue;

        ma5Address = ma5->indexOffX(ma15->x(i));
        if (ma5Address < 0)
            continue;

        ma8Address = ma8->indexOffX(ma15->x(i));
        if (ma8Address < 0)
            continue;

        ma10Address = ma10->indexOffX(ma15->x(i));
        if (ma10Address < 0)
            continue;

        ma12Address = ma12->indexOffX(ma15->x(i));
        if (ma12Address < 0)
            continue;

        atrAddress = ATR->indexOffX(ma15->x(i));
        if (atrAddress < 0)
            continue;

        if (ma8->y(ma8Address) > ma10->y(ma10Address))
            SGN = 1.0;
        else
            SGN = -1.0;

        if (qFuzzyIsNull(pow(ATR->y(atrAddress) / 100.0, 2.0)))
            strength = 0.0;
        else
            strength = (pow(ma3->y(ma3Address) - ma5->y(ma5Address), 2.0) + pow(ma5->y(ma5Address) - ma8->y(ma8Address), 2.0) + pow(ma8->y(ma8Address) - ma10->y(ma10Address), 2.0) + pow(ma10->y(ma10Address) - ma12->y(ma12Address), 2.0) + pow(ma12->y(ma12Address) - ma15->y(i), 2.0)) / pow(ATR->y(atrAddress) / 100.0, 2.0) * SGN;

        point.setX(ma15->x(i));
        point.setY(strength);
        buffer.append(point);
    }
    stTrend->setData(buffer);

    mtTrend->clearData();
    buffer.clear();
    for (int i = 1; i < ma60->dataSize(); ++i)
    {
        ma30Address = ma30->indexOffX(ma60->x(i));
        if (ma30Address < 0)
            continue;

        ma35Address = ma35->indexOffX(ma60->x(i));
        if (ma35Address < 0)
            continue;

        ma40Address = ma40->indexOffX(ma60->x(i));
        if (ma40Address < 0)
            continue;

        ma45Address = ma45->indexOffX(ma60->x(i));
        if (ma45Address < 0)
            continue;

        ma50Address = ma50->indexOffX(ma60->x(i));
        if (ma50Address < 0)
            continue;

        atrAddress = ATR->indexOffX(ma60->x(i));
        if (atrAddress < 0)
            continue;

        if (ma40->y(ma40Address) > ma45->y(ma45Address))
            SGN = 1;
        else
            SGN = -1;

        strength = (pow(ma30->y(ma30Address) - ma35->y(ma35Address), 2) + pow(ma35->y(ma35Address) - ma40->y(ma40Address), 2) + pow(ma40->y(ma40Address) - ma45->y(ma45Address), 2) + pow(ma45->y(ma45Address) - ma50->y(ma50Address), 2) + pow(ma50->y(ma50Address) - ma60->y(i), 2)) / pow(ATR->y(atrAddress) / 100.0, 2) * SGN;
        point.setX(ma60->x(i));
        point.setY(strength);
        buffer.append(point);
    }

    mtTrend->setData(buffer);

    ltTrend->clearData();
    buffer.clear();
    for (int i = 1; i < ma120->dataSize(); ++i)
    {
        ma60Address = ma60->indexOffX(ma120->x(i));
        if (ma60Address < 0)
            continue;

        ma70Address = ma70->indexOffX(ma120->x(i));
        if (ma70Address < 0)
            continue;

        ma80Address = ma80->indexOffX(ma120->x(i));
        if (ma80Address < 0)
            continue;

        ma90Address = ma90->indexOffX(ma120->x(i));
        if (ma90Address < 0)
            continue;

        ma100Address = ma100->indexOffX(ma120->x(i));
        if (ma100Address < 0)
            continue;

        atrAddress = ATR->indexOffX(ma120->x(i));
        if (atrAddress < 0)
            continue;

        if (ma80->y(ma80Address) > ma90->y(ma90Address))
            SGN = 1;
        else
            SGN = -1;

        strength = (pow(ma60->y(ma60Address) - ma70->y(ma70Address), 2) + pow(ma70->y(ma70Address) - ma80->y(ma80Address), 2) + pow(ma80->y(ma80Address) - ma90->y(ma90Address), 2) + pow(ma90->y(ma90Address) - ma100->y(ma100Address), 2) + pow(ma100->y(ma100Address) - ma120->y(i), 2)) / pow(ATR->y(atrAddress) / 100.0, 2) * SGN;
        point.setX(ma120->x(i));
        point.setY(strength);
        buffer.append(point);
    }

    ltTrend->setData(buffer);
    d_spectrogram->setData(SpectrogramData(stTrend, mtTrend, ltTrend));

    Indicator::plotCurve();
}


QWidget *Trend::customIndicatorWidget(void)
{
    ciWidget = new MovingAverageUI;
    ciWidget->setMaType(maType);
    ciWidget->setFastSmoothing(fastSmoothing);
    ciWidget->setSlowSmoothing(slowSmoothing);
    connect(ciWidget, SIGNAL(propertiesChanged()), this, SLOT(setProperties()));

    return ciWidget;
}

void Trend::setProperties(void)
{
    maType = ciWidget->maType();
    fastSmoothing = ciWidget->fastSmoothing();
    slowSmoothing = ciWidget->slowSmoothing();

    buttonName.clear();
    buttonName.append("Trend ");
    buttonName.append(ciWidget->maTitle());
    setTitle(buttonName);

    Indicator::setProperties();
}

// kate: indent-mode cstyle; space-indent on; indent-width 4;
