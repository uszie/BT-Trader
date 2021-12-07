#ifndef PRICECHANNEL_H
#define PRICECHANNEL_H

#include <indicator.h>

class PriceChannel : public Indicator
{

    public:
        PriceChannel(Plot *plot, PriceChart *priceChart = strategie->currentPriceChart(), bool setupDialog = true, const QString &configGroup = "", Indicator::ProcessMode processMode = Indicator::View);
        virtual ~PriceChannel();

        virtual QWidget *customIndicatorWidget(void);
        virtual void readSettingsPrivate(void);
        virtual void writeSettingsPrivate(void);

    public slots:
        virtual void loadDefaultRules(void);
        virtual void plotCurve(void);
        virtual void setProperties(void);
        virtual void updateGUI(void);

    private:
        PlotCurve *tbCurve;
        PlotCurve *bbCurve;
        PeriodsUI *ciWidget;
        PeriodOptions options;
};

#endif // PRICECHANNEL_H
