#ifndef VOLATILITY_H
#define VOLATILITY_H

#include "indicator.h"

class Volatility : public Indicator
{
        Q_OBJECT

    public:
        Volatility(Plot *plot, PriceChart *priceChart = strategie->currentPriceChart(), bool setupDialog = true, const QString &configGroup = "", Indicator::ProcessMode processMode = Indicator::View);
        ~Volatility(void);

        virtual void readSettingsPrivate(void);
        virtual void writeSettingsPrivate(void);
        virtual QWidget *customIndicatorWidget(void);

        PlotCurve *volatilityCurve;
        PeriodOptions options;
        PeriodsUI *ciWidget;

    public slots:
        virtual void loadDefaultRules(void);
        virtual void setProperties(void);
        virtual void plotCurve(void);
        virtual void updateGUI(void);
};

#endif // VOLATILITY_H
