#ifndef DOMINANTCYCLE_H
#define DOMINANTCYCLE_H

#include "bttrader.h"
#include "indicator.h"
#include "qwt_plot_spectrogram.h"
#include <QCheckBox>

class DominantCycle : public Indicator
{
        Q_OBJECT

    public:
        DominantCycle(Plot *plot, PriceChart *priceChart = strategie->currentPriceChart(), bool setupDialog = true, const QString &configGroup = "", Indicator::ProcessMode processMode = Indicator::View, bool drawCorona = true);
        virtual ~DominantCycle(void);

        virtual QWidget *customIndicatorWidget(void);
        virtual void readSettingsPrivate(void);
        virtual void writeSettingsPrivate(void);

        PlotCurve *dcpCurve;

    public slots:
        virtual void loadDefaultRules(void);
        virtual void setProperties(void);
        virtual void plotCurve(void);
        virtual void updateGUI(void);

    private:
        PlotSpectrogram *dcRasterData;
        bool i_drawCorona;
        QCheckBox *dcCheckBox;
};

#endif // DOMINANTCYCLE_H
