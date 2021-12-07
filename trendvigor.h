#ifndef TRENDVIGOR_H
#define TRENDVIGOR_H

#include <indicator.h>

class TrendVigor : public Indicator
{
        Q_OBJECT

    public:
        TrendVigor(Plot *plot, PriceChart *priceChart = strategie->currentPriceChart(), bool setupDialog = true, const QString &configGroup = "", Indicator::ProcessMode processMode = Indicator::View, bool drawCorona = true);
        virtual ~TrendVigor(void);

        virtual QWidget *customIndicatorWidget(void);
        virtual void readSettingsPrivate(void);
        virtual void writeSettingsPrivate(void);

        PlotCurve *tvCurve;

    public slots:
        virtual void loadDefaultRules(void);
        virtual void setProperties(void);
        virtual void plotCurve(void);
        virtual void updateGUI(void);

    private:
        PlotSpectrogram *tvRasterData;
        bool i_drawCorona;
        QCheckBox *dcCheckBox;
};

#endif // TRENDVIGOR_H
