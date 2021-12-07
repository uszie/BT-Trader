#include <errno.h>
#include <iostream>
#include <QtGui>
#include "ui_strategie.h"
#include "ui_quote.h"
#include "ui_pricecurveproperties.h"
#include "ui_sourcepicker.h"
#include "strategie.h"
#include "stoplossoptions.h"
#include "plotimagebackground.h"
#include "strategieoverview.h"
#include "menucontextmenu.h"
#include "main.h"
#include "volatility.h"
#include "er.h"
#include "rsi.h"
#include "adx.h"
#include "dominantcycle.h"
#include "trendvigor.h"
#include "snr.h"
#include "movingaverage.h"
#include "2movingaverage.h"
#include "3movingaverage.h"
#include "line.h"
#include "bollingerbands.h"
#include "pricechannel.h"
#include "dominantcycle.h"
#include "trendvigor.h"
#include "snr.h"
#include "stochastic.h"
#include "instantaneoustrend.h"
#include "trend.h"
#include <qwt_scale_widget.h>
#include <qwt_legend_item.h>

bool aboutToQuit = false;

class MarkerPoints::PrivateData
{
    public:
        PriceChart *priceChart;
        Plot *indexPlot;
        QList <QwtPlotSvgItem *> markerList;
        QByteArray iconExit;
        QByteArray iconExitSelected;
        QByteArray iconLong;
        QByteArray iconLongSelected;
        QByteArray iconShort;
        QByteArray iconShortSelected;
        QByteArray iconTakeProfits;
        QByteArray iconTakeProfitsSelected;
        QwtPlotSvgItem *selected;
        QColor markerColor;
        QTimer hoverTimer;
        int maxPixelWidth;
};

MarkerPoints::MarkerPoints(PriceChart *priceChart)
{
    privateData = new PrivateData;
    privateData->selected = NULL;
    privateData->hoverTimer.setInterval(500);
    privateData->hoverTimer.setSingleShot(true);
    privateData->priceChart = priceChart;
    privateData->indexPlot = priceChart->indexPlot;
    privateData->maxPixelWidth = 36;//25;
    privateData->indexPlot->canvas()->installEventFilter(this);
    init();
    connect(privateData->indexPlot->zoomer(), SIGNAL(zoomed(const QwtDoubleRect &)), this, SLOT(rescaleMarkers()));
    connect(&privateData->hoverTimer, SIGNAL(timeout()), this, SLOT(hover()));
}

MarkerPoints::~MarkerPoints(void)
{
    int size = privateData->markerList.size();
    for (int i = 0; i < size; ++i)
    {
        QwtPlotSvgItem *svgPlotItem = privateData->markerList.takeFirst();
        svgPlotItem->hide();
        svgPlotItem->detach();
        delete svgPlotItem;
    }
}

bool MarkerPoints::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::Resize)
        rescaleMarkers();
    else if (event->type() == QEvent::MouseMove)
    {
        QMouseEvent *mouseEvent = (QMouseEvent *) event;
        for (int i = 0; i < privateData->markerList.size(); ++i)
        {
            QPointF point;
            point.setX(privateData->indexPlot->invTransform(QwtPlot::xBottom, mouseEvent->x()));
            point.setY(privateData->indexPlot->invTransform(QwtPlot::yLeft, mouseEvent->y()));
            if (privateData->markerList.at(i)->boundingRect().contains(point))
            {
                if (privateData->selected == privateData->markerList.at(i))
                    return QObject::eventFilter(object, event);
                else
                {
                    privateData->selected = privateData->markerList.at(i);
                    privateData->hoverTimer.start();
                    return QObject::eventFilter(object, event);
                }
            }
            else if (privateData->markerList.at(i) == privateData->selected)
            {
                emit hoverLeft(*privateData->selected);
                privateData->selected = NULL;
                privateData->hoverTimer.stop();
                return QObject::eventFilter(object, event);
            }
        }

    }
    else if (event->type() == QEvent::Leave)
    {
        if (privateData->selected)
            emit hoverLeft(*privateData->selected);
        privateData->selected = NULL;
        privateData->hoverTimer.stop();
    }


    return QObject::eventFilter(object, event);
}

void MarkerPoints::hover(void)
{
    if (!privateData->selected)
        return;

    emit hovered(*privateData->selected);
}

void MarkerPoints::init(void)
{
    QString iconPath = ":/images/process-stop.svg";
    QFile iconFile(iconPath);
    if (!iconFile.open(QIODevice::ReadOnly | QIODevice::Text))
        qWarning() << "MarkerPoints::init(): iconFile.open( " << iconPath.toAscii().data() << " ): " << strerror(errno);
    else
    {
        privateData->iconExit = iconFile.readAll();
        privateData->iconExitSelected = privateData->iconExit;
        iconFile.close();
    }

    iconPath = ":/images/go-up.svg";
    iconFile.setFileName(iconPath);
    if (!iconFile.open(QIODevice::ReadOnly | QIODevice::Text))
        qWarning() << "MarkerPoints::init(): iconFile.open( " << iconPath.toAscii().data() << " ): " << strerror(errno);
    else
    {
        privateData->iconLong = iconFile.readAll();
        privateData->iconLongSelected = privateData->iconLong;
        iconFile.close();
    }

    iconPath = ":/images/go-down.svg";;
    iconFile.setFileName(iconPath);
    if (!iconFile.open(QIODevice::ReadOnly | QIODevice::Text))
        qWarning() << "MarkerPoints::init(): iconFile.open( " << iconPath.toAscii().data() << " ): " << strerror(errno);
    else
    {
        privateData->iconShort = iconFile.readAll();
        privateData->iconShortSelected = privateData->iconShort;
        iconFile.close();
    }

    iconPath = ":/images/go-right.svg";;
    iconFile.setFileName(iconPath);
    if (!iconFile.open(QIODevice::ReadOnly | QIODevice::Text))
        qWarning() << "MarkerPoints::init(): iconFile.open( " << iconPath.toAscii().data() << " ): " << strerror(errno);
    else
    {
        privateData->iconTakeProfits = iconFile.readAll();
        privateData->iconTakeProfitsSelected = privateData->iconTakeProfits;
        iconFile.close();
    }

    privateData->markerColor = QColor(Qt::darkBlue);
    setColor(privateData->markerColor);
}

void MarkerPoints::setColor(const QColor &color)
{
    privateData->markerColor = color;
    colorizeIcon(privateData->iconExit);
    colorizeIcon(privateData->iconExitSelected, true);
    colorizeIcon(privateData->iconLong);
    colorizeIcon(privateData->iconLongSelected, true);
    colorizeIcon(privateData->iconShort);
    colorizeIcon(privateData->iconShortSelected, true);
    colorizeIcon(privateData->iconTakeProfits);
    colorizeIcon(privateData->iconTakeProfitsSelected, true);

    rescaleMarkers(true);
    privateData->indexPlot->replot();
}


void MarkerPoints::colorizeIcon(QByteArray &icon, bool selected)
{
    char *colorName;
    QColor orColor;
    char c;
    for (int i = 0; (i = icon.indexOf("stop-color", i)) != -1; ++i)
    {
        colorName = icon.data() + (i + 11);
        c = colorName[ 7 ];
        colorName[ 7 ] = '\0';
        orColor.setNamedColor(colorName);
        colorName[ 7 ] = c;

        int h, s, v;
        orColor.getHsv(&h, &s, &v);
        h = privateData->markerColor.hsvHue();
        orColor.setHsv(h, s, v);
        if (selected)
            orColor = orColor.darker(150/*135*/);

        orColor.setAlpha(255);
        icon.replace(i + 11, 7, orColor.name().toAscii().data());
    }

    icon.replace("fill-opacity:1", "fill-opacity:0.9");
}

void MarkerPoints::addMarker(struct transaction data)
{
    double xCenter = data.x + 0.20;
    QwtDoublePoint svgPoint;
    svgPoint.setX(xCenter);

    double factor = privateData->indexPlot->axisScaleDiv(QwtPlot::yLeft)->range() / privateData->indexPlot->axisScaleDiv(QwtPlot::xBottom)->range();
    QRectF svgRect = privateData->indexPlot->zoomer()->zoomBase();
    if (data.type == Short)
    {
        svgRect.setWidth(1);
        svgRect.setHeight(1 * factor);
        svgPoint.setY(data.value + (svgRect.height() / 2));
    }
    else
    {
        svgRect.setWidth(1);
        svgRect.setHeight(1 * factor);
        svgPoint.setY(data.value - (svgRect.height() / 2));
    }

    svgRect.moveCenter(svgPoint);
    if ((data.previousType == Exit) && (!privateData->markerList.isEmpty()))
    {
        QwtPlotSvgItem *pItem = privateData->markerList.last();
        if ((pItem->boundingRect().x() + 0.5) == xCenter)
        {
            pItem->detach();
            privateData->markerList.removeLast();
            delete pItem;
        }
    }

    QwtPlotSvgItem *svgPlotItem = new QwtPlotSvgItem();
    svgPlotItem->setItemAttribute(QwtPlotItem::AutoScale, false);
    svgPlotItem->setRenderHint(QwtPlotItem::RenderAntialiased, false);
    svgPlotItem->setZ(200);
    svgPlotItem->setBoundingRect(svgRect);

    if (data.type == Short)
    {
        svgPlotItem->loadData(svgRect, privateData->iconShort);
        svgPlotItem->setTitle("Short");
    }
    else if (data.type == Long)
    {
        svgPlotItem->loadData(svgRect, privateData->iconLong);
        svgPlotItem->setTitle("Long");
    }
    else if (data.type == Exit)
    {
        svgPlotItem->loadData(svgRect, privateData->iconExit);
        svgPlotItem->setTitle("Exit");
    }
    else if (data.type == TakeProfits)
    {
        svgPlotItem->loadData(svgRect, privateData->iconTakeProfits);
        svgPlotItem->setTitle("Take Profits");
    }
    else
        return;

    privateData->markerList << svgPlotItem;
}

QByteArray MarkerPoints::svgIcon(MarkerPoints::IconType type) const
{
    switch (type)
    {
        case MarkerPoints::IconLong:
            return privateData->iconLong;
        case MarkerPoints::IconLongSelected:
            return privateData->iconLongSelected;
        case MarkerPoints::IconShort:
            return privateData->iconShort;
        case MarkerPoints::IconShortSelected:
            return privateData->iconShortSelected;
        case MarkerPoints::IconExit:
            return privateData->iconExit;
        case MarkerPoints::IconExitSelected:
            return privateData->iconExitSelected;
        default:
            return QByteArray();

    }

    return QByteArray();
}

void MarkerPoints::rescaleMarkers(bool forceDataReload)
{
    if (privateData->markerList.isEmpty())
        return;

    if (privateData->indexPlot->transform(QwtPlot::xBottom, privateData->indexPlot->zoomer()->zoomBase().right()) == privateData->indexPlot->transform(QwtPlot::xBottom, privateData->indexPlot->zoomer()->zoomBase().left()))
        return;

    QwtDoublePoint centerPoint;
    QRectF zRect = privateData->indexPlot->zoomer()->zoomRect();
    double factor = zRect.height() / zRect.width();
    double zFactor = (((double) privateData->indexPlot->periods) / ((double) privateData->markerList.count())) / 2.0;
    double y;

    for (int i = 0; i < privateData->markerList.count(); ++i)
    {
        QRectF svgRect = privateData->markerList.at(i)->boundingRect();
        if (!zRect.contains(svgRect.center()))
        {
            privateData->markerList.at(i)->detach();
            continue;
        }

        double xCenter = (svgRect.left() + svgRect.right()) / 2;
        double yCenter = (svgRect.top() + svgRect.bottom()) / 2;
        centerPoint.setX(xCenter);

        if (privateData->markerList.at(i)->title() == QwtText("Short") || privateData->markerList.at(i)->title() == QwtText("Short Selected"))
        {
            y = yCenter - (svgRect.height() / 2);
            svgRect.setWidth(1 * zFactor);
            svgRect.setHeight(1 * factor * zFactor);
            yCenter = y + (svgRect.height() / 2);
        }
        else
        {
            y = yCenter + (svgRect.height() / 2);
            svgRect.setWidth(1 * zFactor);
            svgRect.setHeight(1 * factor * zFactor);
            yCenter = y - (svgRect.height() / 2);
        }

        double pixelWidth = privateData->indexPlot->transform(QwtPlot::xBottom, svgRect.right()) - privateData->indexPlot->transform(QwtPlot::xBottom, svgRect.left());
        if (pixelWidth > privateData->maxPixelWidth)
        {
            double xWidth;
            double yHeight;
            xWidth = privateData->indexPlot->invTransform(QwtPlot::xBottom, privateData->maxPixelWidth) - privateData->indexPlot->invTransform(QwtPlot::xBottom, 0);
            double xyRatio = svgRect.height() / svgRect.width();
            yHeight = xyRatio * xWidth;
            svgRect.setWidth(xWidth);
            svgRect.setHeight(yHeight);
            if (privateData->markerList.at(i)->title() == QwtText("Short") || privateData->markerList.at(i)->title() == QwtText("Short Selected"))
                yCenter = y + (svgRect.height() / 2);
            else
                yCenter = y - (svgRect.height() / 2);
        }

        if (pixelWidth < 5)
        {
            privateData->markerList.at(i)->detach();
            continue;
        }
        else
            privateData->markerList.at(i)->attach(privateData->indexPlot);

        centerPoint.setY(yCenter);
        svgRect.moveCenter(centerPoint);

        if (!forceDataReload /*&& privateData->markerList.at(i)->dataLoaded*/)
        {
            privateData->markerList.at(i)->setBoundingRect(svgRect);
            continue;
        }

        privateData->markerList.at(i)->attach(privateData->indexPlot);

        if (privateData->markerList.at(i)->title() == QwtText("Exit"))
            privateData->markerList.at(i)->loadData(svgRect, privateData->iconExit);
        else if (privateData->markerList.at(i)->title() == QwtText("Exit Selected"))
            privateData->markerList.at(i)->loadData(svgRect, privateData->iconExitSelected);
        else if (privateData->markerList.at(i)->title() == QwtText("Long"))
            privateData->markerList.at(i)->loadData(svgRect, privateData->iconLong);
        else if (privateData->markerList.at(i)->title() == QwtText("Long Selected"))
            privateData->markerList.at(i)->loadData(svgRect, privateData->iconLongSelected);
        else if (privateData->markerList.at(i)->title() == QwtText("Short"))
            privateData->markerList.at(i)->loadData(svgRect, privateData->iconShort);
        else if (privateData->markerList.at(i)->title() == QwtText("Short Selected"))
            privateData->markerList.at(i)->loadData(svgRect, privateData->iconShortSelected);
        else if (privateData->markerList.at(i)->title() == QwtText("Take Profits"))
            privateData->markerList.at(i)->loadData(svgRect, privateData->iconTakeProfits);
        else if (privateData->markerList.at(i)->title() == QwtText("Take Profits Selected"))
            privateData->markerList.at(i)->loadData(svgRect, privateData->iconTakeProfitsSelected);

    }
}

void MarkerPoints::highlightMarker(struct transaction data)
{
    for (int i = 0; i < privateData->markerList.count(); ++i)
    {
        QRectF rect = privateData->markerList.at(i)->boundingRect();
        double xCenter = rect.center().x() - 0.2;

        if (qFuzzyCompare(data.x + 1.0, xCenter + 1.0))
        {
            if (data.type == Short)
            {
                privateData->markerList.at(i)->setTitle("Short Selected");
                privateData->markerList.at(i)->loadData(rect, privateData->iconShortSelected);
            }
            else if (data.type == Long)
            {
                privateData->markerList.at(i)->setTitle("Long Selected");
                privateData->markerList.at(i)->loadData(rect, privateData->iconLongSelected);
            }
            else if (data.type == Exit)
            {
                privateData->markerList.at(i)->setTitle("Exit Selected");
                privateData->markerList.at(i)->loadData(rect, privateData->iconExitSelected);
            }
            else if (data.type == TakeProfits)
            {
                privateData->markerList.at(i)->setTitle("Take Profits Selected");
                privateData->markerList.at(i)->loadData(rect, privateData->iconTakeProfitsSelected);
            }

            if (i < (privateData->markerList.count() - 1))
            {
                QwtPlotSvgItem *nextItem = privateData->markerList.at(i + 1);
                QRectF nextRect = nextItem->boundingRect();
                if (nextItem->title().text() == "Short")
                {
                    nextItem->setTitle("Short Selected");
                    nextItem->loadData(nextRect, privateData->iconShortSelected);
                }
                else if (nextItem->title().text() == "Long")
                {
                    nextItem->setTitle("Long Selected");
                    nextItem->loadData(nextRect, privateData->iconLongSelected);
                }
                else if (nextItem ->title().text() == "Exit")
                {
                    nextItem->setTitle("Exit Selected");
                    nextItem->loadData(nextRect, privateData->iconExitSelected);
                }
                else if (nextItem ->title().text() == "Take Profits")
                {
                    nextItem->setTitle("Take Profits Selected");
                    nextItem->loadData(nextRect, privateData->iconTakeProfitsSelected);
                }
            }

            return;
        }
    }
}

void MarkerPoints::unHighlightMarker(struct transaction data)
{
    for (int i = 0; i < privateData->markerList.count(); ++i)
    {
        QRectF rect = privateData->markerList.at(i)->boundingRect();
        double xCenter = rect.center().x() - 0.2;

        if (qFuzzyCompare(data.x + 1.0, xCenter + 1.0))
        {
            if (data.type == Short)
            {
                privateData->markerList.at(i)->setTitle("Short");
                privateData->markerList.at(i)->loadData(rect, privateData->iconShort);
            }
            else if (data.type == Long)
            {
                privateData->markerList.at(i)->setTitle("Long");
                privateData->markerList.at(i)->loadData(rect, privateData->iconLong);
            }
            else if (data.type == Exit)
            {
                privateData->markerList.at(i)->setTitle("Exit");
                privateData->markerList.at(i)->loadData(rect, privateData->iconExit);
            }
            else if (data.type == TakeProfits)
            {
                privateData->markerList.at(i)->setTitle("Take Profits");
                privateData->markerList.at(i)->loadData(rect, privateData->iconTakeProfits);
            }

            if (i < (privateData->markerList.count() - 1))
            {
                QwtPlotSvgItem *nextItem = privateData->markerList.at(i + 1);
                QRectF nextRect = nextItem->boundingRect();
                if (nextItem->title().text() == "Short Selected")
                {
                    nextItem->setTitle("Short");
                    nextItem->loadData(nextRect, privateData->iconShort);
                }
                else if (nextItem->title().text() == "Long Selected")
                {
                    nextItem->setTitle("Long");
                    nextItem->loadData(nextRect, privateData->iconLong);
                }
                else if (nextItem ->title().text() == "Exit Selected")
                {
                    nextItem->setTitle("Exit");
                    nextItem->loadData(nextRect, privateData->iconExit);
                }
                else if (nextItem ->title().text() == "Take Profits Selected")
                {
                    nextItem->setTitle("Take Profits");
                    nextItem->loadData(nextRect, privateData->iconTakeProfits);
                }
            }

        }
    }
}

class PriceChart::PrivateData
{
    public:
        Ui::PriceChart *ui;
        QList<Plot *> plotList;
        bool saveSettings;
        SlidingStackedWidget *indicatorStackWidet;
        QPointer<StrategieOverview> strategieOverviewWidget;
        QTabWidget *transactionsTabWidget;
        PriceChart::ProcessMode processMode;
        int timeScale;
        IsinInfo isinInfo;
        QDateTime firstAvailableDateTime;
        QDateTime lastAvailableDateTime;
        QString configGroup;
        Trend *trendIndicator;
        DominantCycle *dominantCycleIndicator;
        TrendVigor *trendVigorIndicator;
        int plotManualIndicators;
        CalculateCache *indicatorCalculateCache;
        QList<QVariant> plotSplitterSizes;
        ColorPool colorPool;
        QwtPlotMarker *tickMarker;
        QWidget *priceCurvePropertiesWidget;
        Ui::PriceCurvePropertiesForm *priceCurvePropertiesForm;
        bool showComposition;
        QwtPlotPrice::CurveStyle compositionStyle;
        PriceChart::SourceType compositionSource;
        bool showOpen;
        PriceChart::SourceType openSource;
        bool showHigh;
        PriceChart::SourceType highSource;
        bool showLow;
        PriceChart::SourceType lowSource;
        bool showClose;
        PriceChart::SourceType closeSource;
        QList<PriceEntry> priceEntryList;
        SimulationPropertiesData simulationPropertiesData;
};

