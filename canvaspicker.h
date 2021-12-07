#ifndef CANVASPICKER_H
#define CANVASPICKER_H

#include "plot.h"
#include <qobject.h>
#include <qpoint.h>

class QPoint;
class QwtPlot;
class Plot;
class PlotCurve;

struct curveLink
{
    const PlotCurve *src;
    const PlotCurve *dest;
};

class CanvasPicker: public QObject
{
        Q_OBJECT
    public:
        CanvasPicker(Plot *plot);
        virtual bool eventFilter(QObject *, QEvent *);
        void linkCurves(const PlotCurve *src, const PlotCurve *dest);
        void select(const QPoint &);
        void unselect(void);
        PlotCurve *selectedCurve(void) { return d_selectedCurve; }

    public slots:
        void repositionLineCurves(void);

    signals:
        void curveMoved(PlotCurve &c);

    private:
        void move(const QPoint &);
        void moveBy(int dx, int dy);

        void release();

        void showCursor(bool enable);
        void shiftPointCursor(bool up);
        void shiftCurveCursor(bool up);

        Plot *plot()
        {
            return (Plot *)parent();
        }
        const Plot *plot() const
        {
            return (Plot *)parent();
        }

        PlotCurve *d_selectedCurve;
        const PlotCurve *stickCurve;
        int d_selectedPoint;
        QPoint d_startPoint;
        bool moveCurve;
        bool trackerEnabled;
        QList <struct curveLink *> curveLinks;
};

#endif
