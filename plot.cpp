#include "plot.h"
#include "main.h"
#include "scrollbar.h"
#include "scrollzoomer.h"
#include "transparentblurredpopup.h"
#include "ui_curveproperties.h"
#include <QMouseEvent>
#include <qwt_plot_picker.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_layout.h>
#include <qwt_scale_widget.h>
#include <qwt_plot_curve.h>
#include <qwt_symbol.h>
#include <qwt_curve_fitter.h>
#include <qwt_picker_machine.h>
#include <qwt_legend.h>
#include <qwt_legend_item.h>

#define AXIS_STEP_SIZE 0

PlotPrice::PlotPrice(const QString &title): QwtPlotPrice(title)
{
}

PlotPrice::~PlotPrice(void)
{
}

void PlotPrice::drawSeries(QPainter *, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QRect &canvasRect, int from, int to) const
{
    (void)xMap;
    (void)yMap;
    (void)canvasRect;
    (void)from;
    (void)to;
}

void PlotPrice::draw(QPainter *painter, const QwtScaleMap &xMap, const QwtScaleMap &yMap, int from, int to) const
{
    if (to < 0)
        to = dataSize() - 1;

    if (xMap.s2() < to)
        to = xMap.s2();

    from = xMap.s1();

    QwtPlotPrice::draw(painter, xMap, yMap, from, to);
}

PlotMCurve::PlotMCurve(const QString &title, Plot *plot)//: QwtPlotPrice( title )
{
    indexPlot = plot;
    priceCurve = new PlotPrice(title);
    priceCurve->setRenderHint(QwtPlotItem::RenderAntialiased, false);
    priceCurve->setZ(25);
    QPen pen = priceCurve->pen();
    pen.setColor(qApp->palette().color(QPalette::Text));
    pen.setWidth(1);
    priceCurve->setPen(pen);
    priceCurve->setUpBrush(Qt::green);
    priceCurve->setDownBrush(Qt::red);
    priceCurve->attach(indexPlot);
}

PlotMCurve::~PlotMCurve()
{
    priceCurve->detach();
    indexPlot->replot();
}

void PlotMCurve::setData(const QwtArray< QwtPriceMove > &data)
{
    p_Data = data;
    priceCurve->setData(p_Data);
}

void PlotMCurve::appendData(const QwtPriceMove &price)
{
    if (p_Data.size() == indexPlot->periods)
        p_Data.remove(0);

    p_Data.append(price);
    priceCurve->setData(p_Data);
}

void PlotMCurve::replaceData(double x, const QwtPriceMove &price)
{
    int address =  findAddressForX(x);
    if (address < 0)
        return;

    p_Data.replace(address, price);
    priceCurve->setData(p_Data);
}

void PlotMCurve::clearData(void)
{
    p_Data.clear();
    priceCurve->setData(p_Data);
}

void PlotMCurve::setXAxisStartPoint(double x)
{
    for (int i = 0; i < p_Data.size(); i++)
    {
        p_Data[ i ].interval.setInterval(x, x + 1);
        x += 1;
    }

    setData(p_Data);
}

double PlotMCurve::x(int i)
{
    return p_Data.at(i).interval.minValue();
}

QwtPriceMove PlotMCurve::y(int i)
{
    return p_Data.at(i);
}

QwtArray<QwtPriceMove> &PlotMCurve::data(void)
{
    return p_Data;
}

int PlotMCurve::findAddressForX(double x)
{
    if (priceCurve->dataSize() < 1)
        return -1;

    if (this->x(0) == x)
        return 0;
    else if (this->x(0) < x)
    {
        int address = x - this->x(0);
        if (address >= priceCurve->dataSize())
            return -1;
        else
            return x - this->x(0);
    }

    return -1;
}

class PlotCurve::PrivateData
{
    public:
        double offset;
        Plot *pricePlot;
        QVector<QPointF> pointData;
        float *floatData;
        PlotCurve::ProcessMode processMode;
        PlotCurve::Style style;
};

PlotCurve::PlotCurve(const QString &title, Plot *plot, ProcessMode processMode, PlotCurve::Style style) :
    QwtPlotCurve(title)
{
    privateData = new PrivateData;
    privateData->pricePlot = plot;
    privateData->floatData = NULL;
    privateData->style = style;
    setRenderHint(QwtPlotItem::RenderAntialiased, true);
    setPaintAttribute(QwtPlotCurve::ClipPolygons, true);
    setPaintAttribute(QwtPlotCurve::PaintFiltered, true);
    setZ(100);
    setProcessMode(processMode);
    setPlotOffset(0.8);
    QPen p = pen();
    p.setColor(Qt::transparent);
    p.setWidth(2);
    setPen(p);
}