PriceChart::PriceChart(const IsinInfo &info, int timeScale, Strategie *strategie, const QString &configGroup) : QWidget(strategie)
{
    privateData = new PrivateData;
    privateData->trendIndicator = NULL;
    privateData->dominantCycleIndicator = NULL;
    privateData->trendVigorIndicator = NULL;
    privateData->plotManualIndicators = INT_MAX;
    privateData->processMode = PriceChart::View;
    privateData->ui = new Ui::PriceChart();
    privateData->ui->setupUi(this);
    delete privateData->ui->qwtPlot;

    privateData->simulationPropertiesData = simulationProperties->properties();
    connect(simulationProperties, SIGNAL(propertiesChanged()), this, SLOT(updateSimulationProperties()));

    privateData->transactionsTabWidget = new QTabWidget(this);
    privateData->indicatorStackWidet = new SlidingStackedWidget(this);

    privateData->trendIndicator = NULL;
    privateData->dominantCycleIndicator = NULL;
    privateData->trendVigorIndicator = NULL;
    privateData->configGroup = configGroup.isEmpty() ? configGroupKey() : configGroup;
    privateData->priceCurvePropertiesWidget = NULL;
    privateData->priceCurvePropertiesForm = NULL;
    privateData->indicatorCalculateCache =  new CalculateCache();
    connect(this, SIGNAL(dataAppended()), privateData->indicatorCalculateCache, SLOT(clearCache()));
    connect(this, SIGNAL(dataSet()), privateData->indicatorCalculateCache, SLOT(clearCache()));
    privateData->saveSettings = true;
    this->strategie = strategie;
    indexPlot = new Plot(this);
    privateData->plotList.append(indexPlot);
    privateData->ui->plotSplitter->addWidget(indexPlot);
    privateData->ui->plotSplitter->setStretchFactor(0, 2);
    connect(this, SIGNAL(calculateProfits()), indexPlot, SLOT(magnifieYAxis()));

    privateData->isinInfo = info;
    privateData->timeScale = timeScale;

    averagePriceComposition = new PlotMCurve(QString(privateData->isinInfo.name + " Gem. Prijs"), indexPlot);
    askPriceComposition = new PlotMCurve(QString(privateData->isinInfo.name + " ASK Prijs"), indexPlot);
    bidPriceComposition = new PlotMCurve(QString(privateData->isinInfo.name + " Bid Prijs"), indexPlot);

    averagePriceOpen = new PlotCurve(tr("Gem. Prijs Open"), indexPlot, PlotCurve::View);
    averagePriceOpen->setPlotOffset(0.2);
    averagePriceOpen->setZ(50);
    priceCurveList << averagePriceOpen;
    askPriceOpen = new PlotCurve(tr("ASK Prijs Open"), indexPlot, PlotCurve::View);
    askPriceOpen->setPlotOffset(0.2);
    askPriceOpen->setZ(50);
    priceCurveList << askPriceOpen;
    bidPriceOpen = new PlotCurve(tr("BID Prijs Open"), indexPlot, PlotCurve::View);
    bidPriceOpen->setPlotOffset(0.2);
    bidPriceOpen->setZ(50);
    priceCurveList << bidPriceOpen;

    averagePriceHigh = new PlotCurve(tr(" Gem. Prijs High"), indexPlot, PlotCurve::View);
    averagePriceHigh->setPlotOffset(0.8);
    averagePriceHigh->setZ(50);
    priceCurveList << averagePriceHigh;
    askPriceHigh = new PlotCurve(tr("ASK Prijs High"), indexPlot, PlotCurve::View);
    askPriceHigh->setPlotOffset(0.8);
    askPriceHigh->setZ(50);
    priceCurveList << askPriceHigh;
    bidPriceHigh = new PlotCurve(tr("BID Prijs High"), indexPlot, PlotCurve::View);
    bidPriceHigh->setPlotOffset(0.8);
    bidPriceHigh->setZ(50);
    priceCurveList << bidPriceHigh;

    averagePriceLow = new PlotCurve(tr("Gem. Prijs Low"), indexPlot, PlotCurve::View);
    averagePriceLow->setPlotOffset(0.8);
    averagePriceLow->setZ(50);
    priceCurveList << averagePriceLow;
    askPriceLow = new PlotCurve(tr("ASK Prijs Low"), indexPlot, PlotCurve::View);
    askPriceLow->setPlotOffset(0.8);
    askPriceLow->setZ(50);
    priceCurveList << askPriceLow;
    bidPriceLow = new PlotCurve(tr("BID Prijs Low"), indexPlot, PlotCurve::View);
    bidPriceLow->setPlotOffset(0.8);
    bidPriceLow->setZ(50);
    priceCurveList << bidPriceLow;

    averagePriceClose = new PlotCurve(tr("Gem. Prijs Close"), indexPlot, PlotCurve::View);
    averagePriceClose->setPlotOffset(0.8);
    averagePriceClose->setZ(50);
    priceCurveList << averagePriceClose;
    askPriceClose = new PlotCurve(tr("ASK Prijs Close"), indexPlot, PlotCurve::View);
    askPriceClose->setPlotOffset(0.8);
    askPriceClose->setZ(50);
    priceCurveList << askPriceClose;
    bidPriceClose = new PlotCurve(tr("BID Prijs Close"), indexPlot, PlotCurve::View);
    bidPriceClose->setPlotOffset(0.8);
    bidPriceClose->setZ(50);
    priceCurveList << bidPriceClose;

    readSettings();

    updatePriceCurveVisibility();

    privateData->tickMarker = new QwtPlotMarker();
    privateData->tickMarker->setLabelAlignment(Qt::AlignRight);
    privateData->tickMarker->setSymbol(QwtSymbol(QwtSymbol::LTriangle, QApplication::palette().color(QPalette::Highlight), QApplication::palette().color(QPalette::Text), QSize(15, 15)));
    connect(dataConnection, SIGNAL(notification(const QString &)), this, SLOT(appendDataToPlot(const QString &)));

    startCapital = /*100000;*/10000;
    fixedTransactionCosts = /*0.0;*/9.5;
    variableTransactionCosts = /*0;*/0.0025;
    minimumTransactionCosts = /*0.0;*/10.00;
    maximumTransactionCosts = /*0;*/150.00;
    averageSpread = /*0;*/0.0015;

    setStartDateAction = new QAction(this);
    setStartDateAction->setText(tr("Als start datum zetten"));
    setEndDateAction = new QAction(this);
    setEndDateAction->setText(tr("Als eind datum zetten"));

    indexPlot->tracker()->popMenu->addAction(setStartDateAction);
    indexPlot->tracker()->popMenu->addAction(setEndDateAction);
    connect(setStartDateAction, SIGNAL(triggered(bool)), this, SLOT(setStartDateFromPointer()));
    connect(setEndDateAction, SIGNAL(triggered(bool)), this, SLOT(setEndDateFromPointer()));
    connect(indexPlot->tracker(), SIGNAL(moved(const QPoint &)), this, SLOT(cursorMoved(const QPoint &)));

    privateData->strategieOverviewWidget = new StrategieOverview(this);
    connect(privateData->strategieOverviewWidget, SIGNAL(destroyed(QObject *)), this, SLOT(handleOverviewDestroyed()), Qt::QueuedConnection); //moet een queued connection zijn zodat de overview widget al verwijderd is als men deze prijskaart wilt verwijderen

    privateData->trendIndicator = new Trend(indexPlot, this, false, "", Indicator::Data);
    privateData->trendIndicator->setAutoRestore(false);
    privateData-> trendIndicator->readSettings();
    QApplication::processEvents();

    privateData->dominantCycleIndicator = new DominantCycle(NULL, this, false, "", Indicator::Hidden, false);
    privateData->dominantCycleIndicator->setAutoRestore(false);
    privateData->dominantCycleIndicator->readSettings();
    QApplication::processEvents();

    privateData->trendVigorIndicator = new TrendVigor(NULL, this, false, "", Indicator::Hidden, false);
    privateData->trendVigorIndicator->setAutoRestore(false);
    privateData->trendVigorIndicator->readSettings();
    QApplication::processEvents();

    setTimeScale(privateData->timeScale);
    QApplication::processEvents();

    writeSettings();
}

PriceChart::~PriceChart(void)
{
    if (aboutToQuit)
        writeSettings();
    else
        removeSettings();

    QString name;
    if (isinInfo().providesBidAsk)
        name = QString(isinInfo().isin + "_ASK_" + dataConnection->timeScaleIntToString(timeScale()));
    else
        name = QString(isinInfo().isin + "_" + dataConnection->timeScaleIntToString(timeScale()));

    dataConnection->unsubscribeFromNotification(name, this);

    while (!indicatorList.isEmpty())
    {
        Indicator *indicator = indicatorList.takeFirst();
        if (indicator->autoRestore() && indicator->processMode() != Indicator::Hidden)
        {
            indicator->writeSettings();
            indicator->writeRules();
        }

        delete indicator;
    }

    delete privateData->indicatorCalculateCache;
    if (privateData->strategieOverviewWidget)
        delete privateData->strategieOverviewWidget;
    delete privateData;
}

QList<Plot *> &PriceChart::plotList(void)
{
    return privateData->plotList;
}

const QList<Plot *> &PriceChart::plotList(void) const
{
    return privateData->plotList;
}

StrategieOverview *PriceChart::strategieOverviewWidget(void) const
{
    return privateData->strategieOverviewWidget;
}

SlidingStackedWidget *PriceChart::indicatorStackWidget(void) const
{
    return privateData->indicatorStackWidet;
}

QTabWidget *PriceChart::transactionTabWidget(void) const
{
    return privateData->transactionsTabWidget;
}

QSplitter *PriceChart::plotSplitterWidget(void) const
{
    return privateData->ui->plotSplitter;
}

int PriceChart::dominantCyclePeriod(double x)
{
    if (!privateData->dominantCycleIndicator)
        return -1;

    if (privateData->plotManualIndicators & (1 << 0))
    {
        privateData->dominantCycleIndicator->plotCurve();
        privateData->plotManualIndicators &= ~(1 << 0);
    }

    int address = static_cast<int>(x);
    if (address < 0 || address >= privateData->dominantCycleIndicator->dcpCurve->dataSize())
        return -1;

    return privateData->dominantCycleIndicator->dcpCurve->y(address);
}

bool PriceChart::trendAvailable(double x)
{
    if (!privateData->trendVigorIndicator)
        return true;

    if (privateData->plotManualIndicators & (1 << 1))
    {
        privateData->trendVigorIndicator->plotCurve();
        privateData->plotManualIndicators &= ~(1 << 1);
    }

    int address = static_cast<int>(x);
    if (address < 0 || address >= privateData->trendVigorIndicator->tvCurve->dataSize())
        return true;

    double tval = privateData->trendVigorIndicator->tvCurve->y(address);
    if (tval > 4.0 || tval < -4.0)
        return true;

    return false;
}

Indicator *PriceChart::trendIndicator(void)
{
    return privateData->trendIndicator;
}

Indicator *PriceChart::dominantCycleIndicator(void)
{
    return privateData->dominantCycleIndicator;
}

Indicator *PriceChart::trendVigorIndicator(void)
{
    return privateData->trendVigorIndicator;
}

void PriceChart::readSettings(void)
{
    if (configGroupKey().isEmpty())
        return;

    while (!settings.group().isEmpty())
        settings.endGroup();

    settings.beginGroup(QString("Strategie/Charts/" + configGroupKey()));

    privateData->plotSplitterSizes = settings.value("PlotSplitterSizes", QVariant(QList<QVariant>())).toList();
    //    privateData->isinInfo = settings.value("IsinInfo").value<IsinInfo>();
    privateData->timeScale = settings.value("TimeScale", HourByHour).toInt();
    privateData->showComposition = settings.value("ShowComposition", true).toBool();
    privateData->compositionStyle = static_cast<QwtPlotPrice::CurveStyle>(settings.value("CompositionStyle", QwtPlotPrice::Candlestick).toInt());
    privateData->compositionSource = static_cast<SourceType>(settings.value("CompositionSource", Average).toInt());
    privateData->showOpen = settings.value("ShowOpen", false).toBool();
    privateData->openSource = static_cast<SourceType>(settings.value("OpenSource", Average).toInt());
    privateData->showHigh = settings.value("ShowHigh", false).toBool();
    privateData->highSource = static_cast<SourceType>(settings.value("HighSource", Average).toInt());
    privateData->showLow = settings.value("ShowLow", false).toBool();
    privateData->lowSource = static_cast<SourceType>(settings.value("LowSource", Average).toInt());
    privateData->showClose = settings.value("ShowClose", false).toBool();
    privateData->closeSource = static_cast<SourceType>(settings.value("CloseSource", Average).toInt());

    QList<QVariant> defaultPriceCurveColorList;
    defaultPriceCurveColorList << Qt::white << Qt::white << Qt::white << Qt::green << Qt::green << Qt::green << Qt::red << Qt::red << Qt::red << Qt::black << Qt::black << Qt::black;
    QList<QVariant> defaultPriceCurveWidthList;
    defaultPriceCurveWidthList << 2 << 2 << 2 << 2 << 2 << 2 << 2 << 2 << 2 << 2 << 2 << 2;
    QList<QVariant> defaultPriceCurveStyleList;
    defaultPriceCurveStyleList << static_cast<int>(Qt::SolidLine) << static_cast<int>(Qt::SolidLine) << static_cast<int>(Qt::SolidLine) << static_cast<int>(Qt::SolidLine) << static_cast<int>(Qt::SolidLine) << static_cast<int>(Qt::SolidLine) << static_cast<int>(Qt::SolidLine) << static_cast<int>(Qt::SolidLine) << static_cast<int>(Qt::SolidLine) << static_cast<int>(Qt::SolidLine) << static_cast<int>(Qt::SolidLine) << static_cast<int>(Qt::SolidLine);
    QList<QVariant> defaultPriceCurveFillList;
    defaultPriceCurveFillList << Qt::NoBrush << Qt::NoBrush << Qt::NoBrush << Qt::NoBrush << Qt::NoBrush << Qt::NoBrush << Qt::NoBrush << Qt::NoBrush << Qt::NoBrush << Qt::NoBrush << Qt::NoBrush << Qt::NoBrush;

    QList<QVariant> priceCurveColorList = settings.value("PriceCurveColorList", defaultPriceCurveColorList).toList();
    QList<QVariant> priceCurveWidthList = settings.value("PriceCurveWidthList", defaultPriceCurveWidthList).toList();
    QList<QVariant> priceCurveStyleList = settings.value("PriceCurveStyleList", defaultPriceCurveStyleList).toList();
    QList<QVariant> priceCurveFillList = settings.value("PriceCurveFillList", defaultPriceCurveFillList).toList();

    for (int i = 0; i < priceCurveList.size(); i++)
    {
        PlotCurve *curve = priceCurveList.at(i);
        QPen pen = curve->pen();
        pen.setColor(priceCurveColorList.at(i).value<QColor>());
        pen.setWidth(priceCurveWidthList.at(i).toInt());
        pen.setStyle(static_cast<Qt::PenStyle>(priceCurveStyleList.at(i).toInt()));
        curve->setPen(pen);
        curve->setBrush(priceCurveFillList.at(i).value<QBrush>());
    }
}

void PriceChart::writeSettings(void)
{
    while (!settings.group().isEmpty())
        settings.endGroup();

    settings.beginGroup(QString("Strategie/Charts/" + configGroupKey()));

    bool writePlotSplitterSizes = true;
    QList<QVariant> splitterSizes;
    if (privateData->ui->plotSplitter->sizes().size() > 1)
    {
        for (int i = 0; i < privateData->ui->plotSplitter->sizes().size(); ++i)
        {
            if (i == 0 && privateData->ui->plotSplitter->sizes().at(i) == 0)
            {
                writePlotSplitterSizes = false;
                break;
            }

            splitterSizes << QVariant(privateData->ui->plotSplitter->sizes().at(i));
        }
    }
    else
        writePlotSplitterSizes = false;

    if (writePlotSplitterSizes)
        settings.setValue("PlotSplitterSizes", QVariant(splitterSizes));

    settings.setValue("IsinInfo", qVariantFromValue(privateData->isinInfo));
    settings.setValue("Isin", privateData->isinInfo.isin);
    settings.setValue("TimeScale", timeScale());
    settings.setValue("ShowComposition", privateData->showComposition);
    settings.setValue("CompositionStyle", privateData->compositionStyle);
    settings.setValue("CompositionSource", privateData->compositionSource);
    settings.setValue("ShowOpen", privateData->showOpen);
    settings.setValue("OpenSource", privateData->openSource);
    settings.setValue("ShowHigh", privateData->showHigh);
    settings.setValue("HighSource", privateData->highSource);
    settings.setValue("ShowLow", privateData->showLow);
    settings.setValue("LowSource", privateData->lowSource);
    settings.setValue("ShowClose", privateData->showClose);
    settings.setValue("CloseSource", privateData->closeSource);

    QList<QVariant> priceCurveColorList;
    QList<QVariant> priceCurveWidthList;
    QList<QVariant> priceCurveStyleList;
    QList<QVariant> priceCurveFillList;

    for (int i = 0; i < priceCurveList.size(); i++)
    {
        QPen pen = priceCurveList.at(i)->pen();
        priceCurveColorList.append(pen.color());
        priceCurveWidthList.append(pen.width());
        priceCurveStyleList.append(static_cast<int>(pen.style()));
        priceCurveFillList.append(priceCurveList.at(i)->brush());
    }

    settings.setValue("PriceCurveColorList", priceCurveColorList);
    settings.setValue("PriceCurveWidthList", priceCurveWidthList);
    settings.setValue("PriceCurveStyleList", priceCurveStyleList);
    settings.setValue("PriceCurveFillList", priceCurveFillList);
}

void PriceChart::removeSettings(void)
{
    while (!settings.group().isEmpty())
        settings.endGroup();

    settings.beginGroup("Strategie/Charts/");
    settings.remove(configGroupKey());
}

QString &PriceChart::configGroupKey(void)
{
    if (!privateData->configGroup.isEmpty())
        return privateData->configGroup;

    QString oldGroup = settings.group();

    while (!settings.group().isEmpty())
        settings.endGroup();

    settings.beginGroup("Strategie");
    settings.beginGroup("Charts");

    QString configGroup = "Chart";
    QStringList chartList = settings.childGroups();
    for (int i = 0; i < INT_MAX; ++i)
    {
        if (i == 0)
            configGroup.append(QString::number(i));
        else
            configGroup.replace(QString::number(i - 1), QString::number(i));

        if (!chartList.contains(configGroup))
        {
            privateData->configGroup = configGroup;
            break;
        }
    }

    while (!settings.group().isEmpty())
        settings.endGroup();

    settings.beginGroup(oldGroup);

    return privateData->configGroup;
}

const QColor &PriceChart::takeColor(const QColor &color)
{
    return privateData->colorPool.takeColor(color);
}

void PriceChart::putColor(const QColor &color)
{
    privateData->colorPool.putColor(color);
}


PriceChart::ProcessMode PriceChart::processMode() const
{
    return privateData->processMode;
}

void PriceChart::setProcessMode(PriceChart::ProcessMode mode)
{
    if (mode == privateData->processMode)
        return;

    privateData->processMode = mode;

    if (processMode() == PriceChart::View)
    {
        indexPlot->setPlottingDisabled(false);
        indexPlot->replot();
    }
    else
        indexPlot->setPlottingDisabled(true);

    for (int i = 0; i < indicatorList.size(); i++)
    {
        Indicator *indicator = indicatorList.at(i);
        if (indicator == privateData->trendIndicator ||
            indicator == privateData->dominantCycleIndicator ||
            indicator == privateData->trendVigorIndicator || indicator->autoOptimizerMode())
            continue;

        indicator->setProcessMode(static_cast<Indicator::ProcessMode>(processMode()));
    }
}

