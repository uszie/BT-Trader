#include "snr.h"
#include <qwt_color_map.h>

SNR::SNR(Plot *plot, PriceChart *priceChart, bool setupDialog, const QString &configGroup, Indicator::ProcessMode processMode, bool drawCorona) :
    Indicator(plot, priceChart, setupDialog, configGroup, processMode)
{
    i_drawCorona = drawCorona;

    setColor(Qt::white);

    snrCurve = new PlotCurve(tr("SNR"), indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    QPen pen = snrCurve->pen();
    pen.setWidth(4);
    pen.setColor(Qt::white);
    snrCurve->setPen(pen);
    curveList.append(snrCurve);

    snrRasterData = new PlotSpectrogram();
    QwtLinearColorMap colorMap(Qt::cyan,  Qt::black, QwtColorMap::RGB);
    colorMap.addColorStop(0.3, QColor(Qt::cyan));
    snrRasterData->setColorMap(colorMap);
    snrRasterData->attach(indicatorPlot);

    indicatorPlot->setAxisScale(QwtPlot::yLeft, 0, 12);
    setIndicatorSourceComboBoxHidden(true);
}

SNR::~SNR(void)
{
}

void SNR::readSettingsPrivate(void)
{
    Indicator::readSettingsPrivate();

    i_drawCorona = settings.value("DrawCorona", i_drawCorona).toBool();
}

void SNR::writeSettingsPrivate(void)
{
    Indicator::writeSettingsPrivate();

    settings.setValue("Type", SNRIndicator);
    settings.setValue("DrawCorona", i_drawCorona);
}

void SNR::loadDefaultRules(void)
{
    clearRuleLists();

    Indicator::loadDefaultRules();
}

void SNR::plotCurve(void)
{
    Array price(indexPlot->periods);
    Array Avg(indexPlot->periods);
    Array Signal(indexPlot->periods);
    Array Noise(indexPlot->periods);
    Array mSNR(indexPlot->periods);
    Array HL(indexPlot->periods);
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
    Array DB(60);
    Array OldDB(60);
    Array OldRaster(60);
    MultiArray Raster(indexPlot->periods, Array(60));
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
        HL[i] = priceChart->averagePriceHigh->y(i) - priceChart->averagePriceLow->y(i);
    }

    for (int i = 6; i < indexPlot->periods; ++i)
    {
        HP[ i ] = 0.5 * (1 + alpha1) * (price[i] - price[i - 1]) + alpha1 * HP[i - 1];
        SmoothHP[i ] = (HP[i] + 2 * HP[ i - 1] + 3 * HP[i - 2] + 3 * HP[i - 3] + 2 * HP[i - 4] + HP[i - 5]) / 12;
        HL[i] = priceChart->averagePriceHigh->y(i) - priceChart->averagePriceLow->y(i);

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


            Avg[i] = 0.1 * price[i] + 0.9 * Avg[i - 1];

            if (Avg[i] != 0 && MaxAmpl > 0)
                Signal[i] = 0.2 * sqrt(MaxAmpl) + 0.9 * Signal[i - 1];
            else
                Signal[i] = Signal[i - 1];

            if (Avg[i] != 0)
                Noise[i] = 0.1 * median(HL, 5, i) + 0.9 * Noise[i - 1];

            if (Signal[i] != 0 && Noise[i] != 0)
                mSNR[i] = 20 * log(Signal[i] / Noise[i]) / log(10.0) + 3.5;

            if (mSNR[i] < 1)
                mSNR[i] = 0;

            if (mSNR[i] > 11)
                mSNR[i] = 10;

            mSNR[i] = 0.1 * mSNR[i];

            double Width = -0.4 * mSNR[i] + 0.2;
            if (mSNR[i] > 0.5)
                Width = 0;

            for (int n = 1; n < 51; n++)
            {
                Raster[i][n] = 20;
                if (Width > 0)
                {
                    if (n < round(50 * mSNR[i]))
                        Raster[i][n] = 0.5 * (pow((20 * mSNR[i] - 0.4 * n) / Width, 0.8) + OldRaster[n]);

                    if (n > round(50 * mSNR[i]) && (0.4 * n - 20 * mSNR[i]) / Width > 1)
                        Raster[i][n] = 0.5 * (pow((-20 * mSNR[i] + 0.4 * n) / Width, 0.8) + OldRaster[n]);
                }

                if (n == round(50 * mSNR[i]))
                    Raster[i][n] = 0.5 * OldRaster[n];

                if (Raster[i][n] < 0)
                    Raster[i][n] = 0;

                if (Raster[i][n] > 20)
                    Raster[i][n] = 20;

                if (mSNR[i] > 0.5)
                    Raster[i][n] = 20;

                OldRaster[n] = Raster[i][n];
            }

            mSNR[i] = (10 * mSNR[i] + 1);
        }
    }

    if (i_drawCorona && indexPlot->periods > 0)
    {
        QwtDoubleRect rect;
        rect.setBottom(12);
        rect.setTop(0);
        rect.setLeft(priceChart->averagePriceClose->x(0));
        rect.setRight(priceChart->averagePriceClose->x(priceChart->averagePriceClose->dataSize() - 1) + 1);
        snrRasterData->attach(indicatorPlot);
        snrRasterData->setData(RasterData(Raster, rect, 20, 1, 50));
    }
    else
    {
        snrRasterData->detach();
        snrRasterData->setData(RasterData(MultiArray(), QRect(), 20, 1, 50));
    }

    QVector<QPointF> SNRData;
    QPointF point;
    for (int i = 0; i < indexPlot->periods; ++i)
    {
        point.setX(priceChart->averagePriceClose->x(i));
        point.setY(mSNR[ i ]);
        SNRData.append(point);
    }

    snrCurve->setData(SNRData);

    indicatorPlot->setXAxisScaleToCurve(*snrCurve);

    Indicator::plotCurve();
}

void SNR::updateGUI(void)
{
    bool block = blockSettingsGUISignals();
    setBlockSettingsGUISignals(true);

    dcCheckBox->setChecked(i_drawCorona);

    setBlockSettingsGUISignals(block);

    Indicator::updateGUI();
}

QWidget *SNR::customIndicatorWidget(void)
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
    layout->setWidget(1, QFormLayout::SpanningRole, line);
    layout->addRow(tr("Draw corona"), dcCheckBox);

    setTitle(tr("SNR"));

    return ciWidget;
}

void SNR::setProperties()
{
    i_drawCorona = dcCheckBox->isChecked();
    if (i_drawCorona)
    {
        snrRasterData->attach(indicatorPlot);
    }
    else
    {
        snrRasterData->detach();
    }

    setTitle("SNR");

    Indicator::setProperties();
}
