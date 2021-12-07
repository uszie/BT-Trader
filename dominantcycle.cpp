#include "dominantcycle.h"
#include <qwt_symbol.h>
#include <qwt_legend.h>
#include <qwt_color_map.h>

DominantCycle::DominantCycle(Plot *plot, PriceChart *priceChart, bool setupDialog, const QString &configGroup, Indicator::ProcessMode processMode, bool drawCorona) :
    Indicator(plot, priceChart, setupDialog, configGroup, processMode)
{
    i_drawCorona = drawCorona;

    setColor(Qt::yellow);

    dcpCurve = new PlotCurve(tr("Dominant Cycle"), indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    QPen pen = dcpCurve->pen();
    pen.setWidth(4);
    pen.setColor(Qt::yellow);
    dcpCurve->setPen(pen);
    curveList.append(dcpCurve);

    dcRasterData = new PlotSpectrogram();
    QwtLinearColorMap colorMap(Qt::yellow,  Qt::black, QwtColorMap::RGB);
    colorMap.addColorStop(0.3, QColor(Qt::red));
    dcRasterData->setColorMap(colorMap);
    dcRasterData->attach(indicatorPlot);

    indicatorPlot->setAxisScale(QwtPlot::yLeft, 6, 30);
    setIndicatorSourceComboBoxHidden(true);
}

DominantCycle::~DominantCycle(void)
{
}

void DominantCycle::readSettingsPrivate(void)
{
    Indicator::readSettingsPrivate();

    i_drawCorona = settings.value("DrawCorona", i_drawCorona).toBool();
}

void DominantCycle::writeSettingsPrivate(void)
{
    Indicator::writeSettingsPrivate();

    settings.setValue("Type", DominantCycleIndicator);
    settings.setValue("DrawCorona", i_drawCorona);
}

void DominantCycle::loadDefaultRules(void)
{
    clearRuleLists();

    Indicator::loadDefaultRules();
}

double Median(Array &price, int per, int bar)
{
    double temp;
    double temparray[ per ];
    for (int k = per - 1; k >= 0; k--)
    {
        temparray[ k ] = price[bar + k - (per - 1) ];
    }

    temp = 0.0;
    for (int k = per - 1; k > 0; k--)
    {
        for (int j = per - 1; j > 0; j--)
        {
            if (temparray[ j - 1 ] > temparray[ j ])
            {
                temp = temparray[ j - 1 ];
                temparray[ j - 1 ] = temparray[ j ];
                temparray[ j ] = temp;
            }
        }
    }

    return temparray[per - 1 - (per / 2) ];
}

void DominantCycle::plotCurve(void)
{
    Array price(indexPlot->periods);
    Array DC(indexPlot->periods);
    Array HP(indexPlot->periods);
    Array SmoothHP(indexPlot->periods);
    Array mDomCyc(indexPlot->periods);
    Array I(60);
    Array OldI(60);
    Array OlderI(60);
    Array Q(60);
    Array OldQ(60);
    Array OlderQ(60);
    Array Real(60);
    Array OldReal(60);
    Array OlderReal(60);
    Array Imag(60);
    Array OldImag(60);
    Array OlderImag(60);
    Array Ampl(60);
    MultiArray DB(indexPlot->periods, Array(60));
    Array OldDB(60);
    double delta;
    double beta;
    double gamma;
    double alpha;
    double alpha1;
    double MaxAmpl;
    double Num;
    double Denom;
    double pi = 3.14159265359;
    double dB;

    if (indexPlot->periods <= 0)
        Indicator::plotCurve();

    calculateMedianCurve(priceChart->averagePriceLow->data(), priceChart->averagePriceHigh->data(), price);

    alpha1 = (1 - sin(2 * pi / 30)) / cos(2 * pi / 30);

    for (int i = 1; i < 6 && i < indexPlot->periods; ++i)
    {
        SmoothHP[i] = price[i] - price[i - 1];
    }

    for (int i = 6; i < indexPlot->periods; ++i)
    {
        HP[ i ] = 0.5 * (1 + alpha1) * (price[i] - price[i - 1]) + alpha1 * HP[i - 1];
        SmoothHP[i ] = (HP[i] + 2 * HP[ i - 1] + 3 * HP[i - 2] + 3 * HP[i - 3] + 2 * HP[i - 4] + HP[i - 5]) / 12;

        delta = -0.15 * (i + 1) + 0.5;
        if (delta < 0.1)
            delta = 0.1;

        if (i > 11)
        {
            for (int n = 11; n < 60; n++)
            {
                beta = cos(4 * pi / (n + 1));
                gamma = 1.0 / cos(8 * pi * delta / (n + 1));
                alpha = gamma - sqrt(gamma * gamma - 1);

                Q[n] = ((n + 1) / 4 / pi) * (SmoothHP[i] - SmoothHP[i - 1]);
                I[n] = SmoothHP[i];
                Real[n] = 0.5 * (1 - alpha) * (I[n] - OlderI[n]) + beta * (1 + alpha) * OldReal[n] - alpha * OlderReal[n];
                Imag[n] = 0.5 * (1 - alpha) * (Q[n] - OlderQ[n]) + beta * (1 + alpha) * OldImag[n] - alpha * OlderImag[n];
                Ampl[n] = (Real[n] * Real[n] + Imag[n] * Imag[n]);
            }

            for (int n = 11; n < 60; n++)
            {
                OlderI[n] = OldI[n];
                OldI[n] = I[n];
                OlderQ[n] = OldQ[n];
                OldQ[n] = Q[n];
                OlderReal[n] = OldReal[n];
                OldReal[n] = Real[n];
                OlderImag[n] = OldImag[n];
                OldImag[n] = Imag[n];
                OldDB[n] = DB[i - 1][n];
            }

            MaxAmpl = Ampl[ 11 ];

            for (int n = 11; n < 60; ++n)
            {
                if (Ampl[n] > MaxAmpl)
                {
                    MaxAmpl = Ampl[n];

                }
            }

            for (int n = 11; n < 60; ++n)
            {
                if (MaxAmpl != 0)
                {
                    if (Ampl[n] / MaxAmpl > 0)
                        dB = -10.0 * log(0.01 / (1 - 0.99 * Ampl[n] / MaxAmpl)) / log(10.0);
                }

                DB[i][n] = 0.33 * dB + 0.67 * OldDB[n];
                if (DB[i][n] > 20)
                    DB[i][n] = 20;
            }

            Num = 0;
            Denom = 0;
            for (int n = 11; n < 60; n++)
            {
                if (DB[i][n] < 7)
                {
                    Num = Num + (n + 1) * (20 - DB[i][n]);
                    Denom = Denom + (20 - DB[i][n]);
                }

                if (Denom != 0)
                    DC[i] = 0.5 * Num / Denom;
                else
                    DC[i] = DC[i - 1];
            }

            mDomCyc[i] = median(DC, 5, i);
        }

    }

    if (i_drawCorona && indexPlot->periods > 0)
    {
        QwtDoubleRect rect;
        rect.setBottom(30);
        rect.setTop(6);
        rect.setLeft(priceChart->averagePriceClose->x(0));
        rect.setRight(priceChart->averagePriceClose->x(priceChart->averagePriceClose->dataSize() - 1) + 1);
        dcRasterData->attach(indicatorPlot);
        dcRasterData->setData(RasterData(DB, rect, 20, 11, 59));
    }
    else
    {
        dcRasterData->detach();
        dcRasterData->setData(RasterData(MultiArray(), QRect(), 20, 11, 59));
    }

    QVector<QPointF> DominantCycleData;
    for (int i = 0; i < indexPlot->periods; ++i)
        DominantCycleData.append(QPointF(priceChart->averagePriceClose->x(i), mDomCyc[i]));

    dcpCurve->setData(DominantCycleData);

    setXAxisScaleToCurve(*indicatorSourceCurve);

    Indicator::plotCurve();
}

void DominantCycle::updateGUI(void)
{
    bool block = blockSettingsGUISignals();
    setBlockSettingsGUISignals(true);

    dcCheckBox->setChecked(i_drawCorona);

    setBlockSettingsGUISignals(block);

    Indicator::updateGUI();
}

QWidget *DominantCycle::customIndicatorWidget(void)
{
    dcCheckBox = new QCheckBox;
    dcCheckBox->setChecked(i_drawCorona);
    connect(dcCheckBox, SIGNAL(toggled(bool)), this, SLOT(setProperties()));

    QWidget *ciWidget = new QWidget;
    QFormLayout *layout = new QFormLayout;
    ciWidget->setLayout(layout);
    QFrame *line = new QFrame;
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->setWidget(0, QFormLayout::SpanningRole, new QLabel);
    layout->setWidget(1, QFormLayout::SpanningRole, line);;
    layout->addRow(tr("Draw corona"), dcCheckBox);

    setTitle(tr("Dominant Cycle"));

    return ciWidget;
}

void DominantCycle::setProperties()
{
    i_drawCorona = dcCheckBox->isChecked();
    if (i_drawCorona)
    {
        dcRasterData->attach(indicatorPlot);
    }
    else
    {
        dcRasterData->detach();
    }

    setTitle(tr("Dominant Cycle"));

    Indicator::setProperties();
}