QList<Indicator *> PriceChart::restoreIndicators(QStringList indicatorNumberList, PriceChart::ProcessMode processMode)
{
    Indicator::ProcessMode mode = static_cast<Indicator::ProcessMode>(processMode);

    bool plottingDisabled = indexPlot->plottingDisabled();
    if (mode == Indicator::View)
        indexPlot->setPlottingDisabled(true);

    while (!settings.group().isEmpty())
        settings.endGroup();

    settings.beginGroup(QString("Strategie/Charts/") + configGroupKey() + QString("/Indicators"));
    QStringList indicators = settings.childGroups();

    QList<Indicator *> restoredIndicatorList;
    for (int i = 0; i < indicatorNumberList.size(); i++)
    {
        QString indicatorNumber = indicatorNumberList.at(i);
        Indicator *indicator;
        int indicatorType;
        for (int i = 0; i < indicators.size(); ++i)
        {
            if (!indicators.at(i).startsWith("Indicator"))
                continue;

            if (!indicators.at(i).contains(indicatorNumber))
                continue;

            while (!settings.group().isEmpty())
                settings.endGroup();

            settings.beginGroup(QString("Strategie/Charts/" + configGroupKey() + "/Indicators/"));

            if (settings.value(QString(indicators.at(i) + "/AutoRestore")).toBool() == false)
                continue;

            indicatorType = settings.value(QString(indicators.at(i) + "/Type"), -1).toInt();
            switch (indicatorType)
            {
                case MovingAverageIndicator:
                    indicator = new MovingAverageCurve(indexPlot, this, true, indicators.at(i), mode);
                    break;

                case TwoMovingAveragesIndicator:
                    indicator = new TwoMovingAverageCurve(indexPlot, this, true, indicators.at(i), mode);
                    break;

                case ThreeMovingAveragesIndicator:
                    indicator = new ThreeMovingAverageCurve(indexPlot, this, true, indicators.at(i), mode);
                    break;

                case TrendLineIndicator:
                    indicator = new Line(indexPlot, this, Line::Free, true, indicators.at(i), mode);
                    break;

                case SupportCeilingLineIndicator:
                    indicator = new Line(indexPlot, this, Line::Horizontal, true, indicators.at(i), mode);
                    break;

                case BollingerBandsIndicator:
                    indicator = new BollingerBands(indexPlot, this, true, indicators.at(i), mode);
                    break;

                case PriceChannelIndicator:
                    indicator = new PriceChannel(indexPlot, this, true, indicators.at(i), mode);
                    break;

                case ITrendIndicator:
                    indicator = new InstantaneousTrend(indexPlot, this, true, indicators.at(i), mode);
                    break;

                case RSIIndicator:
                    indicator = new RSI(NULL, this, true, indicators.at(i), mode);
                    break;

                case StochasticIndicator:
                    indicator = new Stochastic(NULL, this, true, indicators.at(i), mode);
                    break;

                case ADXIndicator:
                    indicator = new ADX(NULL, this, true, indicators.at(i), mode);
                    break;

                case DominantCycleIndicator:
                    indicator = new DominantCycle(NULL, this, true, indicators.at(i), mode);
                    break;

                case TrendVigorIndicator:
                    indicator = new TrendVigor(NULL, this, true, indicators.at(i), mode);
                    break;

                case SNRIndicator:
                    indicator = new SNR(NULL, this, true, indicators.at(i), mode);
                    break;

                case VolatilityIndicator:
                    indicator = new Volatility(NULL, this, true, indicators.at(i), mode);
                    break;

                case ERIndicator:
                    indicator = new ER(NULL, this, true, indicators.at(i), mode);
                    break;

                default:
                    continue;
            }

            restoredIndicatorList << indicator;

            if (indicatorNumber != "")
                break;
        }
    }

    for (int i = 0; i < restoredIndicatorList.size(); i++)
    {
        Indicator *indicator = restoredIndicatorList.at(i);
        finishCreateIndicator(indicator);
    }

    while (!settings.group().isEmpty())
        settings.endGroup();

    QList<int> splitterSizes;
    for (int i = 0; i < privateData->plotSplitterSizes.size(); ++i)
        splitterSizes << privateData->plotSplitterSizes.at(i).toInt();
    privateData->ui->plotSplitter->setSizes(splitterSizes);

    if (mode == Indicator::View)
    {
        indexPlot->setPlottingDisabled(plottingDisabled);
        indexPlot->replot();
    }

    return restoredIndicatorList;
}

void PriceChart::finishCreateIndicator(Indicator *indicator)
{
    if (!indicator)
        return;

    if (indicator->useOwnPlot && indicator->processMode() != Indicator::Hidden)
    {
        strategie->updateDockSpacing();
        connect(indicator, SIGNAL(destroyed()), strategie, SLOT(updateDockSpacing()));
        connect(indicator->indicatorPlot->tracker(), SIGNAL(movedFromTo(QPoint, QPoint)), strategie, SLOT(updateDockBackground(QPoint, QPoint)));
    }

    indicator->readSettings();
    indicator->setupPropertiesWidget();
    indicator->readRules();
    indicator->updateGUI();
    indicator->writeSettings();
    indicator->writeRules();
    indicator->plotCurve();
    indicator->executeTransactionRules();
    if (indicator->processMode() == Indicator::View)
        indicator->indicatorPlot->replot();
}

void PriceChart::setStartDateFromPointer(void)
{
    int x = indexPlot->tracker()->xClicked;
    if (x < 0)
        x = 0;

    int address = indexPlot->findAddressForX(x);
    if (address < 0 || address >= indexPlot->timeScaleData().size())
        address = indexPlot->timeScaleData().size() - 1;

    QDateTime startDateTime = indexPlot->timeScaleData().at(address);
    if (!startDateTime.isValid())
        return;

    QDateTime endDateTime = indexPlot->timeScaleData().last();
    if (!endDateTime.isValid())
        return;

    loadData(startDateTime, endDateTime);
}

void PriceChart::setEndDateFromPointer(void)
{
    int x = indexPlot->tracker()->xClicked;
    if (x < 0)
        x = 0;

    int address = indexPlot->findAddressForX(x);
    if (address < 0 || address >= indexPlot->timeScaleData().size())
        address = indexPlot->timeScaleData().size() - 1;

    QDateTime endDateTime = indexPlot->timeScaleData().at(address);
    if (!endDateTime.isValid())
        return;

    QDateTime startDateTime = indexPlot->timeScaleData().first();
    if (!startDateTime.isValid())
        return;

    loadData(startDateTime, endDateTime);
}

void PriceChart::cursorMoved(const QPoint &pos)
{
    if (averagePriceClose->dataSize() < 1)
        return;

    int x = (int) indexPlot->invTransform(QwtPlot::xBottom, pos.x());
    x = x - averagePriceClose->x(0);
    if (x < 0 || x >= indexPlot->timeScaleData().size())
        return;

    QString message;
    message.sprintf("%s Open=%g High=%g Low=%g Close=%g X=%d", indexPlot->timeScaleData().at(x).toString().toAscii().data(),
                    averagePriceOpen->y(x),
                    averagePriceHigh->y(x),
                    averagePriceLow->y(x),
                    averagePriceClose->y(x),
                    x);

    mainWindow->statusBar()->showMessage(message);
}

void PriceChart::handleOverviewDestroyed(void)
{
    emit removeRequested(this);
}

CalculateCache *PriceChart::indicatorCalculateCache(void) const
{
    return privateData->indicatorCalculateCache;
}

void PriceChart::handleGetFirstAvailableDateTimeRequest(QDateTime &dateTime)
{
    if (!dateTime.isValid() || dateTime.isNull())
        return;

    QDateTime oldFirstAvailableDateTime = firstAvailableDateTime();
    privateData->firstAvailableDateTime = dateTime;
    if (oldFirstAvailableDateTime != firstAvailableDateTime())
        emit firstAvailableDateTimeChanged(firstAvailableDateTime());
}

void PriceChart::handleGetLastAvailableDateTimeRequest(QDateTime &dateTime)
{
    if (!dateTime.isValid() || dateTime.isNull())
        return;

    QDateTime oldLastAvailableDateTime = lastAvailableDateTime();
    privateData->lastAvailableDateTime = dateTime;
    if (oldLastAvailableDateTime != lastAvailableDateTime())
        emit lastAvailableDateTimeChanged(lastAvailableDateTime());
}

void PriceChart::doLoadData(QList<PriceEntry> &priceEntrylist)
{
    privateData->priceEntryList = priceEntrylist;

    float averageSpread = 0.0f;
    for (int i = 0; i < privateData->priceEntryList.size(); i++)
    {
        PriceEntry priceEntry = privateData->priceEntryList.at(i);
        averageSpread += fabsf(priceEntry.openAsk - priceEntry.openBid);
        averageSpread += fabsf(priceEntry.highAsk - priceEntry.highBid);
        averageSpread += fabsf(priceEntry.lowAsk - priceEntry.lowBid);
        averageSpread += fabsf(priceEntry.closeAsk - priceEntry.closeBid);
    }
    averageSpread = averageSpread / static_cast<float>(privateData->priceEntryList.size() * 4);

    float correction = 0.0f;
    if (privateData->simulationPropertiesData.spreadMode == SimulationProperties::Fill)
    {
        if (privateData->simulationPropertiesData.transactionCostsMode == SimulationProperties::Pips)
        {
            if ((privateData->simulationPropertiesData.spread / 10000.0f) <= averageSpread)
                correction = 0.0f;
            else
                correction = ((privateData->simulationPropertiesData.spread / 10000.0f) - averageSpread) * 0.5f;
        }
        else
        {
            if (privateData->simulationPropertiesData.spread <= averageSpread)
                correction = 0.0f;
            else
                correction = (privateData->simulationPropertiesData.spread - averageSpread) * 0.5f;
        }
    }
    else if (privateData->simulationPropertiesData.spreadMode == SimulationProperties::Fixed)
    {
        if (privateData->simulationPropertiesData.transactionCostsMode == SimulationProperties::Pips)
            correction = (privateData->simulationPropertiesData.spread / 10000.0f) * 0.5;
        else
            correction = privateData->simulationPropertiesData.spread * 0.5;
    }

    QVector<QDateTime> scaleData;
    QVector<QwtPriceMove> averageCompositionData;
    QVector<QwtPriceMove> askCompositionData;
    QVector<QwtPriceMove> bidCompositionData;
    QVector<QPointF> averageOpenData;
    QVector<QPointF> askOpenData;
    QVector<QPointF> bidOpenData;
    QVector<QPointF> averageHighData;
    QVector<QPointF> askHighData;
    QVector<QPointF> bidHighData;
    QVector<QPointF> averageLowData;
    QVector<QPointF> askLowData;
    QVector<QPointF> bidLowData;
    QVector<QPointF> averageCloseData;
    QVector<QPointF> askCloseData;
    QVector<QPointF> bidCloseData;
    for (int i = 0; i < privateData->priceEntryList.size(); i++)
    {
        PriceEntry priceEntry = privateData->priceEntryList.at(i);
        if (privateData->simulationPropertiesData.spreadMode == SimulationProperties::Fill)
        {
            priceEntry.openAsk += correction;
            priceEntry.openBid -= correction;
            priceEntry.highAsk += correction;
            priceEntry.highBid -= correction;
            priceEntry.lowAsk += correction;
            priceEntry.lowBid -= correction;
            priceEntry.closeAsk += correction;
            priceEntry.closeBid -= correction;
        }
        else if (privateData->simulationPropertiesData.spreadMode == SimulationProperties::Fixed && !qFuzzyIsNull(privateData->simulationPropertiesData.spread))
        {
            priceEntry.openAsk = priceEntry.openAverage + correction;
            priceEntry.openBid = priceEntry.openAverage - correction;
            priceEntry.highAsk = priceEntry.highAverage + correction;
            priceEntry.highBid = priceEntry.highAverage - correction;
            priceEntry.lowAsk = priceEntry.lowAverage + correction;
            priceEntry.lowBid = priceEntry.lowAverage - correction;
            priceEntry.closeAsk = priceEntry.closeAverage + correction;
            priceEntry.closeBid = priceEntry.closeAverage - correction;
        }

        scaleData.append(priceEntry.dateTime);
        QwtPriceMove averagePriceMove(QwtDoubleInterval(i, i + 1), priceEntry.openAverage, priceEntry.highAverage, priceEntry.lowAverage, priceEntry.closeAverage);
        averageCompositionData.append(averagePriceMove);
        QwtPriceMove askPriceMove(QwtDoubleInterval(i, i + 1), priceEntry.openAsk, priceEntry.highAsk, priceEntry.lowAsk, priceEntry.closeAsk);
        askCompositionData.append(askPriceMove);
        QwtPriceMove bidPriceMove(QwtDoubleInterval(i, i + 1), priceEntry.openBid, priceEntry.highBid, priceEntry.lowBid, priceEntry.closeBid);
        bidCompositionData.append(bidPriceMove);

        averageOpenData.append(QPointF(i, priceEntry.openAverage));
        askOpenData.append(QPointF(i, priceEntry.openAsk));
        bidOpenData.append(QPointF(i, priceEntry.openBid));

        averageHighData.append(QPointF(i, priceEntry.highAverage));
        askHighData.append(QPointF(i, priceEntry.highAsk));
        bidHighData.append(QPointF(i, priceEntry.highBid));

        averageLowData.append(QPointF(i, priceEntry.lowAverage));
        askLowData.append(QPointF(i, priceEntry.lowAsk));
        bidLowData.append(QPointF(i, priceEntry.lowBid));

        averageCloseData.append(QPointF(i, priceEntry.closeAverage));
        askCloseData.append(QPointF(i, priceEntry.closeAsk));
        bidCloseData.append(QPointF(i, priceEntry.closeBid));
    }

    bool plDisabled = indexPlot->plottingDisabled();
    indexPlot->setPlottingDisabled(true);

    averagePriceComposition->clearData();
    askPriceComposition->clearData();
    bidPriceComposition->clearData();

    averagePriceOpen->clearData();
    askPriceOpen->clearData();
    bidPriceOpen->clearData();

    averagePriceHigh->clearData();
    askPriceHigh->clearData();
    bidPriceHigh->clearData();

    averagePriceLow->clearData();
    askPriceLow->clearData();
    bidPriceLow->clearData();

    averagePriceClose->clearData();
    askPriceClose->clearData();
    bidPriceClose->clearData();

    indexPlot->setPeriods(privateData->priceEntryList.size());
    averagePriceComposition->setData(averageCompositionData);
    askPriceComposition->setData(askCompositionData);
    bidPriceComposition->setData(bidCompositionData);

    averagePriceOpen->setData(averageOpenData);
    askPriceOpen->setData(askOpenData);
    bidPriceOpen->setData(bidOpenData);

    averagePriceHigh->setData(averageHighData);
    askPriceHigh->setData(askHighData);
    bidPriceHigh->setData(bidHighData);

    averagePriceLow->setData(averageLowData);
    askPriceLow->setData(askLowData);
    bidPriceLow->setData(bidLowData);

    averagePriceClose->setData(averageCloseData);
    askPriceClose->setData(askCloseData);
    bidPriceClose->setData(bidCloseData);

    if (indexPlot->zoomer()->zoomRectIndex() != 0)
        indexPlot->zoomReset();

    QString format;
    if (timeScale() == DayByDay || timeScale() == WeekByWeek)
        format = "dd MMM yy";
    else if (timeScale() == MonthByMonth)
        format = "MMM yy";
    else
        format = "dd MMM yy";

    for (int i = 0; i < plotList().size(); i++)
    {
        Plot *pricePlot = plotList().at(i);
        pricePlot->setTimeScaleData(scaleData);
        pricePlot->setTimeScaleFormat(format);
    }

    QDateTime oldStartDateTime = (indexPlot->timeScaleData().size() >= 1) ? indexPlot->timeScaleData().first() : QDateTime();
    QDateTime oldEndDateTime = (indexPlot->timeScaleData().size() >= 1) ? indexPlot->timeScaleData().last() : QDateTime();
    if (((indexPlot->timeScaleData().size() >= 1) ? indexPlot->timeScaleData().first() : QDateTime()) != oldStartDateTime)
        emit startDateTimeChanged(((indexPlot->timeScaleData().size() >= 1) ? indexPlot->timeScaleData().first() : QDateTime()));

    if (((indexPlot->timeScaleData().size() >= 1) ? indexPlot->timeScaleData().last() : QDateTime()) != oldEndDateTime)
        emit endDateTimeChanged(((indexPlot->timeScaleData().size() >= 1) ? indexPlot->timeScaleData().last() : QDateTime()));

    indexPlot->setXAxisScaleToCurve(*averagePriceClose);

    if (!priceEntrylist.isEmpty() && priceEntrylist.last().dateTime >= lastAvailableDateTime())
        strategieOverviewWidget()->updatePriceQuote(priceEntrylist.last());

    emit dataSet();

    privateData->plotManualIndicators = INT_MAX;
    indexPlot->setPlottingDisabled(plDisabled);
    indexPlot->replot(); // adding a transaction marker needs the proper axis scales, these are updated by replot.

    emit calculateProfits();
}

void PriceChart::loadData(QDateTime startDateTime, QDateTime endDateTime, bool cached)
{
    if (!cached)
        dataConnection->getPriceEntries(this, SLOT(doLoadData(QList<PriceEntry> &)), isinInfo(), startDateTime, endDateTime, timeScale());
    else
        doLoadData(privateData->priceEntryList);

    dataConnection->getFirstAvailableDateTime(this, SLOT(handleGetFirstAvailableDateTimeRequest(QDateTime &)), isinInfo(), timeScale());
    dataConnection->getLastAvailableDateTime(this, SLOT(handleGetLastAvailableDateTimeRequest(QDateTime &)), isinInfo(), timeScale());
}

void PriceChart::doAppendDataToPlot(QList<PriceEntry> &priceEntryList)
{
    QDateTime oldStartDateTime = (indexPlot->timeScaleData().size() >= 1) ? indexPlot->timeScaleData().first() : QDateTime();
    QDateTime oldEndDateTime = (indexPlot->timeScaleData().size() >= 1) ? indexPlot->timeScaleData().last() : QDateTime();

    float averageSpread = 0.0f;
    for (int i = 0; i < privateData->priceEntryList.size(); i++)
    {
        PriceEntry priceEntry = privateData->priceEntryList.at(i);
        averageSpread += fabsf(priceEntry.openAsk - priceEntry.openBid);
        averageSpread += fabsf(priceEntry.highAsk - priceEntry.highBid);
        averageSpread += fabsf(priceEntry.lowAsk - priceEntry.lowBid);
        averageSpread += fabsf(priceEntry.closeAsk - priceEntry.closeBid);
    }
    averageSpread = averageSpread / static_cast<float>(privateData->priceEntryList.size() * 4);

    float correction = 0.0f;
    if (privateData->simulationPropertiesData.spreadMode == SimulationProperties::Fill)
    {
        if (privateData->simulationPropertiesData.transactionCostsMode == SimulationProperties::Pips)
        {
            if ((privateData->simulationPropertiesData.spread / 10000.0f) <= averageSpread)
                correction = 0.0f;
            else
                correction = ((privateData->simulationPropertiesData.spread / 10000.0f) - averageSpread) * 0.5f;
        }
        else
        {
            if (privateData->simulationPropertiesData.spread <= averageSpread)
                correction = 0.0f;
            else
                correction = (privateData->simulationPropertiesData.spread - averageSpread) * 0.5f;
        }
    }
    else if (privateData->simulationPropertiesData.spreadMode == SimulationProperties::Fixed)
    {
        if (privateData->simulationPropertiesData.transactionCostsMode == SimulationProperties::Pips)
            correction = (privateData->simulationPropertiesData.spread / 10000.0f) * 0.5;
        else
            correction = privateData->simulationPropertiesData.spread * 0.5;
    }

    bool replottingDisabled = indexPlot->plottingDisabled();
    indexPlot->setPlottingDisabled(true);

    for (int i = 0; i < priceEntryList.size(); i++)
    {
        PriceEntry priceEntry = priceEntryList.at(i);
        if (indexPlot->timeScaleData().last() >= privateData->lastAvailableDateTime)
        {
            double x;
            if (averagePriceClose->dataSize() >= 1)
                x = averagePriceOpen->x(averagePriceClose->dataSize() - 1);
            else
                x = 0.0;

            bool replace = true;
            if (indexPlot->timeScaleData().size() >= 1)
            {
                if (priceEntry.dateTime > indexPlot->timeScaleData().last())
                {
                    replace = false;
                    x = x + 1.0;
                }
                else if (priceEntry.dateTime < indexPlot->timeScaleData().last())
                    continue;
            }

            if (privateData->simulationPropertiesData.spreadMode == SimulationProperties::Fill)
            {
                priceEntry.openAsk += correction;
                priceEntry.openBid -= correction;
                priceEntry.highAsk += correction;
                priceEntry.highBid -= correction;
                priceEntry.lowAsk += correction;
                priceEntry.lowBid -= correction;
                priceEntry.closeAsk += correction;
                priceEntry.closeBid -= correction;
            }
            else if (privateData->simulationPropertiesData.spreadMode == SimulationProperties::Fixed && !qFuzzyIsNull(privateData->simulationPropertiesData.spread))
            {
                priceEntry.openAsk = priceEntry.openAverage + correction;
                priceEntry.openBid = priceEntry.openAverage - correction;
                priceEntry.highAsk = priceEntry.highAverage + correction;
                priceEntry.highBid = priceEntry.highAverage - correction;
                priceEntry.lowAsk = priceEntry.lowAverage + correction;
                priceEntry.lowBid = priceEntry.lowAverage - correction;
                priceEntry.closeAsk = priceEntry.closeAverage + correction;
                priceEntry.closeBid = priceEntry.closeAverage - correction;
            }

            QwtPriceMove averagePriceMove;
            averagePriceMove.interval = QwtDoubleInterval(x, x + 1);
            averagePriceMove.open = priceEntry.openAverage;
            averagePriceMove.close = priceEntry.closeAverage;
            averagePriceMove.high = priceEntry.highAverage;
            averagePriceMove.low = priceEntry.lowAverage;

            QwtPriceMove askPriceMove;
            askPriceMove.interval = QwtDoubleInterval(x, x + 1);
            askPriceMove.open = priceEntry.openAsk;
            askPriceMove.close = priceEntry.closeAsk;
            askPriceMove.high = priceEntry.highAsk;
            askPriceMove.low = priceEntry.lowAsk;

            QwtPriceMove bidPriceMove;
            bidPriceMove.interval = QwtDoubleInterval(x, x + 1);
            bidPriceMove.open = priceEntry.openBid;
            bidPriceMove.close = priceEntry.closeBid;
            bidPriceMove.high = priceEntry.highBid;
            bidPriceMove.low = priceEntry.lowBid;

            if (replace)
            {
                privateData->priceEntryList.takeLast();
                privateData->priceEntryList.append(priceEntry);

                averagePriceComposition->replaceData(x, averagePriceMove);
                askPriceComposition->replaceData(x, askPriceMove);
                bidPriceComposition->replaceData(x, bidPriceMove);

                averagePriceOpen->replaceData(x, QPointF(x, priceEntry.openAverage));
                askPriceOpen->replaceData(x, QPointF(x, priceEntry.openAsk));
                bidPriceOpen->replaceData(x, QPointF(x, priceEntry.openBid));

                averagePriceHigh->replaceData(x, QPointF(x, priceEntry.highAverage));
                askPriceHigh->replaceData(x, QPointF(x, priceEntry.highAsk));
                bidPriceHigh->replaceData(x, QPointF(x, priceEntry.highBid));

                averagePriceLow->replaceData(x, QPointF(x, priceEntry.lowAverage));
                askPriceLow->replaceData(x, QPointF(x, priceEntry.lowAsk));
                bidPriceLow->replaceData(x, QPointF(x, priceEntry.lowBid));

                averagePriceClose->replaceData(x, QPointF(x, priceEntry.closeAverage));
                askPriceClose->replaceData(x, QPointF(x, priceEntry.closeAsk));
                bidPriceClose->replaceData(x, QPointF(x, priceEntry.closeBid));
            }
            else
            {
                privateData->priceEntryList.append(priceEntry);

                averagePriceComposition->appendData(averagePriceMove);
                averagePriceComposition->setXAxisStartPoint(0);
                askPriceComposition->appendData(askPriceMove);
                askPriceComposition->setXAxisStartPoint(0);
                bidPriceComposition->appendData(bidPriceMove);
                bidPriceComposition->setXAxisStartPoint(0);

                averagePriceOpen->appendData(QPointF(x, priceEntry.openAverage));
                averagePriceOpen->setXAxisStartPoint(0);
                askPriceOpen->appendData(QPointF(x, priceEntry.openAsk));
                askPriceOpen->setXAxisStartPoint(0);
                bidPriceOpen->appendData(QPointF(x, priceEntry.openBid));
                bidPriceOpen->setXAxisStartPoint(0);

                averagePriceHigh->appendData(QPointF(x, priceEntry.highAverage));
                averagePriceHigh->setXAxisStartPoint(0);
                askPriceHigh->appendData(QPointF(x, priceEntry.highAsk));
                askPriceHigh->setXAxisStartPoint(0);
                bidPriceHigh->appendData(QPointF(x, priceEntry.highBid));
                bidPriceHigh->setXAxisStartPoint(0);

                averagePriceLow->appendData(QPointF(x, priceEntry.lowAverage));
                averagePriceLow->setXAxisStartPoint(0);
                askPriceLow->appendData(QPointF(x, priceEntry.lowAsk));
                askPriceLow->setXAxisStartPoint(0);
                bidPriceLow->appendData(QPointF(x, priceEntry.lowBid));
                bidPriceLow->setXAxisStartPoint(0);

                averagePriceClose->appendData(QPointF(x, priceEntry.closeAverage));
                averagePriceClose->setXAxisStartPoint(0);
                askPriceClose->appendData(QPointF(x, priceEntry.closeAsk));
                askPriceClose->setXAxisStartPoint(0);
                bidPriceClose->appendData(QPointF(x, priceEntry.closeBid));
                bidPriceClose->setXAxisStartPoint(0);

                for (int i = 0; i < plotList().size(); i++)
                {
                    Plot *pricePlot = plotList().at(i);
                    pricePlot->appendTimeScaleData(priceEntry.dateTime);
                }
            }
        }

        if (priceEntry.dateTime >= privateData->lastAvailableDateTime)
            strategieOverviewWidget()->updatePriceQuote(priceEntry);

        if (priceEntry.dateTime > privateData->lastAvailableDateTime)
        {
            privateData->lastAvailableDateTime = priceEntry.dateTime;
            emit lastAvailableDateTimeChanged(privateData->lastAvailableDateTime);
        }
    }

    if (((indexPlot->timeScaleData().size() >= 1) ? indexPlot->timeScaleData().first() : QDateTime()) != oldStartDateTime)
        emit startDateTimeChanged(((indexPlot->timeScaleData().size() >= 1) ? indexPlot->timeScaleData().first() : QDateTime()));

    if (((indexPlot->timeScaleData().size() >= 1) ? indexPlot->timeScaleData().last() : QDateTime()) != oldEndDateTime)
        emit endDateTimeChanged(((indexPlot->timeScaleData().size() >= 1) ? indexPlot->timeScaleData().last() : QDateTime()));

    if ((indexPlot->timeScaleData().size() >= 1) && indexPlot->timeScaleData().last() < privateData->lastAvailableDateTime)
        privateData->tickMarker->detach();
    else
    {
        if (averagePriceClose->dataSize() >= 1)
        {
            privateData->tickMarker->setValue(QwtDoublePoint(averagePriceClose->x(averagePriceClose->dataSize() - 1) + 1, averagePriceClose->y(averagePriceClose->dataSize() - 1)));
            privateData->tickMarker->setLabel(QString::number(averagePriceClose->y(averagePriceClose->dataSize() - 1), 'g'));
        }

        privateData->tickMarker->attach(indexPlot);
    }

    emit dataAppended();

    privateData->plotManualIndicators = INT_MAX;
    indexPlot->setPlottingDisabled(replottingDisabled);
    indexPlot->replot(); // adding a transaction marker needs the proper axis scales, these are updated by replot.

    emit calculateProfits();
}

