#ifndef RSI_H
#define RSI_H

#include "indicator.h"

class RSI : public Indicator
{
        Q_OBJECT

    public:
        RSI(Plot *plot, PriceChart *priceChart = strategie->currentPriceChart(), bool setupDialog = true, const QString &configGroup = "", Indicator::ProcessMode processMode = Indicator::View);
        virtual ~RSI(void);

        virtual QWidget *customIndicatorWidget(void);
        virtual void readSettingsPrivate(void);
        virtual void writeSettingsPrivate(void);

    public slots:
        virtual void loadDefaultRules(void);
        virtual void setProperties(void);
        virtual void plotCurve(void);
        virtual void updateGUI(void);

    private:
        QVector<QPointF> riseData;
        QVector<QPointF> fallData;
        QVector<QPointF> riseCurve;
        QVector<QPointF> fallCurve;
        QVector<QPointF> rsiData;
        MovingAverageUI *rsiForm;
        MovingAverageOptions rsiOptions;
        PlotCurve *rsiCurve;
        MovingAverageUI *maForm;
        MovingAverageOptions maOptions;
        PlotCurve *maCurve;
        PlotCurve *erCurve;
};

#endif // RSI_H
