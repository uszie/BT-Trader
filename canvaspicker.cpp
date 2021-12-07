#include "canvaspicker.h"
#include "plot.h"
#include <qwt_plot.h>
#include <qwt_symbol.h>
#include <qwt_scale_map.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_scale_div.h>
#include <qwt_plot_directpainter.h>
#include <qapplication.h>
#include <qevent.h>
#include <qwhatsthis.h>
#include <qpainter.h>
#include <qline.h>

CanvasPicker::CanvasPicker(Plot *plot) :
    QObject(plot),
    d_selectedCurve(NULL),
    d_selectedPoint(-1)
{
    QwtPlotCanvas *canvas = plot->canvas();
    stickCurve = NULL;
    trackerEnabled = true;
    canvas->installEventFilter(this);
    canvas->setFocusPolicy(Qt::StrongFocus);
    canvas->setFocusIndicator(QwtPlotCanvas::ItemFocusIndicator);
    canvas->setFocus();

    shiftCurveCursor(true);
}

bool CanvasPicker::eventFilter(QObject *object, QEvent *e)
{
    if (object != plot()->canvas())
        return false;

    switch (e->type())
    {
        case QEvent::FocusIn:
            showCursor(true);
        case QEvent::FocusOut:
            showCursor(false);

        case QEvent::Paint:
        {
            QApplication::postEvent(this, new QEvent(QEvent::User));
            break;
        }
        case QEvent::MouseButtonPress:
        {
            select(((QMouseEvent *) e)->pos());
            return true;
        }
        case QEvent::MouseButtonRelease:
        {
            unselect();
            return true;
        }
        case QEvent::MouseMove:
        {
            move(((QMouseEvent *) e)->pos());

            return true;
        }

        default:
            break;
    }

    return QObject::eventFilter(object, e);
}

void CanvasPicker::linkCurves(const PlotCurve *src, const PlotCurve *dest)
{
    bool replace = false;
    int i;

    for (i = 0; i < curveLinks.size(); ++i)
    {
        if (curveLinks.at(i)->src == src)
        {
            if (curveLinks.at(i)->dest != dest)
            {
                replace = true;
                break;
            }

            return;
        }
    }

    if (replace)
        delete curveLinks.takeAt(i);

    struct curveLink *link = new struct curveLink;
    link->src = src;
    link->dest = dest;
    curveLinks.append(link);
}

void CanvasPicker::select(const QPoint &pos)
{
    PlotCurve *curve = NULL;
    double dist = 10e10;
    int index = -1;

    const QwtPlotItemList &itmList = plot()->itemList();
    for (QwtPlotItemIterator it = itmList.begin(); it != itmList.end(); ++it)
    {
        if ((*it)->rtti() == QwtPlotItem::Rtti_PlotCurve)
        {
            PlotCurve *c = dynamic_cast<PlotCurve *>(*it);
            if (!c)
                continue;

            if (c->dataSize() != 2)
                continue;

            double d;
            int idx = c->closestPoint(pos, &d);
            if (d < dist)
            {
                curve = c;
                index = idx;
                dist = d;
            }
        }
    }

    showCursor(false);
    d_selectedCurve = NULL;
    d_selectedPoint = -1;
    moveCurve = false;
    stickCurve = NULL;

    if (curve && dist < 10)
    {
        d_selectedCurve = curve;
        d_selectedPoint = index;
        d_startPoint = pos;
        showCursor(true);

        Plot *pricePlot = dynamic_cast<Plot *>(plot());
        if (pricePlot)
        {
            trackerEnabled = pricePlot->tracker()->enabled();
            pricePlot->tracker()->setEnabled(false);
        }

        return;
    }

    for (QwtPlotItemIterator it = itmList.begin(); it != itmList.end(); ++it)
    {
        if ((*it)->rtti() == QwtPlotItem::Rtti_PlotCurve)
        {
            PlotCurve *curve = dynamic_cast<PlotCurve *>(*it);
            if (!curve)
                continue;

            if (curve->dataSize() != 2)
                continue;

            QRect screenGeometry = QApplication::desktop()->screenGeometry();
            double xFactor = (plot()->axisScaleDiv(QwtPlot::xBottom)->range() / screenGeometry.width()) * 10;
            double yFactor = (plot()->axisScaleDiv(QwtPlot::yLeft)->range() / screenGeometry.height()) * 10;

            QLineF side1;
            QLineF side2;
            QLineF curveLine(QPointF(curve->x(0), curve->y(0)), QPointF(curve->x(1), curve->y(1)));
            qreal angle = curveLine.angle();
            if ((angle >= 225 && angle <= 315) || (angle >= 45 && angle <= 135))
            {
                side1 = curveLine.translated(QPointF(1 * xFactor, 0));
                side2 = curveLine.translated(QPointF(-1 * xFactor, 0));
            }
            else
            {
                side1 = curveLine.translated(QPointF(0, 1 * yFactor));
                side2 = curveLine.translated(QPointF(0, -1 * yFactor));
            }

            QPolygonF framePolygon;
            framePolygon << side1.p1();
            framePolygon << side1.p2();
            framePolygon << side2.p2();
            framePolygon << side2.p1();
            framePolygon << side1.p1();

            QPointF tPoint(plot()->invTransform(curve->xAxis(), pos.x()), plot()->invTransform(curve->yAxis(), pos.y()));
            if (framePolygon.containsPoint(tPoint, Qt::OddEvenFill))
            {
                d_selectedCurve = curve;
                d_selectedPoint = 1;
                d_startPoint = pos;
                moveCurve = true;
                showCursor(true);

                Plot *pricePlot = dynamic_cast<Plot *>(plot());
                if (pricePlot)
                {
                    trackerEnabled = pricePlot->tracker()->enabled();
                    pricePlot->tracker()->setEnabled(false);
                }

                return;
            }
        }
    }
}