PlotCurve::~PlotCurve()
{
    delete privateData;
}

void PlotCurve::drawSeries(QPainter *painter, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QRect &canvasRect, int from, int to) const
{
    QwtScaleMap newMap = xMap;
    newMap.setScaleInterval(newMap.s1() - privateData->offset, newMap.s2() - privateData->offset);
    QwtPlotCurve::drawSeries(painter, newMap, yMap, canvasRect, from, to);
}

void PlotCurve::setData(const QVector< QPointF > &data)
{
    privateData->pointData = data;
    if (privateData->floatData)
        _mm_free(privateData->floatData);

    privateData->floatData = (float *) _mm_malloc(sizeof(float) * privateData->pricePlot->periods, 16);

    if (privateData->style == PlotCurve::Curve)
    {
        int size = privateData->pointData.size();
        int offset = privateData->pricePlot->periods - size;
        for (int i = 0; i < offset; ++i)
            privateData->floatData[i] = IGNORE_VALUE;

        for (int i = offset; i < privateData->pricePlot->periods; i++)
            privateData->floatData[i] = privateData->pointData.at(i - offset).y();
    }
    else
    {
        QPointF startPoint = (data.at(0).x() < data.at(1).x()) ? data.at(0) : data.at(1);
        QPointF endPoint = (data.at(0).x() < data.at(1).x()) ? data.at(1) : data.at(0);
        QLineF line(startPoint, endPoint);
        qreal range = endPoint.x() - startPoint.x();
        for (int i = 0; i < privateData->pricePlot->periods; i++)
        {
            if (i < startPoint.x() || i > endPoint.x() || qFuzzyCompare(startPoint.x(), endPoint.x()))
                privateData->floatData[i] = IGNORE_VALUE;
            else
                privateData->floatData[i] = line.pointAt((static_cast<qreal>(i) - startPoint.x()) / range).y();
        }
    }

    if (processMode() != PlotCurve::View)
        return;

    setSamples(privateData->pointData);
}

void PlotCurve::appendData(const QPointF &point)
{
    if (privateData->pointData.size() == privateData->pricePlot->periods)
        privateData->pointData.remove(0);

    privateData->pointData.append(point);

    if (privateData->floatData)
        _mm_free(privateData->floatData);
    privateData->floatData = (float *) _mm_malloc(sizeof(float) * privateData->pricePlot->periods, 16);

    int size = privateData->pointData.size();
    int offset = privateData->pricePlot->periods - size;
    for (int i = 0; i < offset; ++i)
        privateData->floatData[i] = IGNORE_VALUE;

    for (int i = offset; i < privateData->pricePlot->periods; ++i)
        privateData->floatData[i] = privateData->pointData.at(i - offset).y();

    if (processMode() != PlotCurve::View)
        return;

    setSamples(privateData->pointData);
}

void PlotCurve::replaceData(double x, const QPointF &point)
{
    int address =  indexOffX(x);
    if (address < 0)
        return;

    privateData->pointData.replace(address, point);

    if (privateData->floatData)
        _mm_free(privateData->floatData);
    privateData->floatData = (float *) _mm_malloc(sizeof(float) * privateData->pricePlot->periods, 16);

    int size = privateData->pointData.size();
    int offset = privateData->pricePlot->periods - size;
    for (int i = 0; i < offset; ++i)
        privateData->floatData[i] = IGNORE_VALUE;

    for (int i = offset; i < privateData->pricePlot->periods; ++i)
        privateData->floatData[i] = privateData->pointData.at(i - offset).y();

    if (processMode() != PlotCurve::View)
        return;

    setSamples(privateData->pointData);
}

void PlotCurve::clearData(void)
{
    privateData->pointData.clear();
    _mm_free(privateData->floatData);
    privateData->floatData = NULL;

    if (processMode() != PlotCurve::View)
        return;

    setSamples(privateData->pointData);
}

const QVector<QPointF> &PlotCurve::data(void) const
{
    return privateData->pointData;
}

const float *PlotCurve::fastData() const
{
    return privateData->floatData;
}

void PlotCurve::setXAxisStartPoint(double x)
{
    for (int i = 0; i < privateData->pointData.size(); i++)
    {
        privateData->pointData[i].setX(x);
        x += 1.0;
    }

    if (processMode() != PlotCurve::View)
        return;

    setSamples(privateData->pointData);
}

