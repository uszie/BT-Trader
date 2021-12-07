#ifndef INSTANTANEOUSTREND_H
#define INSTANTANEOUSTREND_H

#include <indicator.h>


class InstantaneousTrend : public Indicator
{
        Q_OBJECT
    public:
        InstantaneousTrend(Plot *plot, PriceChart *priceChart = strategie->currentPriceChart(), bool setupDialog = true, const QString &configGroup = "", Indicator::ProcessMode processMode = Indicator::View);
        virtual ~InstantaneousTrend();

        virtual void writeSettingsPrivate(void);

    public slots:
        virtual void loadDefaultRules(void);
        virtual void plotCurve(void);

    private:
        PlotCurve *iTrend;
        PlotCurve *iTrendTrigger;
        QVector<QPointF> median;
        QVector<QPointF> it;
        QVector<QPointF> trigger;
};

#endif // INSTANTANEOUSTREND_H