void PriceChart::appendDataToPlot(const QString &notifier)
{
    QString isin = notifier.section("_", 0, 0);
    if (isin != isinInfo().isin)
        return;

    if (dataConnection->timeScaleStringToInt(notifier.section("_", -1, -1)) != timeScale())
        return;

    dataConnection->getPriceEntries(this, SLOT(doAppendDataToPlot(QList<PriceEntry> &)), isinInfo(), indexPlot->timeScaleData().last(), QDateTime(), timeScale());
}

QWidget *PriceChart::priceCurvePropertiesWidget(void)
{
    if (!privateData->priceCurvePropertiesWidget)
    {
        privateData->priceCurvePropertiesWidget = new QWidget(this);
        privateData->priceCurvePropertiesForm = new Ui::PriceCurvePropertiesForm();
        privateData->priceCurvePropertiesForm->setupUi(privateData->priceCurvePropertiesWidget);

        connect(privateData->priceCurvePropertiesForm->showComposistionCheckBox, SIGNAL(clicked()), this, SLOT(setPriceProperties()));
        connect(privateData->priceCurvePropertiesForm->compositionStyleComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setPriceProperties()));
        connect(privateData->priceCurvePropertiesForm->compositionSourceComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setPriceProperties()));
        connect(privateData->priceCurvePropertiesForm->showOpenCheckBox, SIGNAL(clicked()), this, SLOT(setPriceProperties()));
        connect(privateData->priceCurvePropertiesForm->openSourceComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setPriceProperties()));
        connect(privateData->priceCurvePropertiesForm->showHighCheckBox, SIGNAL(clicked()), this, SLOT(setPriceProperties()));
        connect(privateData->priceCurvePropertiesForm->highSourceComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setPriceProperties()));
        connect(privateData->priceCurvePropertiesForm->showLowCheckBox, SIGNAL(clicked()), this, SLOT(setPriceProperties()));
        connect(privateData->priceCurvePropertiesForm->lowSourceComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setPriceProperties()));
        connect(privateData->priceCurvePropertiesForm->showCloseCheckBox, SIGNAL(clicked()), this, SLOT(setPriceProperties()));
        connect(privateData->priceCurvePropertiesForm->closeSourceComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setPriceProperties()));
    }

    privateData->priceCurvePropertiesForm->showComposistionCheckBox->blockSignals(true);
    privateData->priceCurvePropertiesForm->showComposistionCheckBox->setChecked(privateData->showComposition);
    privateData->priceCurvePropertiesForm->showComposistionCheckBox->blockSignals(false);

    privateData->priceCurvePropertiesForm->compositionStyleComboBox->blockSignals(true);
    privateData->priceCurvePropertiesForm->compositionStyleComboBox->setCurrentIndex(privateData->compositionStyle - 1);
    privateData->priceCurvePropertiesForm->compositionStyleComboBox->blockSignals(false);

    privateData->priceCurvePropertiesForm->compositionSourceComboBox->blockSignals(true);
    privateData->priceCurvePropertiesForm->compositionSourceComboBox->setCurrentIndex(privateData->compositionSource);
    privateData->priceCurvePropertiesForm->compositionSourceComboBox->blockSignals(false);

    privateData->priceCurvePropertiesForm->showOpenCheckBox->blockSignals(true);
    privateData->priceCurvePropertiesForm->showOpenCheckBox->setChecked(privateData->showOpen);
    privateData->priceCurvePropertiesForm->showOpenCheckBox->blockSignals(false);

    privateData->priceCurvePropertiesForm->openSourceComboBox->blockSignals(true);
    privateData->priceCurvePropertiesForm->openSourceComboBox->setCurrentIndex(privateData->openSource);
    privateData->priceCurvePropertiesForm->openSourceComboBox->blockSignals(false);

    privateData->priceCurvePropertiesForm->showHighCheckBox->blockSignals(true);
    privateData->priceCurvePropertiesForm->showHighCheckBox->setChecked(privateData->showHigh);
    privateData->priceCurvePropertiesForm->showHighCheckBox->blockSignals(false);

    privateData->priceCurvePropertiesForm->highSourceComboBox->blockSignals(true);
    privateData->priceCurvePropertiesForm->highSourceComboBox->setCurrentIndex(privateData->highSource);
    privateData->priceCurvePropertiesForm->highSourceComboBox->blockSignals(false);

    privateData->priceCurvePropertiesForm->showLowCheckBox->blockSignals(true);
    privateData->priceCurvePropertiesForm->showLowCheckBox->setChecked(privateData->showLow);
    privateData->priceCurvePropertiesForm->showLowCheckBox->blockSignals(false);

    privateData->priceCurvePropertiesForm->lowSourceComboBox->blockSignals(true);
    privateData->priceCurvePropertiesForm->lowSourceComboBox->setCurrentIndex(privateData->lowSource);
    privateData->priceCurvePropertiesForm->lowSourceComboBox->blockSignals(false);

    privateData->priceCurvePropertiesForm->showCloseCheckBox->blockSignals(true);
    privateData->priceCurvePropertiesForm->showCloseCheckBox->setChecked(privateData->showClose);
    privateData->priceCurvePropertiesForm->showCloseCheckBox->blockSignals(false);

    privateData->priceCurvePropertiesForm->closeSourceComboBox->blockSignals(true);
    privateData->priceCurvePropertiesForm->closeSourceComboBox->setCurrentIndex(privateData->closeSource);
    privateData->priceCurvePropertiesForm->closeSourceComboBox->blockSignals(false);

    return privateData->priceCurvePropertiesWidget;
}

void PriceChart::setPriceProperties(void)
{
    privateData->showComposition = privateData->priceCurvePropertiesForm->showComposistionCheckBox->isChecked();
    privateData->compositionStyle = static_cast<QwtPlotPrice::CurveStyle>(privateData->priceCurvePropertiesForm->compositionStyleComboBox->currentIndex() + 1);
    privateData->compositionSource = static_cast<PriceChart::SourceType>(privateData->priceCurvePropertiesForm->compositionSourceComboBox->currentIndex());

    privateData->showOpen = privateData->priceCurvePropertiesForm->showOpenCheckBox->isChecked();
    privateData->openSource = static_cast<PriceChart::SourceType>(privateData->priceCurvePropertiesForm->openSourceComboBox->currentIndex());

    privateData->showHigh = privateData->priceCurvePropertiesForm->showHighCheckBox->isChecked();
    privateData->highSource = static_cast<PriceChart::SourceType>(privateData->priceCurvePropertiesForm->highSourceComboBox->currentIndex());

    privateData->showLow = privateData->priceCurvePropertiesForm->showLowCheckBox->isChecked();
    privateData->lowSource = static_cast<PriceChart::SourceType>(privateData->priceCurvePropertiesForm->lowSourceComboBox->currentIndex());

    privateData->showClose = privateData->priceCurvePropertiesForm->showCloseCheckBox->isChecked();
    privateData->closeSource = static_cast<PriceChart::SourceType>(privateData->priceCurvePropertiesForm->closeSourceComboBox->currentIndex());

    updatePriceCurveVisibility();

    writeSettings();
}

void PriceChart::updatePriceCurveVisibility()
{
    averagePriceComposition->priceCurve->detach();
    askPriceComposition->priceCurve->detach();
    bidPriceComposition->priceCurve->detach();
    if (privateData->showComposition)
    {
        if (privateData->compositionSource == Average)
            averagePriceComposition->priceCurve->attach(indexPlot);
        else if (privateData->compositionSource == Ask)
            askPriceComposition->priceCurve->attach(indexPlot);
        else if (privateData->compositionSource == Bid)
            bidPriceComposition->priceCurve->attach(indexPlot);
    }

    averagePriceComposition->priceCurve->setStyle(privateData->compositionStyle);
    askPriceComposition->priceCurve->setStyle(privateData->compositionStyle);
    bidPriceComposition->priceCurve->setStyle(privateData->compositionStyle);

    averagePriceOpen->detach();
    askPriceOpen->detach();
    bidPriceOpen->detach();
    if (privateData->showOpen)
    {
        if (privateData->openSource == Average)
            averagePriceOpen->attach(indexPlot);
        else if (privateData->openSource == Ask)
            askPriceOpen->attach(indexPlot);
        else if (privateData->openSource == Bid)
            bidPriceOpen->attach(indexPlot);

    }

    averagePriceHigh->detach();
    askPriceHigh->detach();
    bidPriceHigh->detach();
    if (privateData->showHigh)
    {
        if (privateData->highSource == Average)
            averagePriceHigh->attach(indexPlot);
        else if (privateData->highSource == Ask)
            askPriceHigh->attach(indexPlot);
        else if (privateData->highSource == Bid)
            bidPriceHigh->attach(indexPlot);
    }

    averagePriceLow->detach();
    askPriceLow->detach();
    bidPriceLow->detach();
    if (privateData->showLow)
    {
        if (privateData->lowSource == Average)
            averagePriceLow->attach(indexPlot);
        else if (privateData->lowSource == Ask)
            askPriceLow->attach(indexPlot);
        else if (privateData->lowSource == Bid)
            bidPriceLow->attach(indexPlot);
    }

    averagePriceClose->detach();
    askPriceClose->detach();
    bidPriceClose->detach();
    if (privateData->showClose)
    {
        if (privateData->closeSource == Average)
            averagePriceClose->attach(indexPlot);
        else if (privateData->closeSource == Ask)
            askPriceClose->attach(indexPlot);
        else if (privateData->closeSource == Bid)
            bidPriceClose->attach(indexPlot);
    }

    indexPlot->replot();
    indexPlot->magnifieYAxis();
}

void PriceChart::updateSimulationProperties(void)
{
    for (int i = 0; i < indicatorList.size(); i++)
    {
        if (indicatorList.at(i)->autoOptimizerMode())
            return;
    }

    bool chartUpdatesEnabled = testAttribute(Qt::WA_UpdatesDisabled);
    setAttribute(Qt::WA_UpdatesDisabled);

    privateData->simulationPropertiesData = simulationProperties->properties();
    loadData(QDateTime(), QDateTime(), true);

    setAttribute(Qt::WA_UpdatesDisabled, chartUpdatesEnabled);
}

int PriceChart::showSetupDialog(void)
{
    QDialog *dlg = new QDialog();
    dlg->setWindowModality(Qt::WindowModal);
    ssForm = new Ui::StrategieSetup();
    ssForm->setupUi(dlg);

    ssForm->scDoubleSpinBox->setValue(startCapital);
    ssForm->fcDoubleSpinBox->setValue(fixedTransactionCosts);
    ssForm->vcDoubleSpinBox->setValue(variableTransactionCosts * 100.0);
    ssForm->micDoubleSpinBox->setValue(minimumTransactionCosts);
    ssForm->macDoubleSpinBox->setValue(maximumTransactionCosts);
    ssForm->asDoubleSpinBox->setValue(averageSpread * 100);

    connect(ssForm->okPushButton, SIGNAL(clicked()), dlg, SLOT(accept()));
    connect(ssForm->apPushButton, SIGNAL(clicked()), this, SLOT(setProperties()));
    connect(ssForm->scDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setProperties()));
    connect(ssForm->fcDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setProperties()));
    connect(ssForm->vcDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setProperties()));
    connect(ssForm->micDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setProperties()));
    connect(ssForm->macDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setProperties()));
    connect(ssForm->asDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setProperties()));
    connect(ssForm->sltComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setProperties()));
    connect(ssForm->slvDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setProperties()));
    connect(ssForm->sltvDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setProperties()));

    if (dlg->exec() == QDialog::Rejected)
        return QDialog::Rejected;

    setProperties();

    return QDialog::Accepted;
}

void PriceChart::setProperties(void)
{
    startCapital = ssForm->scDoubleSpinBox->value();
    fixedTransactionCosts = ssForm->fcDoubleSpinBox->value();
    variableTransactionCosts = ssForm->vcDoubleSpinBox->value() / 100;
    minimumTransactionCosts = ssForm->micDoubleSpinBox->value();
    maximumTransactionCosts = ssForm->macDoubleSpinBox->value();
    averageSpread = ssForm->asDoubleSpinBox->value() / 100;

    emit strategie->currentPriceChart()->calculateProfits();

    indexPlot->replot();
}

IsinInfo PriceChart::isinInfo(void) const
{
    return privateData->isinInfo;
}

void PriceChart::setIsinInfo(const IsinInfo &info)
{
    if (!info.isValid())
    {
        QMessageBox messageBox;
        messageBox.setText(tr("Er is geen data beschikbaar voor de gekozen bron"));
        messageBox.exec();
        return;
    }
    else
    {
        QString name;
        if (isinInfo().providesBidAsk)
            name = QString(isinInfo().isin + "_ASK_" + dataConnection->timeScaleIntToString(timeScale()));
        else
            name = QString(isinInfo().isin + "_" + dataConnection->timeScaleIntToString(timeScale()));

        dataConnection->unsubscribeFromNotification(name, this);

        if (info.providesBidAsk)
            name = QString(info.isin + "_ASK_" + dataConnection->timeScaleIntToString(timeScale()));
        else
            name = QString(info.isin + "_" + dataConnection->timeScaleIntToString(timeScale()));
        dataConnection->subscribeToNotification(name, this);
    }

    IsinInfo oldIsinInfo = isinInfo();
    privateData->isinInfo = info;

    loadData();
    strategieOverviewWidget()->updateTitle();

    emit isinInfoChanged(oldIsinInfo, privateData->isinInfo);

    writeSettings();
}

int PriceChart::timeScale() const
{
    return privateData->timeScale;
}

void PriceChart::setTimeScale(int newTimeScale)
{
    if (!(privateData->isinInfo.availableTimeScales & newTimeScale))
    {
        QMessageBox messageBox;
        messageBox.setText(tr("Er is geen data beschikbaar voor de gekozen tijdschaal"));
        messageBox.exec();
        return;
    }
    else
    {
        QString name;
        if (isinInfo().providesBidAsk)
            name = QString(isinInfo().isin + "_ASK_" + dataConnection->timeScaleIntToString(timeScale()));
        else
            name = QString(isinInfo().isin + "_" + dataConnection->timeScaleIntToString(timeScale()));

        dataConnection->unsubscribeFromNotification(name, this);

        if (isinInfo().providesBidAsk)
            name = QString(isinInfo().isin + "_ASK_" + dataConnection->timeScaleIntToString(newTimeScale));
        else
            name = QString(isinInfo().isin + "_" + dataConnection->timeScaleIntToString(newTimeScale));
        dataConnection->subscribeToNotification(name, this);
    }

    int oldTimeScale = timeScale();
    privateData->timeScale = newTimeScale;

    loadData();
    strategieOverviewWidget()->updateTitle();

    emit timeScaleChanged(oldTimeScale, newTimeScale);

    writeSettings();
}

const QDateTime &PriceChart::firstAvailableDateTime() const
{
    return privateData->firstAvailableDateTime;
}

const QDateTime &PriceChart::lastAvailableDateTime() const
{
    return privateData->lastAvailableDateTime;
}

QDateTime PriceChart::startDateTime() const
{
    return ((indexPlot->timeScaleData().size() >= 1) ? indexPlot->timeScaleData().first() : QDateTime());
}

QDateTime PriceChart::endDateTime() const
{
    return ((indexPlot->timeScaleData().size() >= 1) ? indexPlot->timeScaleData().last() : QDateTime());
}

class Strategie::PrivateData
{
    public:
        PrivateData(void)
        {
            stopLossType = StopLossOptions::Normal;
            stopLossValue = 0.0;
            longStopLossValue = 0.0;
            shortStopLossValue = 0.0;
            profitTrailingThreshold = 0.0;
            longProfitTrailingThreshold = 0.0;
            shortProfitTrailingThreshold = 0.0;
            maximumStopLossFactor = 1.0;
            maximumThresholdStopLossFactor = 1.0;
            longMaximumStopLossFactor = 1.0;
            longMaximumThresholdStopLossFactor = 1.0;
            shortMaximumStopLossFactor = 1.0;
            shortMaximumThresholdStopLossFactor = 1.0;
            stopLossHits = 1;
            longStopLossHits = 1;
            shortStopLossHits = 1;
        }

