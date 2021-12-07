#ifndef MOVINGAVERAGE_H
#define MOVINGAVERAGE_H

#include "indicator.h"

class PlotCurve;

class MovingAverageCurve: public Indicator
{
        Q_OBJECT

    public:
        MovingAverageCurve(Plot *plot, PriceChart *priceChart = strategie->currentPriceChart(), bool setupDialog = true, const QString &configGroup = "", Indicator::ProcessMode processMode = Indicator::View);
        ~MovingAverageCurve(void);

        virtual QWidget *customIndicatorWidget(void);
        virtual void readSettingsPrivate(void);
        virtual void writeSettingsPrivate(void);

    public slots:
        virtual void loadDefaultRules(void);
        virtual void setProperties(void);
        virtual void plotCurve(void);
        virtual void updateGUI(void);

    private:
        PlotCurve *curve;
        MovingAverageUI *ciWidget;
        MovingAverageOptions options;
};

#endif // MOVINGAVERAGE_H
