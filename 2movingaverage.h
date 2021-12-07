#ifndef TWOMOVINGAVERAGE_H
#define TWOMOVINGAVERAGE_H

#include "indicator.h"

class PlotCurve;

class TwoMovingAverageCurve: public Indicator
{
        Q_OBJECT

    public:
        TwoMovingAverageCurve(Plot *plot, PriceChart *priceChart = strategie->currentPriceChart(), bool setupDialog = true, const QString &configGroup = "", Indicator::ProcessMode processMode = Indicator::View);
        ~TwoMovingAverageCurve(void);

        virtual QWidget *customIndicatorWidget(void);
        virtual void readSettingsPrivate(void);
        virtual void writeSettingsPrivate(void);

    public slots:
        virtual void loadDefaultRules(void);
        virtual void setProperties(void);
        virtual void plotCurve(void);
        virtual void updateGUI(void);


    private:
        MovingAverageOptions fastOptions;
        MovingAverageOptions mediumOptions;
        MovingAverageUI *fastForm;
        MovingAverageUI *mediumForm;
        PlotCurve *fastCurve;
        PlotCurve *mediumCurve;
};

#endif // TWOMOVINGAVERAGE_H