    public:
        QWidget *dataSearchButtonsWidget;
        SlidingStackedWidget *chartStack;
        PlotImageBackground *plotImageBackground;
        int stopLossType;
        int stopLossHits;
        int longStopLossHits;
        int shortStopLossHits;
        double stopLossValue;
        double longStopLossValue;
        double shortStopLossValue;
        double profitTrailingThreshold;
        double longProfitTrailingThreshold;
        double shortProfitTrailingThreshold;
        double maximumStopLossFactor;
        double maximumThresholdStopLossFactor;
        double longMaximumStopLossFactor;
        double longMaximumThresholdStopLossFactor;
        double shortMaximumStopLossFactor;
        double shortMaximumThresholdStopLossFactor;
};

Strategie *strategie;
#include <qgl.h>
Strategie::Strategie(QWidget *parent) :
    QWidget(parent)
{
    connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(setApplicationAboutToQuit()));

    privateData = new PrivateData;
    ignoreDateEditChangedSignal = false;

    quotesDockWindow = new QMainWindow;
    quotesDockWindow->setDockOptions(QMainWindow::AnimatedDocks);
    QVBoxLayout *stretchLayout = new QVBoxLayout;
    stretchLayout->addWidget(quotesDockWindow);
    stretchLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
    stretchLayout->setStretch(1, 10);
    QWidget *stretchWidget = new QWidget;
    stretchWidget->setLayout(stretchLayout);
    mainWindow->ui->scrollArea->setWidget(stretchWidget);

    QPalette palette;
    palette.setColor(QPalette::Window, Qt::transparent);
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::AlternateBase, Qt::transparent);
    mainWindow->ui->scrollArea->setPalette(palette);

    QGridLayout *layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    QString iconPath = ":/images/input-mouse";
    QPushButton *mouseButton = new QPushButton(QIcon(iconPath), "", this);
    mouseButton->setToolTip(tr("Muis volger"));
    mouseButton->setCheckable(true);
    mouseButton->setChecked(true);
    layout->addWidget(mouseButton, 0, 1);
    connect(mouseButton, SIGNAL(toggled(bool)), this, SLOT(setMouseTracking(bool)));

    iconPath = ":/images/zoom-original";
    QPushButton *zresetButton = new QPushButton(QIcon(iconPath), ""/*tr( "Zoom <>" )*/, this);
    zresetButton->setToolTip(tr("Orginele zoom."));
    layout->addWidget(zresetButton, 0, 2);
    connect(zresetButton, SIGNAL(clicked(bool)), this, SLOT(zoomReset()));

    iconPath = ":/images/zoom-out";
    QPushButton *zminButton = new QPushButton(QIcon(iconPath), ""/*tr( "Zoom -" )*/, this);
    zminButton->setToolTip(tr("Uit zoomen."));
    layout->addWidget(zminButton, 0, 3);
    connect(zminButton, SIGNAL(clicked(bool)), this, SLOT(zoomOut()));

    iconPath = ":/images/zoom-in";
    QPushButton *zplusButton = new QPushButton(QIcon(iconPath), ""/*tr( "Zoom +" )*/, this);
    zplusButton->setToolTip(tr("In zoomen."));
    layout->addWidget(zplusButton, 0, 4);
    connect(zplusButton, SIGNAL(clicked(bool)), this, SLOT(zoomIn()));

    QPushButton *replotButton = new QPushButton("herplot", this);
    layout->addWidget(replotButton, 0, 5);
    connect(replotButton, SIGNAL(clicked()), this, SLOT(replotCurrentPriceChart()));

    privateData->dataSearchButtonsWidget = new QWidget(this);
    privateData->dataSearchButtonsWidget->setLayout(new QHBoxLayout(privateData->dataSearchButtonsWidget));
    privateData->dataSearchButtonsWidget->layout()->setMargin(0);
    layout->addWidget(privateData->dataSearchButtonsWidget, 0, 45);

    QAction *showAll = new QAction(tr("toon alles"),  privateData->dataSearchButtonsWidget);
    iconPath = ":/images/view-refresh";
    showAll->setIcon(QIcon(iconPath));
    connect(showAll, SIGNAL(triggered(bool)), this, SLOT(fullHistoryView()));

    QToolButton *showAllHistory = new QToolButton(privateData->dataSearchButtonsWidget);
    showAllHistory->setDefaultAction(showAll);
    showAllHistory->setToolTip(tr("Toon volledige geschiedenis."));
    privateData->dataSearchButtonsWidget->layout()->addWidget(showAllHistory/*, 0, 46*/);

    QMenu *rewindMenu = new QMenu(this);
    QAction *rewindAuto = new QAction(tr("terug"),  privateData->dataSearchButtonsWidget);
    iconPath = ":/images/go-first-view";
    rewindAuto->setIcon(QIcon(iconPath));
    connect(rewindAuto, SIGNAL(triggered(bool)), this, SLOT(rewindDate()));

    QAction *rewindDay = new QAction(tr("1 dag terug"),  privateData->dataSearchButtonsWidget);
    rewindMenu->addAction(rewindDay);
    connect(rewindDay, SIGNAL(triggered(bool)), this, SLOT(rewindDay()));

    QAction *rewindWeek = new QAction(tr("1 week terug"),  privateData->dataSearchButtonsWidget);
    rewindMenu->addAction(rewindWeek);
    connect(rewindWeek, SIGNAL(triggered(bool)), this, SLOT(rewindWeek()));

    QAction *rewindMonth = new QAction(tr("1 maand terug"),  privateData->dataSearchButtonsWidget);
    rewindMenu->addAction(rewindMonth);
    connect(rewindMonth, SIGNAL(triggered(bool)), this, SLOT(rewindMonth()));

    QAction *rewindYear = new QAction(tr("1 jaar terug"),  privateData->dataSearchButtonsWidget);
    rewindMenu->addAction(rewindYear);
    connect(rewindYear, SIGNAL(triggered(bool)), this, SLOT(rewindYear()));

    QToolButton *rewindDate = new QToolButton(privateData->dataSearchButtonsWidget);
    rewindDate->setDefaultAction(rewindAuto);
    rewindDate->setToolTip(tr("Terug in de tijd."));
    rewindDate->setMenu(rewindMenu);
    privateData->dataSearchButtonsWidget->layout()->addWidget(rewindDate/*, 0, 47*/);

    startDateEdit = new QDateEdit(privateData->dataSearchButtonsWidget);
    startDateEdit->setCalendarPopup(true);
    //    startDate->setCalendarWidget(new CalendarWidget());
    startDateEdit->calendarWidget()->setGridVisible(true);
    privateData->dataSearchButtonsWidget->layout()->addWidget(startDateEdit/*, 0, 48*/);
    connect(startDateEdit, SIGNAL(dateChanged(QDate)), this, SLOT(setStartDate(QDate)));

    endDateEdit = new QDateEdit(privateData->dataSearchButtonsWidget);
    endDateEdit->setCalendarPopup(true);
    //    endDate->setCalendarWidget(new CalendarWidget());
    endDateEdit->calendarWidget()->setGridVisible(true);
    privateData->dataSearchButtonsWidget->layout()->addWidget(endDateEdit/*, 0, 49*/);
    connect(endDateEdit, SIGNAL(dateChanged(QDate)), this, SLOT(setEndDate(QDate)));

    QMenu *forwardMenu = new QMenu(privateData->dataSearchButtonsWidget);
    QAction *forwardAuto = new QAction(tr("vooruit"), privateData->dataSearchButtonsWidget);
    iconPath = ":/images/go-last-view";
    forwardAuto->setIcon(QIcon(iconPath));
    connect(forwardAuto, SIGNAL(triggered(bool)), this, SLOT(forwardDate()));

    QAction *forwardDay = new QAction(tr("1 dag vooruit"), privateData->dataSearchButtonsWidget);
    forwardMenu->addAction(forwardDay);
    connect(forwardDay, SIGNAL(triggered(bool)), this, SLOT(forwardDay()));

    QAction *forwardWeek = new QAction(tr("1 week vooruit"), privateData->dataSearchButtonsWidget);
    forwardMenu->addAction(forwardWeek);
    connect(forwardWeek, SIGNAL(triggered(bool)), this, SLOT(forwardWeek()));

    QAction *forwardMonth = new QAction(tr("1 maand vooruit"), privateData->dataSearchButtonsWidget);
    forwardMenu->addAction(forwardMonth);
    connect(forwardMonth, SIGNAL(triggered(bool)), this, SLOT(forwardMonth()));

    QAction *forwardYear = new QAction(tr("1 jaar vooruit"), privateData->dataSearchButtonsWidget);
    forwardMenu->addAction(forwardYear);
    connect(forwardYear, SIGNAL(triggered(bool)), this, SLOT(forwardYear()));

    QToolButton *forwardDate = new QToolButton(privateData->dataSearchButtonsWidget);
    forwardDate->setDefaultAction(forwardAuto);
    forwardDate->setToolTip(tr("Vooruit in de tijd."));
    forwardDate->setMenu(forwardMenu);
    privateData->dataSearchButtonsWidget->layout()->addWidget(forwardDate/*, 0, 50*/);

    privateData->plotImageBackground = new PlotImageBackground;
    privateData->plotImageBackground->setImage(QImage(":/images/world-map-background"));

    privateData->chartStack = new SlidingStackedWidget(this);
    privateData->chartStack->setVerticalMode(true);
    privateData->chartStack->setNavigationButtonsEnabled(false);
    layout->addWidget(privateData->chartStack, 1, 0, 1, -1);
    connect(privateData->chartStack, SIGNAL(animationFinished()), SLOT(doSwitchToPriceChart()));
    connect(privateData->chartStack, SIGNAL(widgetRemoved(int)), SLOT(doSwitchToPriceChart()));

    connect(mainWindow->ui->actionBTTraderSettings, SIGNAL(triggered()), settingsDialog, SLOT(show()));
    connect(mainWindow->ui->actionBron_toevoegen, SIGNAL(triggered()), this, SLOT(addPriceChart()));
    connect(mainWindow->ui->actionMoving_Average, SIGNAL(triggered()), this, SLOT(createMAIndicator()));
    connect(mainWindow->ui->action2_Moving_Averages, SIGNAL(triggered()), this, SLOT(create2MAIndicator()));
    connect(mainWindow->ui->action3_Moving_Averages, SIGNAL(triggered()), this, SLOT(create3MAIndicator()));
    connect(mainWindow->ui->actionTrendLine, SIGNAL(triggered()), this, SLOT(createTrendLineIndicator()));
    connect(mainWindow->ui->actionSupportCeilingLine, SIGNAL(triggered()), this, SLOT(createSupportCeilingLineIndicator()));
    connect(mainWindow->ui->actionBollinger_Bands, SIGNAL(triggered()), this, SLOT(createBollingerBandsIndicator()));
    connect(mainWindow->ui->actionPrice_Channel, SIGNAL(triggered()), this, SLOT(createPriceChannelIndicator()));
    connect(mainWindow->ui->actionMesa_Instantaneous_Trend, SIGNAL(triggered()), this, SLOT(createInstantaneousTrendIndicator()));
    connect(mainWindow->ui->actionRSI, SIGNAL(triggered()), this, SLOT(createRSIIndicator()));
    connect(mainWindow->ui->actionStochastic, SIGNAL(triggered()), this, SLOT(createStochasticIndicator()));
    connect(mainWindow->ui->actionADX, SIGNAL(triggered()), this, SLOT(createADXIndicator()));
    connect(mainWindow->ui->actionDominant_Cycle, SIGNAL(triggered()), this, SLOT(createDominantCycleIndicator()));
    connect(mainWindow->ui->actionMesa_Trend_Vigor, SIGNAL(triggered()), this, SLOT(createTrendVigorIndicator()));
    connect(mainWindow->ui->actionMesa_SNR, SIGNAL(triggered()), this, SLOT(createSNRIndicator()));
    connect(mainWindow->ui->actionVolatility, SIGNAL(triggered()), this, SLOT(createVolatilityIndicator()));
    connect(mainWindow->ui->actionER, SIGNAL(triggered()), this, SLOT(createERIndicator()));
    connect(mainWindow->ui->action1_minute, SIGNAL(triggered()), this, SLOT(switchToMinuteTimeScale()));
    connect(mainWindow->ui->actionHourly, SIGNAL(triggered()), this, SLOT(switchToHourTimeScale()));
    connect(mainWindow->ui->actionDaily, SIGNAL(triggered()), this, SLOT(switchToDayTimeScale()));
    connect(mainWindow->ui->actionWeekly, SIGNAL(triggered()), this, SLOT(switchToWeekTimeScale()));
    connect(mainWindow->ui->actionMonthly, SIGNAL(triggered()), this, SLOT(switchToMonthTimeScale()));
    mainWindow->ui->actionOpslaan->setIcon(QIcon::fromTheme("document-save"));
    connect(mainWindow->ui->actionOpslaan, SIGNAL(triggered()), this, SLOT(saveBTTStrategie()));
    mainWindow->ui->actionImporteren->setIcon(QIcon::fromTheme("document-new"));
    connect(mainWindow->ui->actionImporteren, SIGNAL(triggered()), this, SLOT(importBTTStrategie()));
    mainWindow->ui->actionExporteren->setIcon(QIcon::fromTheme("document-save-as"));
    connect(mainWindow->ui->actionExporteren, SIGNAL(triggered()), this, SLOT(exportBTTStrategie()));

    fillStrategieMenu();
}

Strategie::~Strategie(void)
{
    delete privateData;
}

void Strategie::replotCurrentPriceChart(void)
{
    PriceChart *priceChart = currentPriceChart();
    if (!priceChart)
        return;

    qDebug() << "Strategie::replotCurrentPriceChart()";

    priceChart->indexPlot->replot();
}

void Strategie::restorePriceCharts(void)
{
    while (!settings.group().isEmpty())
        settings.endGroup();

    settings.beginGroup("Strategie");
    int activeChart = settings.value("ActiveChart", 0).toInt();
    settings.beginGroup("Charts");

    int timeScale;
    QStringList chartList = settings.childGroups();

    QList<PriceChart *> restoredPriceChartList;
    for (int i = 0; i < chartList.size(); ++i)
    {
        if (!chartList.at(i).startsWith("Chart"))
            continue;

        while (!settings.group().isEmpty())
            settings.endGroup();

        settings.beginGroup(QString("Strategie/charts/" + chartList.at(i)));

        timeScale = settings.value("TimeScale").toInt();
        IsinInfo info =  settings.value("IsinInfo").value<IsinInfo>();//dataConnection->getIsinInfo(isin);
        if (!info.isValid())
        {
            qWarning() << "Strategie::Strategie(): invalid IsinInfo";
            continue;
        }

        PriceChart *priceChart = addPriceChart(info, timeScale, chartList.at(i), false);
        restoredPriceChartList << priceChart;
        QApplication::processEvents();

        priceChart->setProcessMode(PriceChart::Data);

        if (i == 0)
        {
            switchToPriceChart(priceChart);
            QApplication::processEvents();
        }
    }

    for (int i = 0; i < restoredPriceChartList.size(); i++)
    {
        PriceChart *priceChart = restoredPriceChartList.at(i);
        priceChart->restoreIndicators(QStringList(""), priceChart->processMode());
        QApplication::processEvents();
    }

    if (activeChart < priceChartList.count())
    {
        switchToPriceChart(priceChartList.at(activeChart));
        QApplication::processEvents();
    }
}

RoundWidget::RoundWidget(QWidget *parent, Qt::WindowFlags f): QWidget(parent, f)
{
}

void RoundWidget::paintEvent(QPaintEvent *paintEvent)
{
    (void)paintEvent;

    QRect w_rect = rect();
    QStylePainter painter(this);
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath rounded_rect;
    rounded_rect.setFillRule(Qt::WindingFill);
    rounded_rect.addRoundedRect(0, 0, w_rect.width(), w_rect.height(), 5, 5);
    painter.setClipPath(rounded_rect);


    QRegion maskregion = painter.clipRegion();
    setMask(maskregion);
    painter.setOpacity(1.0);

    painter.fillPath(rounded_rect, palette().brush(QPalette::Window));
    painter.restore();
}

PopupWidget::PopupWidget(QWidget *parent, Qt::WindowFlags f): RoundWidget(parent, f)
{
    widget = new RoundWidget;
    //    widget->setBrush( Qt::transparent );
    widget->setWindowFlags(Qt::Popup);
    widget->setWindowOpacity(0.9);
    widget->setLayout(new QHBoxLayout);
    widget->layout()->setContentsMargins(0, 0, 0, 0);
    widget->resize(320, 240);
    widget->move(QCursor::pos());

    int screenNumber = QApplication::desktop()->screenNumber(QCursor::pos());
    QRect screenRect = QApplication::desktop()->screenGeometry(screenNumber);
    if (QCursor::pos().x() + widget->width() > screenRect.right())
        widget->move(QCursor::pos().x() - widget->width(), QCursor::pos().y());

    if (QCursor::pos().y() + widget->height() > screenRect.bottom())
        widget->move(x(), QCursor::pos().y() - widget->height());

    RoundWidget *shadow = new RoundWidget(widget);
    //    shadow->setBrush( Qt::black );
    shadow->setLayout(new QHBoxLayout);
    shadow->layout()->setContentsMargins(1, 1, 2, 2);
    QGraphicsBlurEffect *shadowBlurEffect = new QGraphicsBlurEffect(shadow);
    shadowBlurEffect->setBlurRadius(10);
    shadowBlurEffect->setBlurHints(QGraphicsBlurEffect::QualityHint);
    shadow->setGraphicsEffect(shadowBlurEffect);
    widget->layout()->addWidget(shadow);

    RoundWidget *glassRing = new RoundWidget(shadow);
    //    glassRing->setBrush( Qt::white );
    glassRing->setLayout(new QHBoxLayout);
    glassRing->layout()->setContentsMargins(15, 15, 15, 15);
    shadow->layout()->addWidget(glassRing);

    QColor colorColor = QApplication::palette().highlight().color();
    colorColor = colorColor.darker();
    QLinearGradient colorGradient(0, 0, 320, 240);
    colorGradient.setColorAt(0.0, colorColor.lighter().lighter());
    colorGradient.setColorAt(0.3, colorColor);
    colorGradient.setColorAt(0.5, colorColor.darker());
    QBrush colorBrush = QBrush(colorGradient);

    colorRing = new RoundWidget(glassRing);
    //    colorRing->setBrush( colorBrush );
    colorRing->setLayout(new QHBoxLayout);
    colorRing->layout()->setContentsMargins(4, 4, 4, 4);

    glassRing->layout()->addWidget(colorRing);


    colorRing->layout()->addWidget(this);
    installEventFilter(this);

    QTimer::singleShot(5000, this, SLOT(close()));
}

PopupWidget::~PopupWidget(void)
{
    delete widget;
}

bool PopupWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Close)
        widget->close();

    return QObject::eventFilter(obj, event);
}

void PopupWidget::show(void)
{
    widget->show();
    setParent(NULL);
    setWindowFlags(Qt::Popup);
    setWindowOpacity(0.4);
    QPoint point = widget->pos();
    point.setX(point.x() + 18);
    point.setY(point.y() + 18);
    move(point);
    QWidget::show();
}

void PopupWidget::resize(const QSize &size)
{
    QColor colorColor = QApplication::palette().highlight().color();
    colorColor = colorColor.darker();
    QLinearGradient colorGradient(0, 0, size.width() + 100 , size.height());
    colorGradient.setColorAt(0.0, colorColor.lighter().lighter());
    colorGradient.setColorAt(0.3, colorColor);
    colorGradient.setColorAt(0.5, colorColor.darker());
    QBrush colorBrush = QBrush(colorGradient);
    //    colorRing->setBrush( colorBrush );

    widget->resize(size);
}

PriceChart *Strategie::currentPriceChart(void)
{
    if (!chartStack())
        return NULL;

    PriceChart *priceChart = qobject_cast<PriceChart *>(chartStack()->currentWidget());
    if (priceChartList.contains(priceChart))
        return priceChart;

    return NULL;
}

PriceChart *Strategie::addPriceChart(void)
{
    QDialog *sourcePickerDialog = new QDialog(this);
    Ui::SourcePickerUI *sourcePickerForm = new Ui::SourcePickerUI;
    sourcePickerForm->setupUi(sourcePickerDialog);
    sourcePickerForm->treeWidget->setCurrentItem(0);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, sourcePickerDialog);
    connect(buttonBox, SIGNAL(accepted()), sourcePickerDialog, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), sourcePickerDialog, SLOT(reject()));
    sourcePickerDialog->layout()->addWidget(buttonBox);

    int retval = sourcePickerDialog->exec();
    if (retval == QDialog::Rejected)
        return NULL;

    QList<QTreeWidgetItem *> selectedItems = sourcePickerForm->treeWidget->selectedItems();
    if (selectedItems.isEmpty() || selectedItems.size() != 1)
        return NULL;

    int timeScale = 1 << (sourcePickerForm->comboBox->currentIndex() + 1);

    QString isin = selectedItems[ 0 ]->text(0);

    isin = "EURUSD";
    IsinInfo info = dataConnection->getIsinInfo(isin);
    return addPriceChart(info, timeScale);
}

