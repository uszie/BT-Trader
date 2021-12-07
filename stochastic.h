#ifndef STOCHASTIC_H
#define STOCHASTIC_H

#include <indicator.h>

class Stochastic : public Indicator
{
        Q_OBJECT

    public:
        Stochastic(Plot *plot, PriceChart *priceChart = strategie->currentPriceChart(), bool setupDialog = true, const QString &configGroup = "", Indicator::ProcessMode processMode = Indicator::View);
        virtual ~Stochastic();

        virtual QWidget *customIndicatorWidget(void);
        virtual void readSettingsPrivate(void);
        virtual void writeSettingsPrivate(void);

    public slots:
        virtual void loadDefaultRules(void);
        virtual void setProperties(void);
        virtual void plotCurve(void);
        virtual void updateGUI(void);

    private:
        MovingAverageUI *KUI;
        MovingAverageOptions KOptions;
        PlotCurve *KCurve;
        QVector<QPointF> KData;

        MovingAverageUI *DUI;
        MovingAverageOptions DOptions;
        PlotCurve *DCurve;
        QVector<QPointF> DData;

        PeriodsUI *stochasticPeriodsUI;
        PeriodOptions stochasticOptions;
        QVector<QPointF> StochasticData;
};

#endif // STOCHASTIC_H
