#ifndef LINE_H
#define LINE_H

#include "indicator.h"
#include "ui_line.h"

class Line: public Indicator
{
        Q_OBJECT

    public:
        enum Sticky
        {
            StickToFree,
            StickToOpen,
            StickToHigh,
            StickToLow,
            StickToClose
        };

        enum Mode
        {
            Free,
            Horizontal
        };

        Line(Plot *plot, PriceChart *priceChart = strategie->currentPriceChart(), Line::Mode lineMode = Line::Free, bool setupDialog = true, const QString &configGroup = "", Indicator::ProcessMode processMode = Indicator::View);
        ~Line(void);

        virtual bool eventFilter(QObject *object, QEvent *event);
        virtual QWidget *customIndicatorWidget(void);
        virtual void readSettingsPrivate(void);
        virtual void writeSettingsPrivate(void);

    public slots:
        virtual void loadDefaultRules(void);
        virtual void setProperties(void);
        virtual void plotCurve(void);
        virtual void updateGUI();
        void curveMoved(PlotCurve &curve);

    private:
        Line::Sticky stickTo;
        Line::Mode lineMode;
        bool sticky;
        bool startDrawing;
        bool endDrawing;
        bool trackerEnabled;
        QPointF startPoint;
        QPointF endPoint;
        QDateTime startDateTime;
        QDateTime endDateTime;
        PlotCurve *lineCurve;
        QString isin;
        int timeScale;
        Ui::LineUI *form;
};

#endif // LINE_H
