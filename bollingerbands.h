#ifndef BOLLINGERBANDS_H
#define BOLLINGERBANDS_H

#include <indicator.h>

class BollingerBands : public Indicator
{
        Q_OBJECT

    public:
        BollingerBands(Plot *plot, PriceChart *priceChart = strategie->currentPriceChart(), bool setupDialog = true, const QString &configGroup = "", Indicator::ProcessMode processMode = Indicator::View);
        ~BollingerBands();

        virtual QWidget *customIndicatorWidget(void);
        virtual void readSettingsPrivate(void);
        virtual void writeSettingsPrivate(void);


    public slots:
        virtual void loadDefaultRules(void);
        virtual void plotCurve(void);
        virtual void setProperties(void);
        virtual void updateGUI(void);

    private:
        MovingAverageUI *maForm;
        MovingAverageOptions options;
        PlotCurve *tbCurve;
        PlotCurve *bbCurve;
        PlotCurve *maCurve;
        int stdDeviation;
};

#endif // BOLLINGERBANDS_H