int PlotCurve::dataSize() const
{
    return privateData->pointData.size();
}

double PlotCurve::x(int i) const
{
    return privateData->pointData.at(i).x();
}

double PlotCurve::y(int i) const
{
    return privateData->pointData.at(i).y();
}

void PlotCurve::itemChanged()
{
    if (processMode() == PlotCurve::Data)
        return;

    QwtPlotItem::itemChanged();
}

int PlotCurve::indexOffX(double x) const
{
    int size = dataSize();

    if (size < 1)
        return -1;

    if (this->x(0) == x)
        return 0;
    else if (this->x(0) < x)
    {
        int address = x - this->x(0);
        if (address >= size)
            return -1;
        else
            return x - this->x(0);
    }

    return -1;
}

PlotCurve::ProcessMode PlotCurve::processMode() const
{
    return privateData->processMode;
}

void PlotCurve::setProcessMode(ProcessMode mode)
{
    privateData->processMode = mode;
    if (mode == PlotCurve::View)
    {
        attach(privateData->pricePlot);
        setSamples(privateData->pointData);
        privateData->pricePlot->scheduleReplot();
    }
    else
        detach();
}

double PlotCurve::plotOffset() const
{
    return privateData->offset;
}

void PlotCurve::setPlotOffset(double offset)
{
    if (privateData->style == PlotCurve::Line)
        offset = 0.0;

    privateData->offset = offset;
}

Plot *PlotCurve::pricePlot() const
{
    return privateData->pricePlot;
}

class CopyMouseMoveEvent : public QEvent
{
    public:
        CopyMouseMoveEvent() : QEvent((QEvent::Type) 2000) {}
        virtual ~CopyMouseMoveEvent() {}

        static const QEvent::Type CopyMouseMove = static_cast<QEvent::Type>(2000);

        int x;
};

Tracker::Tracker(int xAxis, int yAxis, QwtPlotCanvas *canvas) :
    QwtPlotPicker(xAxis, yAxis, canvas)
{
    setStateMachine(new QwtPickerDragPointMachine());
    connect(this, SIGNAL(moved(QPoint)), this, SLOT(createFromToMoveEvent(QPoint)));
    canvas->installEventFilter(this);
    releaseMouseGrab = 0;
    setEnabled(true);
    popMenu = new QMenu(canvas);
}

