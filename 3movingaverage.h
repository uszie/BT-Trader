#ifndef THREEMOVINGAVERAGE_H
#define THREEMOVINGAVERAGE_H

#include "indicator.h"

class PlotCurve;

class ThreeMovingAverageCurve: public Indicator
{
        Q_OBJECT

    public:
        ThreeMovingAverageCurve(Plot *plot, PriceChart *priceChart = strategie->currentPriceChart(), bool setupDialog = true, const QString &configGroup = "", Indicator::ProcessMode processMode = Indicator::View);
        ~ThreeMovingAverageCurve(void);

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
        MovingAverageOptions slowOptions;
        MovingAverageUI *fastForm;
        MovingAverageUI *mediumForm;
        MovingAverageUI *slowForm;
        PlotCurve *fastCurve;
        PlotCurve *mediumCurve;
        PlotCurve *slowCurve;
};

#endif // TWOMOVINGAVERAGE_H
