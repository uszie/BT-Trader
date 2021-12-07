#ifndef ADX_H
#define ADX_H

#include "indicator.h"

class ADX : public Indicator
{
        Q_OBJECT

    public:
        ADX(Plot *plot, PriceChart *priceChart = strategie->currentPriceChart(), bool setupDialog = true, const QString &configGroup = "", Indicator::ProcessMode processMode = Indicator::View);
        ~ADX(void);

    public:
        virtual QWidget *customIndicatorWidget(void);
        virtual void readSettingsPrivate(void);
        virtual void writeSettingsPrivate(void);

    public slots:
        virtual void loadDefaultRules(void);
        virtual void setProperties(void);
        virtual void plotCurve(void);
        virtual void updateGUI(void);

    private:
        PlotCurve *adxCurve;
        PlotCurve *plusDICurve;
        PlotCurve *minDICurve;
        QVector<QPointF> plusDM;
        QVector<QPointF> minDM;
        QVector<QPointF> plusADM;
        QVector<QPointF> minADM;
        QVector<QPointF> TR;
        QVector<QPointF> ATR;
        QVector<QPointF> DX;
        QVector<QPointF> _ADX;
        MovingAverageUI *ciWidget;
        MovingAverageOptions options;
};

#endif // ADX_H