bool Tracker::eventFilter(QObject *object, QEvent *event)
{
    if (!enabled())
        return QwtPlotPicker::eventFilter(object, event);

    Plot *ptr = static_cast<Plot *>(plot());
    if (!ptr || (ptr->canvas() != object && !ptr->tracker()->enabled()))
        return QwtPlotPicker::eventFilter(object, event);

    if (event->type() == QEvent::Enter)
    {
        ptr->tracker()->setRubberBand(CrossRubberBand);

        QMouseEvent *mouseEvent = new QMouseEvent(QEvent::MouseButtonPress, QPoint(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::postEvent(ptr->canvas(), mouseEvent);
    }
    else if (event->type() == QEvent::Leave)
    {
        ptr->tracker()->setRubberBand(NoRubberBand);
        QTimer::singleShot(0, ptr->tracker(), SLOT(slotUpdateDisplay()));
    }
    else if (event->type() == QEvent::MouseMove && object != canvas())
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        QPoint mousePos = mouseEvent->pos();
        mousePos.setY(-100000);
        ptr->tracker()->move(mousePos);
    }
    else if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent *mouseClickEvent = static_cast<QMouseEvent *>(event);
        if (mouseClickEvent->button() == Qt::RightButton)
        {
            xClicked = strategie->currentPriceChart()->indexPlot->invTransform(QwtPlot::xBottom, mouseClickEvent->pos().x());
            popMenu->popup(mouseClickEvent->globalPos());
        }
    }
    else if (event->type() == QEvent::MouseButtonRelease)
    {
        QMouseEvent *mouseClickEvent = static_cast<QMouseEvent *>(event);
        if (mouseClickEvent->button() == Qt::LeftButton)
        {
            ptr->tracker()->setRubberBand(CrossRubberBand);

            QMouseEvent *mouseEvent = new QMouseEvent(QEvent::MouseButtonPress, QPoint(-10000, -10000), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QApplication::postEvent(ptr->canvas(), mouseEvent);
        }
    }

    return QwtPlotPicker::eventFilter(object, event);
}

bool Tracker::enabled() const
{
    return trackerEnabled;
}

void Tracker::setEnabled(bool enabled)
{
    trackerEnabled = enabled;

    if (enabled)
    {
        setRubberBand(CrossRubberBand);
        QMouseEvent *mouseEvent = new QMouseEvent(QEvent::MouseButtonPress, QPoint(-10000, -10000), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::postEvent(canvas(), mouseEvent);
    }
    else
        setRubberBand(NoRubberBand);
}

void Tracker::slotUpdateDisplay(void)
{
    updateDisplay();
}

void Tracker::createFromToMoveEvent(const QPoint &point)
{
    if (previousPoint.isNull())
        previousPoint = point;

    emit movedFromTo(previousPoint, point);

    previousPoint = point;
}

TimeScaleDraw::TimeScaleDraw(Plot *plot) : QwtScaleDraw()
{
    pricePlot = plot;
}

QwtText TimeScaleDraw::label(double value) const
{
    const QVector<QDateTime> &timeScaleData = pricePlot->timeScaleData();
    if (timeScaleData.isEmpty())
        return QwtText(QString::number(value));

    int bar = static_cast<int>(value);
    if (bar >= timeScaleData.size() || bar < 0)
        return QwtText("");

    return timeScaleData.at(bar).toString(pricePlot->timeScaleFormat());
}

Legend::Legend(QWidget *parent) :
    QwtLegend(parent)
{
    if (parent)
        parent->installEventFilter(this);
}

Legend::~Legend(void)
{

}

void Legend::adjustGeometry(void)
{
    int minWidth = 1;
    for (int i = 0; i < legendItems().size(); i++)
    {
        if (legendItems().at(i)->sizeHint().width() > minWidth)
            minWidth = legendItems().at(i)->sizeHint().width();
    }

    if (legendItems().count() > 1)
        minWidth = minWidth * 2;

    if (legendItems().count() > 0)
        minWidth = minWidth + verticalScrollBar()->width();

    int minHeight = 1;
    if (heightForWidth(minWidth) > minHeight)
        minHeight = heightForWidth(minWidth);

    if (parentWidget())
    {
        int maxHeight = parentWidget()->height() - 30;
        if (minHeight > maxHeight)
            minHeight = maxHeight;
    }

    resize(minWidth, minHeight);

    if (parentWidget())
    {
        QRect parentRect = parentWidget()->geometry();
        move(parentRect.width() - minWidth - parentWidget()->contentsMargins().right(), 15);
    }

    contentsWidget()->layout()->invalidate();
}

bool Legend::eventFilter(QObject *object, QEvent *event)
{
    if (object != this && event->type() == QEvent::Resize)
    {
        adjustGeometry();
    }

    return QwtLegend::eventFilter(object, event);
}

QwtArray <Plot *> plotList;

class Plot::PrivateData
{
    public:
        double curveOffset;
        QTimer replotTimer;
        PlotCurve *configCurve;
        TransparentBlurredPopup *curvePropertiesPopup;
        Ui::CurvePropertiesForm *curvePropertiesForm;
        ScrollZoomer *zoomer;
        bool autoZoom;
        CanvasPicker *picker;
        Tracker *tracker;
        bool plottingDisabled;
        TimeScaleDraw *timeScaleDraw;
        QString timeScaleFormat;
        QVector<QDateTime> timeScaleData;
};

Plot::Plot(QWidget *parent) :
    QwtPlot(parent)
{
    privateData = new PrivateData;
    privateData->curveOffset = 0.2;
    privateData->configCurve = NULL;
    privateData->curvePropertiesPopup = NULL;
    privateData->plottingDisabled = false;
    privateData->replotTimer.setSingleShot(true);
    privateData->replotTimer.setInterval(40);
    connect(&privateData->replotTimer, SIGNAL(timeout()), this, SLOT(replot()));

    periods = 0;

    plotList.append(this);
    canvas()->installEventFilter(this);
    setAutoReplot(false);

    QPalette palette;
    QwtPlotGrid *grid = new QwtPlotGrid;
    grid->setMajPen(QPen(palette.color(QPalette::Text).darker(), 1.0, Qt::DashLine));
    grid->setMinPen(QPen(palette.color(QPalette::Text).darker(), 1.0, Qt::DashLine));
    grid->enableXMin(true);
    grid->enableYMin(false);
    grid->attach(this);

    canvas()->setFrameShape(QFrame::NoFrame);
    canvas()->setMouseTracking(true);

    QwtScaleWidget *xScaleWidget = axisWidget(QwtPlot::xBottom);
    QFont font = xScaleWidget->font();
    if (font.pixelSize() != -1)
        font.setPixelSize(font.pixelSize() - (font.pixelSize() / 5));
    else if (font.pointSizeF() != -1)
        font.setPointSizeF(font.pointSizeF() - (font.pointSizeF() / 5.0));
    xScaleWidget->setFont(font);
    privateData->timeScaleDraw = new TimeScaleDraw(this);
    setAxisScaleDraw(QwtPlot::xBottom, privateData->timeScaleDraw);
    setAxisLabelAlignment(QwtPlot::xBottom, Qt::AlignCenter);
    enableAxis(QwtPlot::xBottom, true);
    setAxisScale(QwtPlot::xBottom, 0.0, 1.0 + (roundl(0.1 * 1.0)), AXIS_STEP_SIZE);

    QwtScaleWidget *yScaleWidget = axisWidget(QwtPlot::yLeft);
    font = yScaleWidget->font();
    if (font.pixelSize() != -1)
        font.setPixelSize(font.pixelSize() - (font.pixelSize() / 5.0));
    else if (font.pointSizeF() != -1)
        font.setPointSizeF(font.pointSizeF() - (font.pointSizeF() / 5.0));
    yScaleWidget->setFont(font);
    yScaleWidget->scaleDraw()->setMinimumExtent(60);
    setAxisScale(QwtPlot::yLeft, 0, 50000);
    setAxisScale(QwtPlot::yRight, -40000, 40000);

    plotLayout()->setAlignCanvasToScales(false);
    plotLayout()->setSpacing(0);
    plotLayout()->setMargin(0);
    plotLayout()->setCanvasMargin(font.pixelSize() / 2);

    privateData->zoomer = new ScrollZoomer(canvas());
    privateData->zoomer->setEnabled(false);
    privateData->zoomer->setMaxStackDepth(50);
    privateData->zoomer->setVScrollBarMode(Qt::ScrollBarAlwaysOff);
    setAutoZoomEnabled(true);

    privateData->picker = new CanvasPicker(this);
    privateData->tracker = new Tracker(QwtPlot::xBottom, QwtPlot::yLeft, canvas());
    QPen rubberBandPen = privateData->tracker->rubberBandPen();
    rubberBandPen.setWidth(1);
    rubberBandPen.setStyle(Qt::DashLine);
    rubberBandPen.setColor(qApp->palette().color(QPalette::Text));
    privateData->tracker->setRubberBandPen(rubberBandPen);

    Legend *legend = new Legend(canvas());
    QFont legendFont = legend->font();
    legendFont.setBold(true);
    legend->setFont(legendFont);
    legend->setItemMode(QwtLegend::ClickableItem);
    legend->setCursor(Qt::ArrowCursor);
    legend->setAttribute(Qt::WA_NoSystemBackground);
    legend->contentsWidget()->setAttribute(Qt::WA_NoSystemBackground);
    insertLegend(legend, QwtPlot::ExternalLegend);
    connect(this, SIGNAL(legendClicked(QwtPlotItem *)), this, SLOT(showCurvePropertiesDialog(QwtPlotItem *)));
}

Plot::~Plot(void)
{
    delete privateData->zoomer;

    int index = plotList.indexOf(this);
    if (index != -1)
        plotList.remove(index);

    delete privateData;
}

CanvasPicker *Plot::picker() const
{
    return privateData->picker;
}

Tracker *Plot::tracker(void) const
{
    return privateData->tracker;
}

bool Plot::plottingDisabled()
{
    return privateData->plottingDisabled;
}

void Plot::setPlottingDisabled(bool enabled)
{
    privateData->plottingDisabled = enabled;
}

ScrollZoomer *Plot::zoomer(void) const
{
    return privateData->zoomer;
}

void Plot::setXAxisScaleToCurve(const PlotCurve &curve)
{
    bool zoomed = false;
    if (curve.dataSize() < 1)
        return;

    bool canvasUpdatesEnabled =  canvas()->updatesEnabled();
    canvas()->setUpdatesEnabled(false);

    QStack<QRectF> tmpStack = zoomer()->zoomStack();
    if (zoomer()->zoomRectIndex() != 0)
    {
        zoomed = true;
    }

    setAxisScale(QwtPlot::xBottom, curve.x(0), curve.x(curve.dataSize() - 1) + (roundl(0.1 * static_cast<double>(curve.dataSize()))), AXIS_STEP_SIZE);
    QwtPlot::replot();
    zoomer()->setZoomBase();

    if (zoomed)
    {
        tmpStack.remove(0);
        tmpStack.prepend(zoomer()->zoomBase());
        zoomer()->setZoomStack(tmpStack);
    }

    QwtPlot::replot();
    canvas()->setUpdatesEnabled(canvasUpdatesEnabled);
}

int Plot::findAddressForX(double x)
{
    return -1;
}

double Plot::findXForDateTime(QDateTime &dateTime)
{
    return -1;
}

double Plot::findXForDate(QDate &date)
{
    return -1;
}

void Plot::replot(void)
{
    if (plottingDisabled() && sender() != &privateData->replotTimer)
        return;

    QwtPlot::replot();
}

void Plot::scheduleReplot(void)
{
    if (plottingDisabled())
        return;

    if (privateData->replotTimer.isActive())
        return;

    privateData->replotTimer.start();
}

void Plot::zoomIn(void)
{
    QRectF zRect = zoomer()->zoomRect();

    QwtDoublePoint center = zRect.center();
    zRect.setWidth(zRect.width() / 2);
    zRect.moveCenter(center);
    zoomer()->zoom(zRect);

    magnifieYAxis();

    if (zoomer()->horizontalScrollBar())
        connect(zoomer()->horizontalScrollBar(), SIGNAL(valueChanged(int)), SLOT(magnifieYAxis()), Qt::UniqueConnection);

    replot();

    emit zoomedIn();
}

void Plot::zoomIn(int factor)
{
    QRectF zRect = zoomer()->zoomRect();
    QRectF bRect = zoomer()->zoomBase();

    zRect.setWidth(zRect.width() / (factor * 2));
    zRect.moveRight(bRect.width());
    zoomer()->zoom(zRect);

    magnifieYAxis();

    if (zoomer()->horizontalScrollBar())
        connect(zoomer()->horizontalScrollBar(), SIGNAL(valueChanged(int)), SLOT(magnifieYAxis()), Qt::UniqueConnection);

    replot();

    emit zoomedIn(factor);
}

void Plot::zoomOut(void)
{
    QStack<QRectF> tmpStack = zoomer()->zoomStack();
    if (tmpStack.count() == 1)
        return;

    tmpStack.pop();
    zoomer()->setZoomStack(tmpStack);

    replot();

    emit zoomedOut();
}


void Plot::zoomReset(void)
{
    zoomer()->zoom(0);
    zoomer()->setZoomBase();

    emit zoomedOut();
    emit zoomedReset();
}

void Plot::zoomGoto(double preferedPeriodsInZoom, double x1, double x2)
{
    QRectF rect = zoomer()->zoomRect();

    rect.setLeft(x1 - ((x2 - x1) / 20));
    if (rect.left() < 0)
        rect.setLeft(0);

    int neededWidth = (x2 - x1) + ((x2 - x1) / 10);
    if (neededWidth > preferedPeriodsInZoom)
        rect.setWidth(neededWidth);
    else
        rect.setWidth(preferedPeriodsInZoom);

    QStack<QRectF> tmpStack = zoomer()->zoomStack();
    if (zoomer()->zoomRectIndex() > 0)
        tmpStack.pop();
    tmpStack.push(rect);
    zoomer()->setZoomStack(tmpStack);

    magnifieYAxis();

    if (zoomer()->horizontalScrollBar())
        connect(zoomer()->horizontalScrollBar(), SIGNAL(valueChanged(int)), SLOT(magnifieYAxis(int)), Qt::UniqueConnection);

    replot();

    emit zoomedGoto(preferedPeriodsInZoom, x1, x2);
}

void Plot::magnifieYAxis(void)
{
    if (!autoZoomEnabled())
        return;

    QList<PlotCurve *> indicatorMagnifyList;
    QList<QwtPlotPrice *> priceMagnifyList;

    for (int i = 0; i < itemList().size(); i++)
    {
        QwtPlotItem *plotItem = itemList().at(i);
        if (plotItem->rtti() == QwtPlotItem::Rtti_PlotCurve)
        {
            PlotCurve *curve = dynamic_cast<PlotCurve *>(plotItem);
            if (curve)
                indicatorMagnifyList.append(curve);
        }

        if (plotItem->rtti() == QwtPlotItem::Rtti_PlotPrice)
        {
            QwtPlotPrice *curve = dynamic_cast<QwtPlotPrice *>(plotItem);
            if (curve)
                priceMagnifyList.append(curve);
        }
    }

    if (indicatorMagnifyList.isEmpty() && priceMagnifyList.isEmpty())
        return;

    QRectF rect = zoomer()->zoomRect();
    int start = static_cast<int>(rect.left());
    int end = static_cast<int>(rect.right());
    double high = DBL_MIN;
    double low = DBL_MAX;

    for (int i = 0; i < indicatorMagnifyList.size(); i++)
    {
        PlotCurve *curve = indicatorMagnifyList.at(i);
        for (int j = 0; j < curve->dataSize(); j++)
        {
            if (curve->x(j) < start || curve->x(j) > end)
                continue;

            if (curve->y(j) > high)
                high = curve->y(j);
            else if (curve->y(j) < low)
                low = curve->y(j);
        }
    }

    for (int i = 0; i < priceMagnifyList.size(); i++)
    {
        QwtPlotPrice *curve = priceMagnifyList.at(i);
        for (int j = 0; j < curve->dataSize(); j++)
        {
            QwtPriceMove priceMove = curve->data().sample(j);
            if (priceMove.interval.minValue() < start || priceMove.interval.minValue() > end)
                continue;

            if (priceMove.high > high)
                high = priceMove.high;
            else if (priceMove.low < low)
                low = priceMove.low;
        }
    }

    if (high == DBL_MIN)
        high = rect.bottom();

    if (low == DBL_MAX)
        low = rect.top();

    double range = fabs(high - low);
    rect.setCoords(rect.left(), low - (0.05 * range), rect.right(), high + (0.05 * range));

    bool updates = canvas()->updatesEnabled();
    canvas()->setUpdatesEnabled(false);

    QStack<QRectF> tmpStack = zoomer()->zoomStack();
    tmpStack.pop();
    tmpStack.push(rect);
    zoomer()->setZoomStack(tmpStack);

    canvas()->setUpdatesEnabled(updates);
}

bool Plot::autoZoomEnabled(void) const
{
    return privateData->autoZoom;
}

void Plot::setAutoZoomEnabled(bool enabled)
{
    privateData->autoZoom = enabled;
}

const QString &Plot::timeScaleFormat(void) const
{
    return privateData->timeScaleFormat;
}

void Plot::setTimeScaleFormat(const QString &format)
{
    privateData->timeScaleFormat = format;
}

const QVector<QDateTime> &Plot::timeScaleData(void) const
{
    return privateData->timeScaleData;
}

void Plot::setTimeScaleData(const QVector <QDateTime> &data)
{
    privateData->timeScaleData = data;
}

void Plot::appendTimeScaleData(const QDateTime &date)
{
    privateData->timeScaleData << date;

    if (privateData->timeScaleData.size() > 1)
        privateData->timeScaleData.remove(0);
}

void Plot::setPeriods(int count)
{
    periods = count;
    emit periodsChanged(periods);
}

void Plot::showCurvePropertiesDialog(QwtPlotItem *plotItem)
{
    PlotCurve *curve = static_cast<PlotCurve *>(plotItem);
    if (!curve)
        return;

    privateData->configCurve = curve;

    if (!privateData->curvePropertiesPopup)
    {
        privateData->curvePropertiesPopup = new TransparentBlurredPopup();
        privateData->curvePropertiesForm = new Ui::CurvePropertiesForm();
        privateData->curvePropertiesForm->setupUi(privateData->curvePropertiesPopup);

        for (int i = 0; i < ColorPool::listColors().size(); i++)
        {
            QColor color = ColorPool::listColors().at(i);
            QPixmap colorPixmap(40, 40);
            colorPixmap.fill(color);

            int index;
            for (index = 0; index < QColor::colorNames().size(); index++)
            {
                if (QColor(QColor::colorNames().at(index)) == color)
                    break;
            }

            if (index >= QColor::colorNames().size())
                privateData->curvePropertiesForm->colorComboBox->addItem(QIcon(colorPixmap), "");
            else
                privateData->curvePropertiesForm->colorComboBox->addItem(QIcon(colorPixmap), tr(QColor::colorNames().at(index).toAscii().data()));
        }

        connect(privateData->curvePropertiesForm->colorComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setCurveProperties()));

        QSize size = privateData->curvePropertiesForm->widthComboBox->iconSize();
        privateData->curvePropertiesForm->widthComboBox->setIconSize(QSize(size.width() * 3, size.height()));
        for (int i = 0; i < 5; i++)
        {
            QPixmap widthPixmap(size.width() * 3, size.height());
            widthPixmap.fill(Qt::white);
            QPainter painter(&widthPixmap);
            painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform, true);
            QPen pen = painter.pen();
            pen.setWidth(i + 1);
            pen.setColor(Qt::black);
            painter.setPen(pen);
            painter.drawLine(0, size.height() / 2, size.width() * 3, size.height() / 2);
            painter.end();

            privateData->curvePropertiesForm->widthComboBox->addItem(QIcon(widthPixmap), QString::number(i + 1));
        }

        connect(privateData->curvePropertiesForm->widthComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setCurveProperties()));

        size = privateData->curvePropertiesForm->styleComboBox->iconSize();
        privateData->curvePropertiesForm->styleComboBox->setIconSize(QSize(size.width() * 3, size.height()));
        for (int i = 0; i < 6; i++)
        {
            QPixmap typePixmap(size.width() * 3, size.height());
            typePixmap.fill(Qt::white);
            QPainter painter(&typePixmap);
            painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform, true);
            QPen pen = painter.pen();
            pen.setWidth(2);
            pen.setColor(Qt::black);
            pen.setStyle(static_cast<Qt::PenStyle>(i));
            painter.setPen(pen);
            painter.drawLine(0, size.height() / 2, size.width() * 3, size.height() / 2);
            painter.end();

            privateData->curvePropertiesForm->styleComboBox->addItem(QIcon(typePixmap), "");
        }

        connect(privateData->curvePropertiesForm->styleComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setCurveProperties()));
        connect(privateData->curvePropertiesForm->fillCheckBox, SIGNAL(clicked()), this, SLOT(setCurveProperties()));
        connect(privateData->curvePropertiesForm->closePushButton, SIGNAL(clicked()), privateData->curvePropertiesPopup, SLOT(hide()));
    }

    privateData->curvePropertiesForm->colorComboBox->blockSignals(true);
    privateData->curvePropertiesForm->colorComboBox->setCurrentIndex(ColorPool::indexOfColor(privateData->configCurve->pen().color()));
    privateData->curvePropertiesForm->colorComboBox->blockSignals(false);

    privateData->curvePropertiesForm->widthComboBox->blockSignals(true);
    privateData->curvePropertiesForm->widthComboBox->setCurrentIndex(privateData->configCurve->pen().width() - 1);
    privateData->curvePropertiesForm->widthComboBox->blockSignals(false);

    privateData->curvePropertiesForm->styleComboBox->blockSignals(true);
    privateData->curvePropertiesForm->styleComboBox->setCurrentIndex(static_cast<int>(privateData->configCurve->pen().style()));
    privateData->curvePropertiesForm->styleComboBox->blockSignals(false);

    privateData->curvePropertiesForm->fillCheckBox->blockSignals(true);
    privateData->curvePropertiesForm->fillCheckBox->setChecked(privateData->configCurve->brush().style() != Qt::NoBrush ? true : false);
    privateData->curvePropertiesForm->fillCheckBox->blockSignals(false);

    bool left;
    bool top;
    if (QCursor::pos().x() > (mapToGlobal(pos()).x() + (width() / 2)))
        left = false;
    else
        left = true;

    if (QCursor::pos().y() > (mapToGlobal(pos()).y() + (height() / 2)))
        top = false;
    else
        top = true;

    QPoint pos = QCursor::pos();
    if (top && !left)
        pos.setX(pos.x() - privateData->curvePropertiesPopup->width());
    else if (!top && left)
        pos.setY(pos.y() - privateData->curvePropertiesPopup->height());
    else if (!top && !left)
    {
        pos.setX(pos.x() - privateData->curvePropertiesPopup->width());
        pos.setY(pos.y() - privateData->curvePropertiesPopup->height());
    }

    privateData->curvePropertiesPopup->move(pos);
    privateData->curvePropertiesPopup->raise();
    privateData->curvePropertiesPopup->show();
}

void Plot::setCurveProperties()
{
    if (!privateData->configCurve)
        return;

    QPen pen = privateData->configCurve->pen();
    pen.setWidth(privateData->curvePropertiesForm->widthComboBox->currentIndex() + 1);
    pen.setStyle(static_cast<Qt::PenStyle>(privateData->curvePropertiesForm->styleComboBox->currentIndex()));
    pen.setColor(ColorPool::listColors().at(privateData->curvePropertiesForm->colorComboBox->currentIndex()));
    privateData->configCurve->setPen(pen);
    if (privateData->curvePropertiesForm->fillCheckBox->isChecked())
    {
        QColor color = pen.color();
        color.setAlpha(127);
        privateData->configCurve->setBrush(color);
    }
    else
        privateData->configCurve->setBrush(Qt::NoBrush);
    privateData->configCurve->plot()->replot();
}