void CanvasPicker::unselect(void)
{
    Plot *pricePlot = dynamic_cast<Plot *>(plot());
    if (pricePlot)
        pricePlot->tracker()->setEnabled(trackerEnabled);

    showCursor(false);
    d_selectedCurve = NULL;
    d_selectedPoint = -1;
    moveCurve = false;
    stickCurve = NULL;
}

void CanvasPicker::moveBy(int dx, int dy)
{
    if (dx == 0 && dy == 0)
        return;

    if (!d_selectedCurve)
        return;

    const int x = plot()->transform(d_selectedCurve->xAxis(), d_selectedCurve->x(d_selectedPoint)) + dx;
    const int y = plot()->transform(d_selectedCurve->yAxis(), d_selectedCurve->y(d_selectedPoint)) + dy;

    move(QPoint(x, y));
}

void CanvasPicker::move(const QPoint &pos)
{
    if (!d_selectedCurve)
        return;

    stickCurve = NULL;
    for (int i = 0; i < curveLinks.size(); ++i)
    {
        if (curveLinks.at(i)->src == d_selectedCurve)
            stickCurve = curveLinks.at(i)->dest;
    }

    QVector<QPointF> data(d_selectedCurve->dataSize());

    if (moveCurve)
    {
        double xMove = plot()->invTransform(d_selectedCurve->xAxis(), pos.x()) - plot()->invTransform(d_selectedCurve->xAxis(), d_startPoint.x()) ;
        double yMove = plot()->invTransform(d_selectedCurve->yAxis(), pos.y()) - plot()->invTransform(d_selectedCurve->yAxis(), d_startPoint.y()) ;
        if (stickCurve)
        {
            double xMoveNew = xMove;
            if (xMove > 0 && xMove < 0.2)
                xMoveNew = 0.5;
            if (xMove > -0.2 && xMove < 0)
                xMoveNew = -0.6;

            if (data[0].x() >= stickCurve->dataSize())
                data[0].setX(d_selectedCurve->x(0) + xMove);
            else
                data[0].setX(lround(d_selectedCurve->x(0) + xMoveNew));

            if (data[1].x() >= stickCurve->dataSize())
                data[1].setX(d_selectedCurve->x(1) + xMove);
            else
                data[1].setX(lround(d_selectedCurve->x(1) + xMoveNew));


            if (data[0].x() >= stickCurve->dataSize())
                data[0].setY(d_selectedCurve->y(0) + yMove);
            else
                data[0].setY(stickCurve->y(data[ 0 ].x()));

            if (data[1].x() >= stickCurve->dataSize())
                data[1].setY(d_selectedCurve->y(1) + yMove);
            else
                data[1].setY(stickCurve->y(data[ 1 ].x()));
        }
        else
        {
            data[0].setX(d_selectedCurve->x(0) + xMove);
            data[1].setX(d_selectedCurve->x(1) + xMove);
            data[0].setY(d_selectedCurve->y(0) + yMove);
            data[1].setY(d_selectedCurve->y(1) + yMove);
        }
    }
    else
    {
        for (int i = 0; i < (int) d_selectedCurve->dataSize(); i++)
        {
            if (i == d_selectedPoint)
            {
                if (stickCurve)
                {
                    if (data[i].x() >= stickCurve->dataSize())
                        data[i].setX(plot()->invTransform(d_selectedCurve->xAxis(), pos.x()));
                    else
                        data[i].setX(lround(plot()->invTransform(d_selectedCurve->xAxis(), pos.x())));

                    if (data[i].x() >= stickCurve->dataSize())
                        data[i].setY(plot()->invTransform(d_selectedCurve->yAxis(), pos.y()));
                    else
                        data[i].setY(stickCurve->y(data[ i ].x()));
                }
                else
                {
                    data[i].setX(plot()->invTransform(d_selectedCurve->xAxis(), pos.x()));
                    data[i].setY(plot()->invTransform(d_selectedCurve->yAxis(), pos.y()));
                }
            }
            else
            {
                data[i].setX(d_selectedCurve->x(i));
                data[i].setY(d_selectedCurve->y(i));
            }

        }
    }

    d_startPoint = pos;
    d_selectedCurve->setData(data);

    plot()->replot();
    showCursor(true);

    emit curveMoved(*d_selectedCurve);
}