PriceChart *Strategie::addPriceChart(const IsinInfo &info, int timeScale, const QString &configKey, bool switchToNewChart)
{
    PriceChart *priceChart = new PriceChart(info, timeScale, this, configKey);
    chartStack()->addWidget(priceChart);

    connect(priceChart, SIGNAL(removeRequested(PriceChart *)), this, SLOT(removePriceChart(PriceChart *)));
    connect(priceChart, SIGNAL(startDateTimeChanged(QDateTime)), this, SLOT(updateStartDate(QDateTime)));
    connect(priceChart, SIGNAL(endDateTimeChanged(QDateTime)), this, SLOT(updateEndDate(QDateTime)));
    connect(priceChart, SIGNAL(firstAvailableDateTimeChanged(QDateTime)), this, SLOT(updateFirstAvailableCalendarDate(QDateTime)));
    connect(priceChart, SIGNAL(lastAvailableDateTimeChanged(QDateTime)), this, SLOT(updateLastAvailableCalendarDate(QDateTime)));

    connect(priceChart->indexPlot->tracker(), SIGNAL(movedFromTo(QPoint, QPoint)), this, SLOT(updateDockBackground(QPoint, QPoint)));
    connect(priceChart->indexPlot->zoomer(), SIGNAL(zoomed(QwtDoubleRect)), this, SLOT(updateDockBackground()));
    connect(priceChart, SIGNAL(dataSet()), this, SLOT(updateDockBackground()));
    connect(priceChart, SIGNAL(dataCleared()), this, SLOT(updateDockBackground()));
    connect(priceChart, SIGNAL(dataAppended()), this, SLOT(updateDockBackground()));

    connect(priceChart->indicatorStackWidget(), SIGNAL(currentChanged(int)), this, SLOT(updateDockStatus()));
    connect(priceChart, SIGNAL(destroyed()), this, SLOT(updateDockStatus()));

    priceChartList << priceChart;
    dataSearchEnabledList << true;

    if (switchToNewChart)
        switchToPriceChart(priceChart);

    return priceChart;
}

void Strategie::replacePriceChart(const IsinInfo &info)
{
    if (!currentPriceChart())
        addPriceChart(info);
    else
        currentPriceChart()->setIsinInfo(info);
}

void Strategie::removePriceChart(PriceChart *priceChart)
{
    if (!priceChart)
        return;

    int index = priceChartList.indexOf(priceChart);
    priceChartList.removeAt(index);
    dataSearchEnabledList.removeAt(index);
    chartStack()->removeWidget(priceChart);

    priceChart->deleteLater();
}

void Strategie::switchToPriceChart(PriceChart *priceChart, bool animated)
{
    if (!priceChart)
        return;

    if (priceChart->processMode() == PriceChart::Data)
        priceChart->setProcessMode(PriceChart::View);

    PriceChart *oldPriceChart = currentPriceChart();
    if (oldPriceChart && oldPriceChart != priceChart && oldPriceChart->processMode() == PriceChart::View)
        oldPriceChart->setProcessMode(PriceChart::Data);


    priceChart->strategieOverviewWidget()->resetAlarm();

    privateData->plotImageBackground->detach();
    privateData->plotImageBackground->attach(priceChart->indexPlot);
    priceChart->indexPlot->replot();

    QApplication::processEvents(); //animatie snelheid is afhankelijk van de event queue, dus zorg ervoor dat deze leeg is om een soepele animatie te verkrijgen

    if (animated)
    {
        int tabIndex = chartStack()->indexOf(priceChart);
        chartStack()->slideInIdx(tabIndex);   // It's not necessary to call doSwitchChart, it will be called when the animation is finished
    }
    else
    {
        chartStack()->setCurrentWidget(priceChart);
        doSwitchToPriceChart();
    }
}

void Strategie::doSwitchToPriceChart(void)
{
    PriceChart *priceChart = currentPriceChart();
    if (!priceChart)
    {
        updateDockSpacing();
        updateDockStatus();
        return;
    }

    bool updates = mainWindow->updatesEnabled();
    mainWindow->setUpdatesEnabled(false);

    if (priceChart->processMode() == PriceChart::Data)
        priceChart->setProcessMode(PriceChart::View);

    if (privateData->plotImageBackground->plot() != priceChart->indexPlot)
    {
        privateData->plotImageBackground->attach(priceChart->indexPlot);
        priceChart->indexPlot->replot();
    }

    priceChart->strategieOverviewWidget()->resetAlarm();

    mainWindow->ui->indicatorDockWidget->setWidget(priceChart->indicatorStackWidget());
    mainWindow->ui->transactionsDockWidget->setWidget(priceChart->transactionTabWidget());

    updateDockSpacing();
    updateDockStatus();

    mainWindow->ui->quotesDockWidget->floatWidget()->setUpdatesEnabled(true);
    mainWindow->ui->dataBrowserDockWidget->floatWidget()->setUpdatesEnabled(true);
    mainWindow->ui->indicatorDockWidget->floatWidget()->setUpdatesEnabled(true);
    mainWindow->ui->transactionsDockWidget->floatWidget()->setUpdatesEnabled(true);

    int index = priceChartList.indexOf(priceChart);
    if (index >= 0)
    {
        bool enabled = dataSearchEnabledList.at(index);
        privateData->dataSearchButtonsWidget->setEnabled(enabled);
    }

    updateStartDate(priceChart->startDateTime());
    updateEndDate(priceChart->endDateTime());
    updateFirstAvailableCalendarDate(priceChart->firstAvailableDateTime());
    updateLastAvailableCalendarDate(priceChart->lastAvailableDateTime());

    updateTimeScaleMenu();

    mainWindow->setUpdatesEnabled(updates);

    int activeChart = -1;
    for (int i = 0; i < priceChartList.size(); ++i)
    {
        if (priceChartList.at(i) == priceChart)
            activeChart = i;
    }

    if (activeChart == -1)
        return;

    while (!settings.group().isEmpty())
        settings.endGroup();

    settings.beginGroup("Strategie");
    settings.setValue("activeChart", activeChart);
    settings.endGroup();
    settings.sync();
}

static QTimer updateDockTimer;

struct UpdateDockRegion
{
    QWidget *floatWidget;
    QList<QRegion> regionList;
    QTime timestamp;
};

QList<UpdateDockRegion *> regionNeedsUpdatingList;

void Strategie::updateDockBackground(const QPoint &from , const QPoint &to)
{
    if (!sender())
        return;

    Tracker *tracker = qobject_cast<Tracker *>(sender());
    if (!tracker)
        return;

    if (!tracker->enabled())
        return;

    QPoint previousGlobalPos = tracker->canvas()->mapToGlobal(from);
    previousGlobalPos.setX((previousGlobalPos.x() / 2) * 2);
    previousGlobalPos.setY((previousGlobalPos.y() / 2) * 2);

    QPoint currentGlobalPos = tracker->canvas()->mapToGlobal(to);
    currentGlobalPos.setX((currentGlobalPos.x() / 2) * 2);
    currentGlobalPos.setY((currentGlobalPos.y() / 2) * 2);

    QObjectList::const_iterator it = mainWindow->children().constBegin();
    QObjectList::const_iterator itEnd = mainWindow->children().constEnd();
    for (; it != itEnd; it++)
    {
        QObject *child = *it;
        DockWidget *dockWidget = qobject_cast<DockWidget *>(child);
        if (!dockWidget)
            continue;

        TransparentDock *floatWidget = dockWidget->floatWidget();
        if (!floatWidget || !floatWidget->isVisible())
            continue;

        QPen pen = tracker->rubberBandPen();
        QPoint previousPos = floatWidget->mapFromGlobal(previousGlobalPos);
        QPoint currentPos = floatWidget->mapFromGlobal(currentGlobalPos);
        QPoint translatedCanvasTopLeft = floatWidget->mapFromGlobal(tracker->canvas()->mapToGlobal(tracker->canvas()->rect().topLeft()));
        int y = translatedCanvasTopLeft.y();
        int x = translatedCanvasTopLeft.x();
        int offset = 48;
        int trackerWidth = tracker->pickRect().width();
        int trackerHeight = tracker->pickRect().height();

        QRegion updateRegion(previousPos.x() - ((pen.width() + offset) / 2) , y, pen.width() + offset, trackerHeight);
        updateRegion = updateRegion.united(QRegion(x , previousPos.y() - ((pen.width() + offset) / 2), trackerWidth, pen.width() + offset));
        updateRegion =  updateRegion.united(QRegion(currentPos.x() - ((pen.width() + offset) / 2) , y, pen.width() + offset, trackerHeight));
        updateRegion = updateRegion.united(QRegion(x , currentPos.y() - ((pen.width() + offset) / 2), trackerWidth, pen.width() + offset));

        //        floatWidget->update(updateRegion);

        UpdateDockRegion *region = NULL;
        for (int i = 0; i < regionNeedsUpdatingList.size(); ++i)
        {
            region = regionNeedsUpdatingList.at(i);
            if (region->floatWidget == floatWidget)
                break;
            else
                region = NULL;
        }

        if (!region)
        {
            region = new UpdateDockRegion;
            region->floatWidget = floatWidget;
            region->timestamp = QTime::currentTime();
            regionNeedsUpdatingList << region;
        }

        region->regionList.append(updateRegion);

        if (!updateDockTimer.isActive())
        {
            updateDockTimer.setInterval(33);
            updateDockTimer.setSingleShot(true);
            connect(&updateDockTimer, SIGNAL(timeout()), this, SLOT(doUpdateDockBackground()), Qt::UniqueConnection);
            updateDockTimer.start();
        }
    }
}

void Strategie::updateDockBackground(void)
{
    PriceChart *priceChart = currentPriceChart();
    if (priceChart && priceChart->testAttribute(Qt::WA_UpdatesDisabled))
        return;

    if (sender() && sender()->isWidgetType() && !static_cast<QWidget *>(sender())->isVisible())
        return;

    QObjectList::const_iterator it = mainWindow->children().constBegin();
    QObjectList::const_iterator itEnd = mainWindow->children().constEnd();
    for (; it != itEnd; it++)
    {
        QObject *child = *it;
        DockWidget *dockWidget = qobject_cast<DockWidget *>(child);
        if (!dockWidget)
            continue;

        TransparentDock *floatWidget = dockWidget->floatWidget();
        if (!floatWidget || !floatWidget->isVisible())
            continue;

        //        floatWidget->update();

        UpdateDockRegion *region = NULL;
        for (int i = 0; i < regionNeedsUpdatingList.size(); ++i)
        {
            region = regionNeedsUpdatingList.at(i);
            if (region->floatWidget == floatWidget)
                break;
            else
                region = NULL;
        }

        if (!region)
        {
            region = new UpdateDockRegion;
            region->floatWidget = floatWidget;
            region->timestamp = QTime::currentTime();
            regionNeedsUpdatingList << region;
        }

        region->regionList.append(QRegion(floatWidget->rect()));

        if (!updateDockTimer.isActive())
        {
            updateDockTimer.setInterval(33);
            updateDockTimer.setSingleShot(true);
            connect(&updateDockTimer, SIGNAL(timeout()), this, SLOT(doUpdateDockBackground()), Qt::UniqueConnection);
            updateDockTimer.start();
        }
    }
}

void Strategie::doUpdateDockBackground(void)
{
    QTime currentTime = QTime::currentTime();
    QTime nextUpdateTime = currentTime;

    for (int i = 0; i < regionNeedsUpdatingList.size(); i++)
    {
        UpdateDockRegion *region = regionNeedsUpdatingList.at(i);
        if (region->timestamp.msecsTo(currentTime) > 33)
        {
            for (int j = 0; j < region->regionList.size(); j++)
                region->floatWidget->update(region->regionList.at(j));

            region->regionList.clear();
            region->timestamp = currentTime;
        }
        else if (region->timestamp < nextUpdateTime)
            nextUpdateTime = region->timestamp;
    }

    if (nextUpdateTime != currentTime)
    {
        updateDockTimer.setInterval((nextUpdateTime.msecsTo(currentTime) * -1) + 33);
        updateDockTimer.setSingleShot(true);
        updateDockTimer.start();
    }
}

void Strategie::updateDockSpacing(void)
{
    PriceChart *priceChart = currentPriceChart();
    if (!priceChart)
    {
        mainWindow->ui->quotesDockWidget->floatWidget()->setWindowToDockSpacing(0, 0, 0, 0);
        mainWindow->ui->dataBrowserDockWidget->floatWidget()->setWindowToDockSpacing(0, 0, 0, 0);
        mainWindow->ui->indicatorDockWidget->floatWidget()->setWindowToDockSpacing(0, 0, 0, 0);
        mainWindow->ui->transactionsDockWidget->floatWidget()->setWindowToDockSpacing(0, 0, 0, 0);

        return;
    }

    int leftSpacing = (mapTo(window(), priceChart->indexPlot->canvas()->pos()) - mapTo(window(), priceChart->indexPlot->pos())).x();
    int bottomSpacing = 0;
    if (/*priceChart->plotList().count() == 1 && */priceChart->indexPlot->axisWidget(QwtPlot::xBottom))
        bottomSpacing = priceChart->indexPlot->axisWidget(QwtPlot::xBottom)->height() - 3;

    mainWindow->ui->quotesDockWidget->floatWidget()->setWindowToDockSpacing(0, 0, bottomSpacing, leftSpacing);
    mainWindow->ui->dataBrowserDockWidget->floatWidget()->setWindowToDockSpacing(0, 0, bottomSpacing, leftSpacing);
    mainWindow->ui->indicatorDockWidget->floatWidget()->setWindowToDockSpacing(0, 0, bottomSpacing, 0);
    mainWindow->ui->transactionsDockWidget->floatWidget()->setWindowToDockSpacing(0, 0, bottomSpacing, 0);
}

void Strategie::updateDockStatus(void)
{
    PriceChart *priceChart = currentPriceChart();
    mainWindow->ui->indicatorDockWidget->blockSignals(true);
    if (!priceChart || priceChart->indicatorStackWidget()->count() == 0)
    {
        if (!priceChart)
            mainWindow->ui->indicatorDockWidget->floatWidget()->setAlignWidget(mainWindow->centralWidget());
        else
            mainWindow->ui->indicatorDockWidget->floatWidget()->setAlignWidget(priceChart);

        mainWindow->ui->indicatorDockWidget->setVisible(false);
        mainWindow->ui->indicatorDockWidget->floatWidget()->setEnabled(false);
        mainWindow->ui->showIndicatorsDockButton->setEnabled(false);
    }
    else
    {
        mainWindow->ui->indicatorDockWidget->floatWidget()->setAlignWidget(priceChart);
        if (mainWindow->ui->showIndicatorsDockButton->isChecked())
            mainWindow->ui->indicatorDockWidget->setVisible(true);
        else
            mainWindow->ui->indicatorDockWidget->setVisible(false);
        mainWindow->ui->indicatorDockWidget->floatWidget()->setEnabled(true);
        mainWindow->ui->showIndicatorsDockButton->setEnabled(true);
    }
    mainWindow->ui->indicatorDockWidget->blockSignals(false);

    mainWindow->ui->transactionsDockWidget->blockSignals(true);
    if (!priceChart || priceChart->indicatorStackWidget()->count() == 0)
    {
        if (!priceChart)
            mainWindow->ui->transactionsDockWidget->floatWidget()->setAlignWidget(mainWindow->centralWidget());
        else
            mainWindow->ui->transactionsDockWidget->floatWidget()->setAlignWidget(priceChart);

        mainWindow->ui->transactionsDockWidget->setVisible(false);
        mainWindow->ui->transactionsDockWidget->floatWidget()->setEnabled(false);
        mainWindow->ui->showTransactionsDockButton->setEnabled(false);
    }
    else
    {
        mainWindow->ui->transactionsDockWidget->floatWidget()->setAlignWidget(priceChart);
        if (mainWindow->ui->showTransactionsDockButton->isChecked())
            mainWindow->ui->transactionsDockWidget->setVisible(true);
        else
            mainWindow->ui->transactionsDockWidget->setVisible(false);
        mainWindow->ui->transactionsDockWidget->floatWidget()->setEnabled(true);
        mainWindow->ui->showTransactionsDockButton->setEnabled(true);
    }
    mainWindow->ui->transactionsDockWidget->blockSignals(false);

    mainWindow->ui->quotesDockWidget->blockSignals(true);
    if (!priceChart)
    {
        mainWindow->ui->quotesDockWidget->floatWidget()->setAlignWidget(mainWindow->centralWidget());
        mainWindow->ui->quotesDockWidget->setVisible(false);
        mainWindow->ui->quotesDockWidget->floatWidget()->setEnabled(false);
        mainWindow->ui->showQuotesDockButton->setEnabled(false);
    }
    else
    {
        mainWindow->ui->quotesDockWidget->floatWidget()->setAlignWidget(priceChart);
        if (mainWindow->ui->showQuotesDockButton->isChecked())
            mainWindow->ui->quotesDockWidget->setVisible(true);
        else
            mainWindow->ui->quotesDockWidget->setVisible(false);
        mainWindow->ui->quotesDockWidget->floatWidget()->setEnabled(true);
        mainWindow->ui->showQuotesDockButton->setEnabled(true);
    }
    mainWindow->ui->quotesDockWidget->blockSignals(false);

    if (!priceChart)
        mainWindow->ui->dataBrowserDockWidget->floatWidget()->setAlignWidget(mainWindow->centralWidget());
    else
        mainWindow->ui->dataBrowserDockWidget->floatWidget()->setAlignWidget(priceChart);
}

void Strategie::testSlot(void)
{
}

void Strategie::setApplicationAboutToQuit(void)
{
    aboutToQuit = true;
}

void Strategie::createMAIndicator(void)
{
    if (!currentPriceChart())
        return;

    MovingAverageCurve *maCurve = new MovingAverageCurve(currentPriceChart()->indexPlot, currentPriceChart());
    currentPriceChart()->finishCreateIndicator(maCurve);

}

void Strategie::create2MAIndicator(void)
{
    if (!currentPriceChart())
        return;

    TwoMovingAverageCurve *maCurve = new TwoMovingAverageCurve(currentPriceChart()->indexPlot, currentPriceChart());
    currentPriceChart()->finishCreateIndicator(maCurve);
}

void Strategie::create3MAIndicator(void)
{
    if (!currentPriceChart())
        return;

    ThreeMovingAverageCurve *maCurve = new ThreeMovingAverageCurve(currentPriceChart()->indexPlot, currentPriceChart());
    currentPriceChart()->finishCreateIndicator(maCurve);
}

void Strategie::createBollingerBandsIndicator(void)
{
    if (!currentPriceChart())
        return;

    BollingerBands *bollingerBands = new BollingerBands(currentPriceChart()->indexPlot, currentPriceChart());
    currentPriceChart()->finishCreateIndicator(bollingerBands);
}

void Strategie::createPriceChannelIndicator(void)
{
    if (!currentPriceChart())
        return;

    PriceChannel *priceChannel = new PriceChannel(currentPriceChart()->indexPlot, currentPriceChart());
    currentPriceChart()->finishCreateIndicator(priceChannel);
}

void Strategie::createInstantaneousTrendIndicator(void)
{
    if (!currentPriceChart())
        return;

    InstantaneousTrend *iTrend = new InstantaneousTrend(currentPriceChart()->indexPlot, currentPriceChart());
    currentPriceChart()->finishCreateIndicator(iTrend);
}

void Strategie::createTrendLineIndicator(void)
{
    if (!currentPriceChart())
        return;

    Line *line = new Line(currentPriceChart()->indexPlot, currentPriceChart(), Line::Free);
    currentPriceChart()->finishCreateIndicator(line);
}

void Strategie::createSupportCeilingLineIndicator(void)
{
    if (!currentPriceChart())
        return;

    Line *line = new Line(currentPriceChart()->indexPlot, currentPriceChart(), Line::Horizontal);
    currentPriceChart()->finishCreateIndicator(line);
}

void Strategie::createRSIIndicator(void)
{
    if (!currentPriceChart())
        return;

    RSI *rsi = new RSI(NULL, currentPriceChart());
    currentPriceChart()->finishCreateIndicator(rsi);
}

void Strategie::createStochasticIndicator(void)
{
    if (!currentPriceChart())
        return;

    Stochastic *stochastic = new Stochastic(NULL, currentPriceChart());
    currentPriceChart()->finishCreateIndicator(stochastic);
}

void Strategie::createADXIndicator(void)
{
    if (!currentPriceChart())
        return;

    ADX *adx = new ADX(NULL, currentPriceChart());
    currentPriceChart()->finishCreateIndicator(adx);
}

void Strategie::createDominantCycleIndicator(void)
{
    if (!currentPriceChart())
        return;

    DominantCycle *dominantCycle = new DominantCycle(NULL, currentPriceChart());
    currentPriceChart()->finishCreateIndicator(dominantCycle);
}

void Strategie::createTrendVigorIndicator(void)
{
    if (!currentPriceChart())
        return;

    TrendVigor *trendVigor = new TrendVigor(NULL, currentPriceChart());
    currentPriceChart()->finishCreateIndicator(trendVigor);
}

void Strategie::createSNRIndicator(void)
{
    if (!currentPriceChart())
        return;

    SNR *snr = new SNR(NULL, currentPriceChart());
    currentPriceChart()->finishCreateIndicator(snr);
}

