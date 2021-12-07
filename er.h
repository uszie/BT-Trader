#ifndef ER_H
#define ER_H

#include "indicator.h"

class ER : public Indicator
{
        Q_OBJECT

    public:
        ER(Plot *plot, PriceChart *priceChart = strategie->currentPriceChart(), bool setupDialog = true, const QString &configGroup = "", Indicator::ProcessMode processMode = Indicator::View);
        ~ER(void);

        virtual QWidget *customIndicatorWidget(void);
        virtual void readSettingsPrivate(void);
        virtual void writeSettingsPrivate(void);

    public slots:
        virtual void loadDefaultRules(void);
        virtual void setProperties(void);
        virtual void plotCurve(void);
        virtual void updateGUI(void);

    private:
        PeriodsUI *erForm;
        PeriodOptions erOptions;
        PlotCurve *erCurve;
        MovingAverageUI *maForm;
        MovingAverageOptions maOptions;
        PlotCurve *maCurve;
};

#endif // ER_H