void CanvasPicker::repositionLineCurves(void)
{
}

void CanvasPicker::showCursor(bool showIt)
{
    if (!d_selectedCurve)
        return;

    QwtSymbol symbol = d_selectedCurve->symbol();

    const QBrush brush = symbol.brush();
    if (showIt)
    {
        symbol.setBrush(symbol.brush().color().dark(150));
        symbol.setBrush(Qt::white);
    }

    const bool doReplot = plot()->autoReplot();

    plot()->setAutoReplot(false);

    QwtPlotDirectPainter directPainter;
    directPainter.drawSeries(d_selectedCurve, d_selectedPoint, d_selectedPoint);

    if (showIt)
        symbol.setBrush(brush);

    plot()->setAutoReplot(doReplot);
}

void CanvasPicker::shiftCurveCursor(bool up)
{
    QwtPlotItemIterator it;

    const QwtPlotItemList &itemList = plot()->itemList();

    QwtPlotItemList curveList;
    for (it = itemList.begin(); it != itemList.end(); ++it)
    {
        if ((*it)->rtti() == QwtPlotItem::Rtti_PlotCurve)
            curveList += *it;
    }
    if (curveList.isEmpty())
        return;

    it = curveList.begin();

    if (d_selectedCurve)
    {
        for (it = curveList.begin(); it != curveList.end(); ++it)
        {
            if (d_selectedCurve == *it)
                break;
        }
        if (it == curveList.end())
            it = curveList.begin();

        if (up)
        {
            ++it;
            if (it == curveList.end())
                it = curveList.begin();
        }
        else
        {
            if (it == curveList.begin())
                it = curveList.end();
            --it;
        }
    }

    showCursor(false);
    d_selectedPoint = 0;
    d_selectedCurve = (PlotCurve *) *it;
    showCursor(true);
}

void CanvasPicker::shiftPointCursor(bool up)
{
    if (!d_selectedCurve)
        return;

    int index = d_selectedPoint + (up ? 1 : -1);
    index = (index + d_selectedCurve->dataSize()) % d_selectedCurve->dataSize();

    if (index != d_selectedPoint)
    {
        showCursor(false);
        d_selectedPoint = index;
        showCursor(true);
    }
}