void Strategie::createVolatilityIndicator(void)
{
    if (!currentPriceChart())
        return;

    Volatility *volatility = new Volatility(NULL, currentPriceChart());
    currentPriceChart()->finishCreateIndicator(volatility);
}

void Strategie::createERIndicator(void)
{
    if (!currentPriceChart())
        return;

    ER *er = new ER(NULL, currentPriceChart());
    currentPriceChart()->finishCreateIndicator(er);
}

int Strategie::getFirstAvailableIndicatorNumber(PriceChart *strategiePage)
{
    int count = 0;
    QString oldGroup = settings.group();

    while (!settings.group().isEmpty())
        settings.endGroup();

    settings.beginGroup("Strategie");
    settings.beginGroup("Charts");
    settings.beginGroup(strategiePage->configGroupKey());
    settings.beginGroup("Indicators");

    QString configGroup = "Indicator";
    QStringList indicatorConfigList = settings.childGroups();
    for (int i = 0; i < INT_MAX; ++i)
    {
        if (i == 0)
            configGroup.append(QString::number(i));
        else
            configGroup.replace(QString::number(i - 1), QString::number(i));

        if (!indicatorConfigList.contains(configGroup))
        {
            bool found = false;
            for (int j = 0; j < strategiePage->indicatorList.size(); ++j)
            {
                if (strategiePage->indicatorList[ j ]->configGroupKey() == configGroup)
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                count = i;
                break;
            }
        }
    }

    while (!settings.group().isEmpty())
        settings.endGroup();

    settings.beginGroup(oldGroup);

    return count;
}

void Strategie::setDataSearchWidgetEnabledForPriceChart(bool enabled, PriceChart *priceChart)
{
    for (int i = 0; i < priceChartList.size(); i++)
    {
        if (priceChartList.at(i) == priceChart)
        {
            dataSearchEnabledList[i] = enabled;
            if (currentPriceChart() == priceChart)
                privateData->dataSearchButtonsWidget->setEnabled(enabled);
        }
    }
}

SlidingStackedWidget *Strategie::chartStack(void) const
{
    return privateData->chartStack;
}

void Strategie::loadBTTStrategie(void)
{
    qDebug() << "Strategie::loadBTTStrategie()";

    if (!currentPriceChart())
    {
        QMessageBox::warning(this, tr("Waarschuwing"), tr("Selecteer eerst een handelsbron uit het bron menu."));
        return;
    }

    QObject *senderObject = sender();
    if (!senderObject)
        return;

    QAction *action = qobject_cast<QAction *>(senderObject);
    if (!action)
        return;

    QString title = action->text();

    QSettings strategieIndicators(QApplication::organizationName(), title);
    if (strategieIndicators.value("ContentType") != QString("Strategie"))
    {
        QMessageBox::warning(this, tr("Waarschuwing"), tr("Dit is geen geldig strategie bestand."));
        return;
    }

    QStringList keys = strategieIndicators.allKeys();
    if (keys.isEmpty())
    {
        QMessageBox::warning(this, tr("Waarschuwing"), tr("Dit is geen leeg strategie bestand."));
        return;
    }

    QString PriceChartConfigKey = currentPriceChart()->configGroupKey();
    QString indicatorNumber = "-1";
    QString newIndicatorNumber = "";
    QString previousIndicatorNumber = "-2";
    QStringList loadList;

    while (!settings.group().isEmpty())
        settings.endGroup();

    for (int i = 0; i < keys.size(); ++i)
    {
        QString cpy = keys.at(i);
        int index = cpy.indexOf(QRegExp("[\\/]"));
        if (index < 0)
            continue;

        cpy.replace(index, 1, QChar('\0'));
        if (strncmp("Indicator", cpy.toAscii().data(), 9) != 0)
            continue;

        indicatorNumber = cpy;

        QVariant value = strategieIndicators.value(keys.at(i));
        QString key = "Strategie/Charts/" + PriceChartConfigKey + "/Indicators/" + keys.at(i);

        if (strcmp(indicatorNumber.toAscii().data(), previousIndicatorNumber.toAscii().data()) != 0)
        {
            newIndicatorNumber = "/Indicator" + QString::number(getFirstAvailableIndicatorNumber(currentPriceChart())) + "/";
            previousIndicatorNumber = indicatorNumber;
            loadList << newIndicatorNumber;
        }

        QRegExp exp("[\\/]Indicator[0-9]{1,4}[\\/]");
        key.replace(exp, newIndicatorNumber);
        settings.setValue(key, value);
    }

    QStringList indicatorNumberList;
    for (int i = 0; i < loadList.size(); ++i)
    {
        indicatorNumber =  loadList.at(i);
        indicatorNumber.replace("/", "");
        indicatorNumberList << indicatorNumber;
        //        currentTab()->restoreIndicators( indicatorNumber );
    }

    if (!indicatorNumberList.isEmpty())
        currentPriceChart()->restoreIndicators(indicatorNumberList);
}

void Strategie::saveBTTStrategie(void)
{
    qDebug() << "Strategie::saveBTTStrategie()";

    if (!currentPriceChart())
    {
        QMessageBox::warning(this, tr("Waarschuwing"), tr("Selecteer eerst een handelsbron uit het bron menu."));
        return;
    }

retry:
    bool ok;
    QString title = QInputDialog::getText(this, tr("Geef naam"), tr("Geef naam:"), QLineEdit::Normal, QString(), &ok);
    if (!ok)
        return;

    QList<QAction *> actionList = mainWindow->ui->strategyMenu->actions();// strategieMenu->actions();
    if (!actionList.isEmpty())
    {
        for (int i = 0; i < actionList.size(); ++i)
        {
            if (actionList.at(i)->text() == title)
            {
                QMessageBox::warning(this, tr("Waarschuwing"), tr("Er bestaat al een strategie met deze titel.\n"));
                goto retry;
            }
        }
    }

    while (!settings.group().isEmpty())
        settings.endGroup();

    QStringList strategieMenuList =  settings.value("Strategie/StrategieMenuEntries").toStringList();
    strategieMenuList << title;
    settings.setValue("strategie/StrategieMenuEntries", strategieMenuList);
    settings.sync();

    QSettings strategieIndicators(QApplication::organizationName(), title);
    strategieIndicators.clear();
    strategieIndicators.sync();
    strategieIndicators.setValue("ContentType", "Strategie");
    int indicatorCount = -1;

    for (int i = 0; i < currentPriceChart()->indicatorList.size(); ++i)
    {
        if (currentPriceChart()->indicatorList.at(i) == currentPriceChart()->trendIndicator() ||
            currentPriceChart()->indicatorList.at(i) == currentPriceChart()->dominantCycleIndicator() ||
            currentPriceChart()->indicatorList.at(i) == currentPriceChart()->trendVigorIndicator() ||
            !currentPriceChart()->indicatorList.at(i)->autoRestore() ||
            currentPriceChart()->indicatorList.at(i)->processMode() == Indicator::Hidden)
            continue;

        QStringList keys = settings.allKeys();
        bool hasConfig = false;
        for (int j = 0; j < keys.size(); ++j)
        {
            if (!keys.at(j).contains(currentPriceChart()->indicatorList.at(i)->configGroupKey()))
                continue;

            if (!hasConfig)
            {
                ++indicatorCount;
                hasConfig = true;
            }

            QVariant value = settings.value(keys.at(j));
            QString key = keys.at(j);
            QRegExp exp("[\\/]Indicator[0-9]{1,4}[\\/]");
            int index = key.lastIndexOf(exp);
            if (index < 0 || exp.matchedLength() < 0)
                continue;

            QString newKey = "Indicator" + QString::number(indicatorCount);
            newKey.append("/");
            newKey.append(key.right(key.length() - (index + exp.matchedLength())));
            strategieIndicators.setValue(newKey, value);
        }
    }

    strategieIndicators.sync();
    createBTTStrategieMenuEntry(title);
}

void Strategie::importBTTStrategie(void)
{
    if (!currentPriceChart())
    {
        QMessageBox::warning(this, tr("Waarschuwing"), tr("Selecteer eerst een handelsbron uit het bron menu."));
        return;
    }

    QString file = QFileDialog::getOpenFileName(this, tr("Importeer"), QDir::homePath(), tr("Strategie (*.bts)"));
    if (file.isNull())
        return;

    QSettings strategieIndicators(file, QSettings::IniFormat);
    if (strategieIndicators.value("ContentType") != QString("Strategie"))
    {
        QMessageBox::warning(this, tr("Waarschuwing"), tr("Dit is geen geldig strategie bestand."));
        return;
    }

    QStringList keys = strategieIndicators.allKeys();
    if (keys.isEmpty())
    {
        QMessageBox::warning(this, tr("Waarschuwing"), tr("Dit is geen leeg strategie bestand."));
        return;
    }

retry:
    bool ok;
    QString title = QInputDialog::getText(this, tr("Geef naam"), tr("Geef naam:"), QLineEdit::Normal, QString(), &ok);
    if (!ok)
        return;

    QList<QAction *> actionList = mainWindow->ui->strategyMenu->actions();// strategieMenu->actions();
    if (!actionList.isEmpty())
    {
        for (int i = 0; i < actionList.size(); ++i)
        {
            if (actionList.at(i)->text() == title)
            {
                QMessageBox::warning(this, tr("Waarschuwing"), tr("Er bestaat al een strategie met deze titel.\n"));
                goto retry;
            }
        }
    }

    while (!settings.group().isEmpty())
        settings.endGroup();

    QStringList strategieMenuList =  settings.value("Strategie/StrategieMenuEntries").toStringList();
    strategieMenuList << title;
    settings.setValue("strategie/StrategieMenuEntries", strategieMenuList);
    settings.sync();

    QSettings newStrategieFile(QApplication::organizationName(), title);
    newStrategieFile.clear();
    newStrategieFile.sync();

    for (int i = 0; i < keys.size(); ++i)
    {
        QVariant value = strategieIndicators.value(keys.at(i));
        newStrategieFile.setValue(keys.at(i), value);
    }

    newStrategieFile.sync();
    createBTTStrategieMenuEntry(title);
}

void Strategie::exportBTTStrategie(void)
{
    if (!currentPriceChart())
    {
        QMessageBox::warning(this, tr("Waarschuwing"), tr("Selecteer eerst een handelsbron uit het bron menu."));
        return;
    }

    QString file = QFileDialog::getSaveFileName(this, tr("Exporteer"), QString(QDir::homePath() + tr("/nieuw.bts")));
    if (file.isNull())
        return;

    QSettings strategieIndicators(file, QSettings::IniFormat);
    strategieIndicators.clear();
    strategieIndicators.sync();
    strategieIndicators.setValue("ContentType", "Strategie");
    int indicatorCount = -1;

    while (!settings.group().isEmpty())
        settings.endGroup();

    for (int i = 0; i < currentPriceChart()->indicatorList.size(); ++i)
    {
        if (currentPriceChart()->indicatorList.at(i) == currentPriceChart()->trendIndicator() ||
            currentPriceChart()->indicatorList.at(i) == currentPriceChart()->dominantCycleIndicator() ||
            currentPriceChart()->indicatorList.at(i) == currentPriceChart()->trendVigorIndicator() ||
            !currentPriceChart()->indicatorList.at(i)->autoRestore() ||
            currentPriceChart()->indicatorList.at(i)->processMode() == Indicator::Hidden)
            continue;

        QStringList keys = settings.allKeys();
        bool hasConfig = false;
        for (int j = 0; j < keys.size(); ++j)
        {
            if (!keys.at(j).contains(currentPriceChart()->indicatorList.at(i)->configGroupKey()))
                continue;

            if (!hasConfig)
            {
                ++indicatorCount;
                hasConfig = true;
            }

            QVariant value = settings.value(keys.at(j));
            QString key = keys.at(j);
            QRegExp exp("[\\/]Indicator[0-9]{1,4}[\\/]");
            int index = key.lastIndexOf(exp);
            if (index < 0 || exp.matchedLength() < 0)
                continue;

            QString newKey = "Indicator" + QString::number(indicatorCount);
            newKey.append("/");
            newKey.append(key.right(key.length() - (index + exp.matchedLength())));
            strategieIndicators.setValue(newKey, value);
        }
    }

    strategieIndicators.sync();
}

void Strategie::createBTTStrategieMenuEntry(const QString title)
{
    QAction *action = mainWindow->ui->strategyMenu->addAction(title);
    connect(action, SIGNAL(triggered()), this, SLOT(loadBTTStrategie()));

    MenuContextMenu *menucontextMenu = new MenuContextMenu(mainWindow->ui->strategyMenu, action);

    QAction *remove = menucontextMenu->addAction(tr("Verwijderen"));
    remove->setIcon(QIcon::fromTheme("edit-delete"));
    connect(remove, SIGNAL(triggered()), this, SLOT(removeBTTStrategie()));

    QAction *rename = menucontextMenu->addAction(tr("Hernoemen"));
    rename->setIcon(QIcon::fromTheme("edit-find-replace"));
    connect(rename, SIGNAL(triggered()), this, SLOT(renameBTTStrategie()));
}

void Strategie::removeBTTStrategie(void)
{
    QObject *senderObject = sender();
    if (!senderObject)
        return;

    QAction *action = qobject_cast<QAction *>(senderObject);
    if (!action)
        return;

    MenuContextMenu *menu = qobject_cast<MenuContextMenu *>(action->parent());
    if (!menu)
        return;

    action = menu->action();
    if (!action)
        return;

    QString title = action->text();

    qDebug() << "Strategie::removeBTTStrategie(): " << title; //<< mainWindow->ui->menuStrategie->activeAction()->text();

    QSettings strategieFile(QApplication::organizationName(), title);
    strategieFile.clear();
    strategieFile.sync();
    QFile::remove(strategieFile.fileName());

    while (!settings.group().isEmpty())
        settings.endGroup();

    QStringList strategieMenuList =  settings.value("Strategie/StrategieMenuEntries").toStringList();
    strategieMenuList.removeAll(title);
    settings.setValue("strategie/StrategieMenuEntries", strategieMenuList);
    settings.sync();

    mainWindow->ui->strategyMenu->removeAction(action);
}

void Strategie::renameBTTStrategie(void)
{
    QObject *senderObject = sender();
    if (!senderObject)
        return;

    QAction *action = qobject_cast<QAction *>(senderObject);
    if (!action)
        return;

    MenuContextMenu *menu = qobject_cast<MenuContextMenu *>(action->parent());
    if (!menu)
        return;

    action = menu->action();
    if (!action)
        return;

retry:
    bool ok;
    QString newTitle = QInputDialog::getText(this, tr("Geef een nieuwe naam op"), tr("Geef een nieuwe naam op"), QLineEdit::Normal, QString(), &ok);
    if (!ok)
        return;

    QList<QAction *> actionsList = mainWindow->ui->strategyMenu->actions();
    for (int i = 0; i < actionsList.size(); ++i)
    {
        if (actionsList.at(i)->text() == newTitle)
        {
            QMessageBox::warning(this, tr("Waarschuwing"), tr("Deze titel bestaat al.\n"));
            goto retry;
        }
    }

    QString oldTitle = action->text();

    qDebug() << "Strategie::renameBTTStrategie(): " << oldTitle << " to " << newTitle;
    QSettings oldStrategieFile(QApplication::organizationName(), oldTitle);
    QStringList keys = oldStrategieFile.allKeys();

    QSettings newStrategieFile(QApplication::organizationName(), newTitle);
    newStrategieFile.clear();
    newStrategieFile.sync();

    for (int i = 0; i < keys.size(); ++i)
    {
        QVariant value = oldStrategieFile.value(keys.at(i));
        newStrategieFile.setValue(keys.at(i), value);
    }
    newStrategieFile.sync();

    oldStrategieFile.clear();
    oldStrategieFile.sync();
    QFile::remove(oldStrategieFile.fileName());

    while (!settings.group().isEmpty())
        settings.endGroup();

    QStringList strategieMenuList =  settings.value("Strategie/StrategieMenuEntries").toStringList();
    strategieMenuList.removeAll(oldTitle);
    strategieMenuList << newTitle;
    settings.setValue("strategie/StrategieMenuEntries", strategieMenuList);
    settings.sync();

    action->setText(newTitle);
}

void Strategie::fillStrategieMenu(void)
{
    QStringList strategieMenuList =  settings.value("Strategie/StrategieMenuEntries").toStringList();
    for (int i = 0; i < strategieMenuList.size(); ++i)
    {
        createBTTStrategieMenuEntry(strategieMenuList.at(i));
    }
}

void Strategie::setMouseTracking(bool enabled)
{
    mouseTracking = enabled;


    for (int i = 0; i < priceChartList.size(); i++)
    {
        PriceChart *priceChart = priceChartList.at(i);
        if (!priceChart || !priceChart->indexPlot)
            continue;

        priceChart->indexPlot->tracker()->setEnabled(enabled);
        for (int j = 0; j < priceChart->indicatorList.size(); j++)
            priceChart->indicatorList.at(j)->indicatorPlot->tracker()->setEnabled(enabled);
    }
}

void Strategie::zoomIn(void)
{
    if (!currentPriceChart())
        return;

    currentPriceChart()->indexPlot->zoomIn();
}

void Strategie::zoomOut(void)
{
    if (!currentPriceChart())
        return;

    currentPriceChart()->indexPlot->zoomOut();
}

void Strategie::zoomReset(void)
{
    if (!currentPriceChart())
        return;

    currentPriceChart()->indexPlot->zoomReset();
}

void Strategie::fullHistoryView(void)
{
    PriceChart *priceChart = currentPriceChart();
    if (!priceChart)
        return;

    if (startDateEdit->date() == priceChart->firstAvailableDateTime().date() && endDateEdit->date() == priceChart->lastAvailableDateTime().date())
        return;

    bool chartUpdatesEnabled = testAttribute(Qt::WA_UpdatesDisabled);
    setAttribute(Qt::WA_UpdatesDisabled);

    bool idec = strategie->ignoreDataEditChanged();
    setIgnoreDateEditChanged(true);
    startDateEdit->setDate(priceChart->firstAvailableDateTime().date());
    endDateEdit->setDate(priceChart->lastAvailableDateTime().date());
    setIgnoreDateEditChanged(idec);

    setStartDate(priceChart->firstAvailableDateTime().date());

    setAttribute(Qt::WA_UpdatesDisabled, chartUpdatesEnabled);
}

void Strategie::rewindDate(int timeSkip)
{
    PriceChart *priceChart = currentPriceChart();
    if (!priceChart)
        return;

    if (startDateEdit->date() == priceChart->firstAvailableDateTime().date())
        return;

    bool chartUpdatesEnabled = testAttribute(Qt::WA_UpdatesDisabled);
    setAttribute(Qt::WA_UpdatesDisabled);

    QDate newDate;

    switch (timeSkip)
    {
        case Day:
            goto rewindDay;
            break;

        case Week:
            goto rewindWeek;
            break;

        case Month:
            goto rewindMonth;
            break;

        case Year:
            goto rewindYear;
            break;

        default:
            break;
    }

    switch (priceChart->timeScale())
    {
        case MinuteByMinute:
        rewindDay:
            newDate = startDateEdit->date().addDays(-1);
            if (newDate < priceChart->firstAvailableDateTime().date())
            {
                int timeTo = startDateEdit->date().daysTo(endDateEdit->date());
                QDateTime endDateTime;
                endDateTime.setDate(priceChart->firstAvailableDateTime().date().addDays(timeTo));
                endDateTime.setTime(QTime(23, 59, 59));

                priceChart->loadData(priceChart->firstAvailableDateTime(), endDateTime);
            }
            else
            {
                int days = -1;
                if (startDateEdit->date().dayOfWeek() == 1)
                    days = -3;
                else if (startDateEdit->date().dayOfWeek() == 7)
                    days = -2;

                QDateTime startDateTime;
                startDateTime.setDate(startDateEdit->date().addDays(days));
                startDateTime.setTime(QTime(0, 0, 0));

                days = -1;
                if (endDateEdit->date().dayOfWeek() == 1)
                    days = -3;
                else if (endDateEdit->date().dayOfWeek() == 7)
                    days = -2;

                QDateTime endDateTime;
                endDateTime.setDate(endDateEdit->date().addDays(-1));
                endDateTime.setTime(QTime(23, 59, 59));

                priceChart->loadData(startDateTime, endDateTime);
            }
            break;

        rewindWeek:
        case HourByHour:
            newDate = startDateEdit->date().addDays(-7);
            if (newDate < priceChart->firstAvailableDateTime().date())
            {
                int timeTo = startDateEdit->date().daysTo(endDateEdit->date());
                QDateTime endDateTime;
                endDateTime.setDate(priceChart->firstAvailableDateTime().date().addDays(timeTo));
                endDateTime.setTime(QTime(23, 59, 59));

                priceChart->loadData(priceChart->firstAvailableDateTime(), endDateTime);
            }
            else
            {
                QDateTime startDateTime;
                startDateTime.setDate(startDateEdit->date().addDays(-7));
                startDateTime.setTime(QTime(0, 0, 0));

                QDateTime endDateTime;
                endDateTime.setDate(endDateEdit->date().addDays(-7));
                endDateTime.setTime(QTime(23, 59, 59));

                priceChart->loadData(startDateTime, endDateTime);
            }
            break;

        case WeekByWeek:
        rewindMonth:
            newDate = startDateEdit->date().addMonths(-1);
            if (newDate < priceChart->firstAvailableDateTime().date())
            {
                int timeTo = startDateEdit->date().daysTo(endDateEdit->date());
                QDateTime endDateTime;
                endDateTime.setDate(priceChart->firstAvailableDateTime().date().addDays(timeTo));
                endDateTime.setTime(QTime(23, 59, 59));

                priceChart->loadData(priceChart->firstAvailableDateTime(), endDateTime);
            }
            else
            {
                QDateTime startDateTime;
                startDateTime.setDate(startDateEdit->date().addMonths(-1));
                startDateTime.setTime(QTime(0, 0, 0));

                QDateTime endDateTime;
                endDateTime.setDate(endDateEdit->date().addMonths(-1));
                endDateTime.setTime(QTime(23, 59, 59));

                priceChart->loadData(startDateTime, endDateTime);
            }
            break;

        case DayByDay:
            newDate = startDateEdit->date().addMonths(-1);
            if (newDate < priceChart->firstAvailableDateTime().date())
            {
                int timeTo = startDateEdit->date().daysTo(endDateEdit->date());
                QDateTime endDateTime;
                endDateTime.setDate(priceChart->firstAvailableDateTime().date().addDays(timeTo));
                endDateTime.setTime(QTime(23, 59, 59));

                priceChart->loadData(priceChart->firstAvailableDateTime(), endDateTime);
            }
            else
            {
                QDateTime startDateTime;
                startDateTime.setDate(startDateEdit->date().addMonths(-1));
                startDateTime.setTime(QTime(0, 0, 0));

                QDateTime endDateTime;
                endDateTime.setDate(endDateEdit->date().addMonths(-1));
                endDateTime.setTime(QTime(23, 59, 59));

                priceChart->loadData(startDateTime, endDateTime);
            }
            break;

        case MonthByMonth:
        rewindYear:
            newDate = startDateEdit->date().addYears(-1);
            if (newDate < priceChart->firstAvailableDateTime().date())
            {
                int timeTo = startDateEdit->date().daysTo(endDateEdit->date());
                QDateTime endDateTime;
                endDateTime.setDate(priceChart->firstAvailableDateTime().date().addDays(timeTo));
                endDateTime.setTime(QTime(23, 59, 59));

                priceChart->loadData(priceChart->firstAvailableDateTime(), endDateTime);
            }
            else
            {
                QDateTime startDateTime;
                startDateTime.setDate(startDateEdit->date().addYears(-1));
                startDateTime.setTime(QTime(0, 0, 0));

                QDateTime endDateTime;
                endDateTime.setDate(endDateEdit->date().addYears(-1));
                endDateTime.setTime(QTime(23, 59, 59));

                priceChart->loadData(startDateTime, endDateTime);
            }
            break;

        default:
            break;
    }

    setAttribute(Qt::WA_UpdatesDisabled, chartUpdatesEnabled);
}

