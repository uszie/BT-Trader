#ifndef TREND_H
#define TREND_H

#include "ui_trend.h"
#include "indicator.h"

class PlotCurve;

class Trend: public Indicator
{
        Q_OBJECT

    public:
        Trend(Plot *plot, PriceChart *priceChart = strategie->currentPriceChart(), bool setupDialog = true, const QString &configGroup = "", Indicator::ProcessMode processMode = Indicator::View);
        ~Trend(void);

        virtual QWidget *customIndicatorWidget(void);
        virtual void readSettingsPrivate(void);
        virtual void writeSettingsPrivate(void);

        PlotCurve *ma3;
        PlotCurve *ma5;
        PlotCurve *ma8;
        PlotCurve *ma10;
        PlotCurve *ma12;
        PlotCurve *ma15;

        PlotCurve *ma30;
        PlotCurve *ma35;
        PlotCurve *ma40;
        PlotCurve *ma45;
        PlotCurve *ma50;

        PlotCurve *ma60;

        PlotCurve *ma70;
        PlotCurve *ma80;
        PlotCurve *ma90;
        PlotCurve *ma100;
        PlotCurve *ma120;
        PlotCurve *TR;
        PlotCurve *ATR;
        PlotCurve *stTrend;
        PlotCurve *mtTrend;
        PlotCurve *ltTrend;

        PlotSpectrogram *d_spectrogram;
        MovingAverageUI *ciWidget;
        Ui::TrendUI *form;
        int maType;
        int fastSmoothing;
        int slowSmoothing;

    public slots:
        virtual void loadDefaultRules(void);
        virtual void setProperties(void);
        virtual void plotCurve(void);
};

#endif // TREND_H
