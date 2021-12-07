#ifndef PLOT_H
#define PLOT_H

#include "bttrader.h"
#include "strategie.h"
#include "canvaspicker.h"
#include "datetime.h"
#include "scrollzoomer.h"
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_scale_draw.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_price.h>
#include <qwt_price_move.h>
#include <qwt_legend.h>

#define AXIS_STEP_SIZE 0

class QwtPlotGrid;
class Strategie;
class PriceChart;
class Plot;
class CanvasPicker;


class Tracker: public QwtPlotPicker
{
        Q_OBJECT
    public:
        Tracker(int xAxis, int yAxis, QwtPlotCanvas *canvas);

        bool enabled(void) const;
        virtual bool eventFilter(QObject *object, QEvent *event);

    public slots:
        void setEnabled(bool enabled);
        void slotUpdateDisplay(void);

    signals:
        void movedFromTo(const QPoint &from, const QPoint &to);

    private slots:
        void createFromToMoveEvent(const QPoint &point);

    private:
        QPoint previousPoint;
        bool trackerEnabled;

    public:
        int releaseMouseGrab;
        double xClicked;
        QMenu *popMenu;
};

class PlotPrice : public QwtPlotPrice
{
    public:
        PlotPrice(const QString &title = QString::null);
        ~PlotPrice(void);
        virtual void drawSeries(QPainter *, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QRect &canvasRect, int from, int to) const;
        virtual void draw(QPainter *painter, const QwtScaleMap &xMap, const QwtScaleMap &yMap, int from, int to) const;
};

class PlotCurve: public QwtPlotCurve
{
    public:
        enum ProcessMode
        {
            Data,
            View
        };

        enum Style
        {
            Curve,
            Line
        };

        PlotCurve(const QString &title, Plot *plot, PlotCurve::ProcessMode processMode, Style style = PlotCurve::Curve);
        ~PlotCurve();

        Plot *pricePlot(void) const;
        PlotCurve::ProcessMode processMode() const;
        void setProcessMode(PlotCurve::ProcessMode mode);
        double plotOffset(void) const;
        void setPlotOffset(double offset);
        void setData(const QVector<QPointF> &data);
        void appendData(const QPointF &point);
        void replaceData(double x, const QPointF &point);
        void clearData(void);
        void setXAxisStartPoint(double x);
        int dataSize(void) const;
        double x(int i) const;
        double y(int i) const;
        int indexOffX(double x) const;
        const QVector<QPointF> &data(void) const;
        const float *fastData(void) const;
        virtual void drawSeries(QPainter *painter, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QRect &canvasRect, int from, int to) const;
        virtual void itemChanged();

    private:
        class PrivateData;
        PrivateData *privateData;
};

class PlotMCurve//: public QwtPlotPrice
{
    public:
        PlotMCurve(const QString &title, Plot *Plot);
        ~PlotMCurve();

        void setData(const QwtArray<QwtPriceMove> &data);
        void appendData(const QwtPriceMove &price);
        void replaceData(double x, const QwtPriceMove &price);
        void clearData(void);
        void setXAxisStartPoint(double x);
        double x(int i);
        QwtPriceMove y(int i);
        int findAddressForX(double x);
        QwtArray<QwtPriceMove> &data(void);

        PlotPrice *priceCurve;

    private:
        QwtArray<QwtPriceMove> p_Data;
        Plot *indexPlot;
};

class TimeScaleDraw: public QwtScaleDraw
{
    public:
        TimeScaleDraw(Plot *plot);
        virtual QwtText label(double value) const;

    private:
        Plot *pricePlot;
};

class Legend : public QwtLegend
{
    public:
        Legend(QWidget *parent = 0);
        virtual ~Legend(void);

    protected:
        virtual bool eventFilter(QObject *object, QEvent *event);

    private:
        void adjustGeometry(void);
};


class Plot : public QwtPlot
{
        Q_OBJECT
    public:
        Plot(QWidget *parent = 0);
        virtual ~Plot(void);

        CanvasPicker *picker(void) const;
        Tracker *tracker(void) const;
        ScrollZoomer *zoomer(void) const;
        bool autoZoomEnabled(void) const;
        const QString &timeScaleFormat(void) const;
        void setTimeScaleFormat(const QString &format);
        const QVector<QDateTime> &timeScaleData() const;
        void setTimeScaleData(const QVector<QDateTime> &data);
        void appendTimeScaleData(const QDateTime &date);

    public slots:
        void setPeriods(int count);
        void setXAxisScaleToCurve(const PlotCurve &curve);
        int findAddressForX(double x);
        double findXForDateTime(QDateTime &dateTime);
        double findXForDate(QDate &date);
        void zoomIn(void);
        void zoomIn(int factor);
        void zoomOut(void);
        void zoomGoto(double preferedPeriodsInZoom, double x1, double x2);
        void zoomReset(void);
        void magnifieYAxis(void);
        void setAutoZoomEnabled(bool enabled);
        bool plottingDisabled(void);
        void setPlottingDisabled(bool enabled);
        virtual void replot(void);
        virtual void scheduleReplot(void);

    signals:
        void zoomedIn(void);
        void zoomedIn(int factor);
        void zoomedOut(void);
        void zoomedGoto(double preferedPeriodsInZoom, double x1, double x2);
        void zoomedReset(void);
        void periodsChanged(int count);

    public:
        int periods;


    private slots:
        void showCurvePropertiesDialog(QwtPlotItem *plotItem);
        void setCurveProperties(void);

    private:
        class PrivateData;
        PrivateData *privateData;
};



#endif // PLOT_H
