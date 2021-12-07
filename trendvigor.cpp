#include "trendvigor.h"
#include <qwt_color_map.h>

TrendVigor::TrendVigor(Plot *plot, PriceChart *priceChart, bool setupDialog, const QString &configGroup, Indicator::ProcessMode processMode, bool drawCorona) :
    Indicator(plot, priceChart, setupDialog, configGroup, processMode)
{
    i_drawCorona = drawCorona;

    setColor(QColor(Qt::blue).lighter());

    tvCurve = new PlotCurve(tr("Trend Vigor"), indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    QPen pen = tvCurve->pen();
    pen.setWidth(4);
    pen.setColor(QColor(Qt::blue).lighter());
    tvCurve->setPen(pen);
    curveList.append(tvCurve);

    tvRasterData = new PlotSpectrogram();
    QwtLinearColorMap colorMap(Qt::blue,  Qt::black, QwtColorMap::RGB);
    colorMap.addColorStop(0.1, QColor(Qt::blue));
    tvRasterData->setColorMap(colorMap);
    tvRasterData->attach(indicatorPlot);

    indicatorPlot->setAxisScale(QwtPlot::yLeft, -10, 10);
    setIndicatorSourceComboBoxHidden(true);
}

TrendVigor::~TrendVigor(void)
{
}

void TrendVigor::readSettingsPrivate(void)
{
    Indicator::readSettingsPrivate();

    i_drawCorona = settings.value("DrawCorona", i_drawCorona).toBool();
}

void TrendVigor::writeSettingsPrivate(void)
{
    Indicator::writeSettingsPrivate();

    settings.setValue("Type", TrendVigorIndicator);
    settings.setValue("DrawCorona", i_drawCorona);
}

void TrendVigor::loadDefaultRules(void)
{
    clearRuleLists();

    Indicator::loadDefaultRules();
}

void TrendVigor::plotCurve(void)
{
    double price[indexPlot->periods];
    double DC[indexPlot->periods];
    double HP[indexPlot->periods];
    double SmoothHP[indexPlot->periods];
    double mDomCyc[indexPlot->periods];
    double Ratio[indexPlot->periods];
    double mTV[indexPlot->periods];
    double IP[indexPlot->periods];

    double I[60];
    double OldI[60];
    double OlderI[60];
    double Q[60];
    double OldQ[60];
    double OlderQ[60];
    double Real[60];
    double OldReal[60];
    double OlderReal[60];
    double Imag[60];
    double OldImag[60];
    double OlderImag[60];
    double Ampl[60];
    double DB[60];
    double OldDB[60];
    double OldRaster[60];
    for (int i = 0; i < 60; i++)
    {
        I[i] = 0.0;
        OldI[i] = 0.0;
        OlderI[i] = 0.0;
        Q[i] = 0.0;
        OldQ[i] = 0.0;
        OlderQ[i] = 0.0;
        Real[i] = 0.0;
        OldReal[i] = 0.0;
        OlderReal[i] = 0.0;
        Imag[i] = 0.0;
        OldImag[i] = 0.0;
        OlderImag[i] = 0.0;
        Ampl[i] = 0.0;
        DB[i] = 0.0;
        OldDB[i] = 0.0;
        OldRaster[i] = 0.0;
    }

    MultiArray Raster(indexPlot->periods, Array(60));
    double delta;
    double delta1;
    double beta;
    double beta1;
    double gamma;
    double gamma1;
    double alpha;
    double alpha1;
    double alpha2;
    double MaxAmpl;
    double Ampl2;
    double Q1;
    double Trend;
    double Width;
    double Num;
    double Denom;
    double pi = 3.14159265359;
    double dB;
    int iDomCyc;
    int CycleStart;

    if (indexPlot->periods <= 0)
        Indicator::plotCurve();

    calculateMedianCurve(priceChart->averagePriceLow->fastData(), priceChart->averagePriceHigh->fastData(), price, indexPlot->periods);

    alpha1 = (1 - sin(2 * pi / 30)) / cos(2 * pi / 30);

    for (int i = 1; i < 6 && i < indexPlot->periods; ++i)
    {
        SmoothHP[i] = price[i] - price[i - 1];
    }

    for (int i = 6; i < indexPlot->periods; ++i)
    {
        HP[i] = 0.5 * (1 + alpha1) * (price[i] - price[i - 1]) + alpha1 * HP[i - 1];
        SmoothHP[i] = (HP[i] + 2 * HP[ i - 1] + 3 * HP[i - 2] + 3 * HP[i - 3] + 2 * HP[i - 4] + HP[i - 5]) / 12;

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
                OldDB[n] = DB[n];
            }

            MaxAmpl = Ampl[11];

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

                DB[n] = 0.33 * dB + 0.67 * OldDB[n];
                if (DB[n] > 20)
                    DB[n] = 20;
            }

            Num = 0;
            Denom = 0;
            for (int n = 11; n < 60; n++)
            {
                if (DB[n] < 7)
                {
                    Num = Num + (n + 1) * (20 - DB[n]);
                    Denom = Denom + (20 - DB[n]);
                }

                if (Denom != 0)
                    DC[i] = 0.5 * Num / Denom;
                else
                    DC[i] = DC[i - 1];
            }

            mDomCyc[i] = median(DC, 5, i);
            if (mDomCyc[i] < 6)
                mDomCyc[i] = 6;

            delta1 = 0.1;
            beta1 = cos(2 * pi / mDomCyc[i]);
            gamma1 = 1.0 / cos(4 * pi * delta1 / mDomCyc[i]);
            alpha2 = gamma1 - sqrt(gamma1 * gamma1 - 1);
            IP[i] = 0.5 * (1 - alpha2) * (price[i] - price[i - 2]) + beta1 * (1 + alpha2) * IP[i - 1] - alpha2 * IP[i - 2];
            Q1 = (mDomCyc[i] / 2 / pi) * (IP[i] - IP[i - 1]);
            Ampl2 = sqrt(IP[i] * IP[i] + Q1 * Q1);
            iDomCyc = round(mDomCyc[i]);
            CycleStart = i - iDomCyc - 1;
            if (CycleStart < 0)
                CycleStart = 0;

            Trend = price[i] - price[CycleStart];
            if (Trend != 0 && Ampl2 != 0)
                Ratio[i] = 0.33 * Trend / Ampl2 + 0.67 * Ratio[i - 1];

            if (Ratio[i] > 10)
                Ratio[i] = 10;

            if (Ratio[i] < -10)
                Ratio[i] = -10;

            mTV[i] = 0.05 * (Ratio[i] + 10);

            if (mTV[i] < 0.3 || mTV[i] > 0.7)
                Width = 0.01;

            if (mTV[i] >= 0.3 && mTV[i] < 0.5)
                Width = mTV[i] - 0.3;

            if (mTV[i] > 0.5 && mTV[i] <= 0.7)
                Width = -mTV[i] + 0.7;

            for (int n = 1; n < 51; n++)
            {
                Raster[i][n] = 20;
                if (n < round(50 * mTV[i]))
                    Raster[i][n] = 0.8 * (pow((20 * mTV[i] - 0.4 * n) / Width, 0.85) + 0.2 * OldRaster[n]);
                if (n > round(50 * mTV[i]))
                    Raster[i][n] = 0.8 * (pow((-20 * mTV[i] + 0.4 * n) / Width, 0.85) + 0.2 * OldRaster[n]);
                if (n == round(50 * mTV[i]))
                    Raster[i][n] = 0.5 * OldRaster[n];
                if (Raster[i][n] < 0)
                    Raster[i][n] = 0;
                if (Raster[i][n] > 20 || mTV[i] < 0.3 || mTV[i] > 0.7)
                    Raster[i][n] = 20;

                OldRaster[n] = Raster[i][n];
            }

            mTV[i] = (20 * mTV[i] - 10);
        }

    }

    if (i_drawCorona && indexPlot->periods > 0)
    {
        QwtDoubleRect rect;
        rect.setBottom(10);
        rect.setTop(-10);
        rect.setLeft(priceChart->averagePriceClose->x(0));
        rect.setRight(priceChart->averagePriceClose->x(priceChart->averagePriceClose->dataSize() - 1) + 1);
        tvRasterData->attach(indicatorPlot);
        tvRasterData->setData(RasterData(Raster, rect, 20, 1, 50));
    }
    else
    {
        tvRasterData->detach();
        tvRasterData->setData(RasterData(MultiArray(), QRect(), 20, 1, 50));
    }

    QVector<QPointF> TrendVigorData;
    QPointF point;
    for (int i = 0; i < indexPlot->periods; ++i)
    {
        point.setX(priceChart->averagePriceClose->x(i));
        point.setY(mTV[i]);
        TrendVigorData.append(point);
    }

    tvCurve->setData(TrendVigorData);

    setXAxisScaleToCurve(*indicatorSourceCurve);

    Indicator::plotCurve();
}

void TrendVigor::updateGUI(void)
{
    bool block = blockSettingsGUISignals();
    setBlockSettingsGUISignals(true);

    dcCheckBox->setChecked(i_drawCorona);

    setBlockSettingsGUISignals(block);

    Indicator::updateGUI();
}

QWidget *TrendVigor::customIndicatorWidget(void)
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

    setTitle(tr("Trend Vigor"));

    return ciWidget;
}

void TrendVigor::setProperties()
{
    i_drawCorona = dcCheckBox->isChecked();
    if (i_drawCorona)
    {
        tvRasterData->attach(indicatorPlot);
    }
    else
    {
        tvRasterData->detach();
    }

    setTitle(tr("Trend Vigor"));

    Indicator::setProperties();
}