void Strategie::forwardDate(int timeSkip)
{
    PriceChart *priceChart = currentPriceChart();
    if (!priceChart)
        return;

    if (endDateEdit->date() == priceChart->lastAvailableDateTime().date())
        return;

    bool chartUpdatesEnabled = testAttribute(Qt::WA_UpdatesDisabled);
    setAttribute(Qt::WA_UpdatesDisabled);

    QDate newDate;
    switch (timeSkip)
    {
        case Day:
            goto forwardDay;
            break;

        case Week:
            goto forwardWeek;
            break;

        case Month:
            goto forwardMonth;
            break;

        case Year:
            goto forwardYear;
            break;

        default:
            break;
    }

    switch (priceChart->timeScale())
    {
        forwardDay:
        case MinuteByMinute:
            newDate = endDateEdit->date().addDays(1);
            if (newDate > priceChart->lastAvailableDateTime().date())
            {
                int timeTo = endDateEdit->date().daysTo(startDateEdit->date());
                QDateTime startDateTime;
                startDateTime.setDate(priceChart->lastAvailableDateTime().date().addDays(timeTo));
                startDateTime.setTime(QTime(0, 0, 0));

                priceChart->loadData(startDateTime, priceChart->lastAvailableDateTime());
            }
            else
            {
                int days = 1;
                if (startDateEdit->date().dayOfWeek() == 5)
                    days = 3;
                else if (startDateEdit->date().dayOfWeek() == 6)
                    days = 2;

                QDateTime startDateTime;
                startDateTime.setDate(startDateEdit->date().addDays(days));
                startDateTime.setTime(QTime(0, 0, 0));

                days = 1;
                if (endDateEdit->date().dayOfWeek() == 5)
                    days = 3;
                else if (endDateEdit->date().dayOfWeek() == 6)
                    days = 2;

                QDateTime endDateTime;
                endDateTime.setDate(endDateEdit->date().addDays(days));
                endDateTime.setTime(QTime(23, 59, 59));

                priceChart->loadData(startDateTime, endDateTime);

            }
            break;

        forwardWeek:
        case HourByHour:
            newDate = endDateEdit->date().addDays(7);
            if (newDate > priceChart->lastAvailableDateTime().date())
            {
                int timeTo = endDateEdit->date().daysTo(startDateEdit->date());
                QDateTime startDateTime;
                startDateTime.setDate(priceChart->lastAvailableDateTime().date().addDays(timeTo));
                startDateTime.setTime(QTime(0, 0, 0));

                priceChart->loadData(startDateTime, priceChart->lastAvailableDateTime());
            }
            else
            {
                QDateTime startDateTime;
                startDateTime.setDate(startDateEdit->date().addDays(7));
                startDateTime.setTime(QTime(0, 0, 0));

                QDateTime endDateTime;
                endDateTime.setDate(endDateEdit->date().addDays(7));
                endDateTime.setTime(QTime(23, 59, 59));

                priceChart->loadData(startDateTime, endDateTime);
            }
            break;

        forwardMonth:
        case WeekByWeek:
            newDate = endDateEdit->date().addMonths(1);
            if (newDate > priceChart->lastAvailableDateTime().date())
            {
                int timeTo = endDateEdit->date().daysTo(startDateEdit->date());
                QDateTime startDateTime;
                startDateTime.setDate(priceChart->lastAvailableDateTime().date().addDays(timeTo));
                startDateTime.setTime(QTime(0, 0, 0));

                priceChart->loadData(startDateTime, priceChart->lastAvailableDateTime());
            }
            else
            {
                QDateTime startDateTime;
                startDateTime.setDate(startDateEdit->date().addMonths(1));
                startDateTime.setTime(QTime(0, 0, 0));

                QDateTime endDateTime;
                endDateTime.setDate(endDateEdit->date().addMonths(1));
                endDateTime.setTime(QTime(23, 59, 59));

                priceChart->loadData(startDateTime, endDateTime);
            }
            break;

        case DayByDay:
            newDate = endDateEdit->date().addMonths(1);
            if (newDate > priceChart->lastAvailableDateTime().date())
            {
                int timeTo = endDateEdit->date().daysTo(startDateEdit->date());
                QDateTime startDateTime;
                startDateTime.setDate(priceChart->lastAvailableDateTime().date().addDays(timeTo));
                startDateTime.setTime(QTime(0, 0, 0));

                priceChart->loadData(startDateTime, priceChart->lastAvailableDateTime());
            }
            else
            {
                QDateTime startDateTime;
                startDateTime.setDate(startDateEdit->date().addMonths(1));
                startDateTime.setTime(QTime(0, 0, 0));

                QDateTime endDateTime;
                endDateTime.setDate(endDateEdit->date().addMonths(1));
                endDateTime.setTime(QTime(23, 59, 59));

                priceChart->loadData(startDateTime, endDateTime);
            }
            break;

        forwardYear:
        case MonthByMonth:
            newDate = endDateEdit->date().addYears(1);
            if (newDate > priceChart->lastAvailableDateTime().date())
            {
                int timeTo = endDateEdit->date().daysTo(startDateEdit->date());
                QDateTime startDateTime;
                startDateTime.setDate(priceChart->lastAvailableDateTime().date().addDays(timeTo));
                startDateTime.setTime(QTime(0, 0, 0));

                priceChart->loadData(startDateTime, priceChart->lastAvailableDateTime());
            }
            else
            {
                QDateTime startDateTime;
                startDateTime.setDate(startDateEdit->date().addYears(1));
                startDateTime.setTime(QTime(0, 0, 0));

                QDateTime endDateTime;
                endDateTime.setDate(endDateEdit->date().addYears(1));
                endDateTime.setTime(QTime(23, 59, 59));

                priceChart->loadData(startDateTime, endDateTime);
            }
            break;

        default:
            break;
    }

    setAttribute(Qt::WA_UpdatesDisabled, chartUpdatesEnabled);
}

void Strategie::setStartDate(QDate date)
{
    PriceChart *priceChart = currentPriceChart();
    if (!priceChart)
        return;

    if (ignoreDateEditChangedSignal)
        return;

    bool chartUpdatesEnabled = testAttribute(Qt::WA_UpdatesDisabled);
    setAttribute(Qt::WA_UpdatesDisabled);

    QDateTime startDateTime;
    startDateTime.setDate(date);
    startDateTime.setTime(QTime(0, 0, 0));
    QDateTime endDateTime;
    endDateTime.setDate(endDateEdit->date());
    endDateTime.setTime(QTime(23, 59, 59));
    priceChart->loadData(startDateTime, endDateTime);

    setAttribute(Qt::WA_UpdatesDisabled, chartUpdatesEnabled);
}


void Strategie::setEndDate(QDate date)
{
    PriceChart *priceChart = currentPriceChart();
    if (!priceChart)
        return;

    if (ignoreDateEditChangedSignal)
        return;

    bool chartUpdatesEnabled = testAttribute(Qt::WA_UpdatesDisabled);
    setAttribute(Qt::WA_UpdatesDisabled);

    QDateTime startDateTime;
    startDateTime.setDate(startDateEdit->date());
    startDateTime.setTime(QTime(0, 0, 0));
    QDateTime endDateTime;
    endDateTime.setDate(date);
    endDateTime.setTime(QTime(23, 59, 59));
    priceChart->loadData(startDateTime, endDateTime);

    setAttribute(Qt::WA_UpdatesDisabled, chartUpdatesEnabled);
}

void Strategie::updateStartDate(const QDateTime &dateTime)
{
    bool idec = ignoreDataEditChanged();
    setIgnoreDateEditChanged(true);
    startDateEdit->setDate(dateTime.date());
    setIgnoreDateEditChanged(idec);
}

void Strategie::updateEndDate(const QDateTime &dateTime)
{
    bool idec = ignoreDataEditChanged();
    setIgnoreDateEditChanged(true);
    endDateEdit->setDate(dateTime.date());
    setIgnoreDateEditChanged(idec);
}

void Strategie::updateFirstAvailableCalendarDate(const QDateTime &dateTime)
{
    bool idec = ignoreDataEditChanged();
    setIgnoreDateEditChanged(true);
    startDateEdit->calendarWidget()->setMinimumDate(dateTime.date());
    startDateEdit->setMinimumDateTime(dateTime);
    endDateEdit->calendarWidget()->setMinimumDate(dateTime.date());
    endDateEdit->setMinimumDateTime(dateTime);
    setIgnoreDateEditChanged(idec);
}

void Strategie::updateLastAvailableCalendarDate(const QDateTime &dateTime)
{
    bool idec = ignoreDataEditChanged();
    setIgnoreDateEditChanged(true);
    startDateEdit->calendarWidget()->setMaximumDate(dateTime.date());
    startDateEdit->setMaximumDateTime(dateTime);
    endDateEdit->calendarWidget()->setMaximumDate(dateTime.date());
    endDateEdit->setMaximumDateTime(dateTime);
    setIgnoreDateEditChanged(idec);
}

void Strategie::switchToMinuteTimeScale(void)
{
    PriceChart *strategieTab = currentPriceChart();
    if (!strategieTab)
        return;

    bool chartUpdatesEnabled = testAttribute(Qt::WA_UpdatesDisabled);
    setAttribute(Qt::WA_UpdatesDisabled);

    mainWindow->ui->action1_minute->setCheckable(true);
    mainWindow->ui->action1_minute->setChecked(true);
    mainWindow->ui->actionHourly->setCheckable(false);
    mainWindow->ui->actionDaily->setCheckable(false);
    mainWindow->ui->actionWeekly->setCheckable(false);
    mainWindow->ui->actionMonthly->setCheckable(false);

    strategieTab->setTimeScale(MinuteByMinute);

    setAttribute(Qt::WA_UpdatesDisabled, chartUpdatesEnabled);
}

void Strategie::switchToHourTimeScale(void)
{
    PriceChart *strategieTab = currentPriceChart();
    if (!strategieTab)
        return;

    bool chartUpdatesEnabled = testAttribute(Qt::WA_UpdatesDisabled);
    setAttribute(Qt::WA_UpdatesDisabled);

    mainWindow->ui->action1_minute->setCheckable(false);
    mainWindow->ui->actionHourly->setCheckable(true);
    mainWindow->ui->actionHourly->setChecked(true);
    mainWindow->ui->actionDaily->setCheckable(false);
    mainWindow->ui->actionWeekly->setCheckable(false);
    mainWindow->ui->actionMonthly->setCheckable(false);

    strategieTab->setTimeScale(HourByHour);

    setAttribute(Qt::WA_UpdatesDisabled, chartUpdatesEnabled);
}

void Strategie::switchToDayTimeScale(void)
{
    PriceChart *strategieTab = currentPriceChart();
    if (!strategieTab)
        return;

    bool chartUpdatesEnabled = testAttribute(Qt::WA_UpdatesDisabled);
    setAttribute(Qt::WA_UpdatesDisabled);

    mainWindow->ui->action1_minute->setCheckable(false);
    mainWindow->ui->actionHourly->setCheckable(false);
    mainWindow->ui->actionDaily->setCheckable(true);
    mainWindow->ui->actionDaily->setChecked(true);
    mainWindow->ui->actionWeekly->setCheckable(false);
    mainWindow->ui->actionMonthly->setCheckable(false);

    strategieTab->setTimeScale(DayByDay);

    setAttribute(Qt::WA_UpdatesDisabled, chartUpdatesEnabled);
}

void Strategie::switchToWeekTimeScale(void)
{
    PriceChart *strategieTab = currentPriceChart();
    if (!strategieTab)
        return;

    bool chartUpdatesEnabled = testAttribute(Qt::WA_UpdatesDisabled);
    setAttribute(Qt::WA_UpdatesDisabled);

    mainWindow->ui->action1_minute->setCheckable(false);
    mainWindow->ui->actionHourly->setCheckable(false);
    mainWindow->ui->actionDaily->setCheckable(false);
    mainWindow->ui->actionWeekly->setCheckable(true);
    mainWindow->ui->actionWeekly->setChecked(true);
    mainWindow->ui->actionMonthly->setCheckable(false);

    strategieTab->setTimeScale(WeekByWeek);

    setAttribute(Qt::WA_UpdatesDisabled, chartUpdatesEnabled);
}

void Strategie::switchToMonthTimeScale(void)
{
    PriceChart *strategieTab = currentPriceChart();
    if (!strategieTab)
        return;

    bool chartUpdatesEnabled = testAttribute(Qt::WA_UpdatesDisabled);
    setAttribute(Qt::WA_UpdatesDisabled);

    mainWindow->ui->action1_minute->setCheckable(false);
    mainWindow->ui->actionHourly->setCheckable(false);
    mainWindow->ui->actionDaily->setCheckable(false);
    mainWindow->ui->actionWeekly->setCheckable(false);
    mainWindow->ui->actionMonthly->setCheckable(true);
    mainWindow->ui->actionMonthly->setChecked(true);

    strategieTab->setTimeScale(MonthByMonth);

    setAttribute(Qt::WA_UpdatesDisabled, chartUpdatesEnabled);
}

void Strategie::updateTimeScaleMenu(void)
{
    PriceChart *strategieTab = currentPriceChart();
    if (!strategieTab)
        return;

    mainWindow->ui->action1_minute->setCheckable(false);
    mainWindow->ui->actionHourly->setCheckable(false);
    mainWindow->ui->actionDaily->setCheckable(false);
    mainWindow->ui->actionWeekly->setCheckable(false);
    mainWindow->ui->actionMonthly->setCheckable(false);

    QAction *action;
    switch (currentPriceChart()->timeScale())
    {
        case MinuteByMinute:
            action = mainWindow->ui->action1_minute;
            break;

        case HourByHour:
            action = mainWindow->ui->actionHourly;
            break;

        case DayByDay:
            action = mainWindow->ui->actionDaily;
            break;

        case WeekByWeek:
            action = mainWindow->ui->actionWeekly;
            break;

        case MonthByMonth:
            action = mainWindow->ui->actionMonthly;
            break;

        default:
            return;
    }

    action->setCheckable(true);
    action->setChecked(true);
}

QString Strategie::timeScaleToString(int timeScale)
{
    QString timeScaleString;

    switch (timeScale)
    {
        case TickByTick:
            timeScaleString = tr("Ticks");
            break;

        case MinuteByMinute:
            timeScaleString = tr("Minuten");
            break;

        case HourByHour:
            timeScaleString = tr("Uren");
            break;

        case DayByDay:
            timeScaleString = tr("Dagen");
            break;

        case WeekByWeek:
            timeScaleString = tr("Weken");
            break;

        case MonthByMonth:
            timeScaleString = tr("Maanden");
            break;

        default:
            timeScaleString = "";
            break;
    }

    return timeScaleString;
}

int Strategie::globalStopLossType(void)
{
    return privateData->stopLossType;
}

void Strategie::setGlobalStopLossType(int type)
{
    privateData->stopLossType = type;
}

double Strategie::globalStopLossValue(void)
{
    return privateData->stopLossValue;
}

void Strategie::setGlobalStopLossValue(double value)
{
    privateData->stopLossValue = value;
}

double Strategie::globalLongStopLossValue(void)
{
    return privateData->longStopLossValue;
}

void Strategie::setGlobalLongStopLossValue(double value)
{
    privateData->longStopLossValue = value;
}

double Strategie::globalShortStopLossValue(void)
{
    return privateData->shortStopLossValue;
}

void Strategie::setGlobalShortStopLossValue(double value)
{
    privateData->shortStopLossValue = value;
}

double Strategie::globalProfitTrailingThreshold(void)
{
    return privateData->profitTrailingThreshold;
}

void Strategie::setGlobalProfitTrailingThresHold(double threshold)
{
    privateData->profitTrailingThreshold = threshold;
}

double Strategie::globalLongProfitTrailingThreshold(void)
{
    return privateData->longProfitTrailingThreshold;
}

void Strategie::setGlobalLongProfitTrailingThresHold(double threshold)
{
    privateData->longProfitTrailingThreshold = threshold;
}

double Strategie::globalShortProfitTrailingThreshold(void)
{
    return privateData->shortProfitTrailingThreshold;
}

void Strategie::setGlobalShortProfitTrailingThresHold(double threshold)
{
    privateData->shortProfitTrailingThreshold = threshold;
}

double Strategie::globalMaximumStopLossFactor(void)
{
    return privateData->maximumStopLossFactor;
}

void Strategie::setGlobalMaximumStopLossFactor(double factor)
{
    privateData->maximumStopLossFactor = factor;
}

double Strategie::globalMaximumThresholdStopLossFactor(void)
{
    return privateData->maximumThresholdStopLossFactor;
}

void Strategie::setGlobalMaximumThresholdStopLossFactor(double factor)
{
    privateData->maximumThresholdStopLossFactor = factor;
}

double Strategie::globalLongMaximumStopLossFactor(void)
{
    return privateData->longMaximumStopLossFactor;
}

void Strategie::setGlobalLongMaximumStopLossFactor(double factor)
{
    privateData->longMaximumStopLossFactor = factor;
}

double Strategie::globalLongMaximumThresholdStopLossFactor(void)
{
    return privateData->longMaximumThresholdStopLossFactor;
}

void Strategie::setGlobalLongMaximumThresholdStopLossFactor(double factor)
{
    privateData->longMaximumThresholdStopLossFactor = factor;
}

double Strategie::globalShortMaximumStopLossFactor(void)
{
    return privateData->shortMaximumStopLossFactor;
}

void Strategie::setGlobalShortMaximumStopLossFactor(double factor)
{
    privateData->shortMaximumStopLossFactor = factor;
}

double Strategie::globalShortMaximumThresholdStopLossFactor(void)
{
    return privateData->shortMaximumThresholdStopLossFactor;
}

void Strategie::setGlobalShortMaximumThresholdStopLossFactor(double factor)
{
    privateData->shortMaximumThresholdStopLossFactor = factor;
}

int Strategie::globalStopLossHits(void)
{
    return privateData->stopLossHits;
}

void Strategie::setGlobalStopLossHits(int hits)
{
    privateData->stopLossHits = hits;
}

int Strategie::globalLongStopLossHits(void)
{
    return privateData->longStopLossHits;
}

void Strategie::setGlobalLongStopLossHits(int hits)
{
    privateData->longStopLossHits = hits;
}

int Strategie::globalShortStopLossHits(void)
{
    return privateData->shortStopLossHits;
}

void Strategie::setGlobalShortStopLossHits(int hits)
{
    privateData->shortStopLossHits = hits;
}
