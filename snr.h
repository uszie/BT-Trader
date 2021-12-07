#ifndef SNR_H
#define SNR_H

#include "bttrader.h"
#include <indicator.h>
#include "qwt_plot_spectrogram.h"
#include <QCheckBox>

class SNR : public Indicator
{
        Q_OBJECT

    public:
        SNR(Plot *plot, PriceChart *priceChart = strategie->currentPriceChart(), bool setupDialog = true, const QString &configGroup = "", Indicator::ProcessMode processMode = Indicator::View, bool drawCorona = true);
        virtual ~SNR(void);

        virtual QWidget *customIndicatorWidget(void);
        virtual void readSettingsPrivate(void);
        virtual void writeSettingsPrivate(void);

        PlotCurve *snrCurve;

    public slots:
        virtual void loadDefaultRules(void);
        virtual void setProperties(void);
        virtual void plotCurve(void);
        virtual void updateGUI(void);

    private:
        PlotSpectrogram *snrRasterData;
        bool i_drawCorona;
        QCheckBox *dcCheckBox;
};

#endif // SNR_H
