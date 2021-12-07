#include "line.h"
#include <qwt_symbol.h>
#include <qdialog.h>
#include <qcolordialog.h>
#include <QMouseEvent>

Line::Line(Plot *plot, PriceChart *priceChart, Line::Mode lineMode, bool setupDialog, const QString &configGroup, Indicator::ProcessMode processMode) :
    Indicator(plot, priceChart, setupDialog, configGroup, processMode)
{
    lineCurve = new PlotCurve(tr("Lijn"), indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode), PlotCurve::Line);
    lineCurve->setSymbol(QwtSymbol(QwtSymbol::Ellipse, lineCurve->pen().brush(), lineCurve->pen(), QSize(8, 8)));
    curveList.append(lineCurve);

    this->lineMode = lineMode;
    trackerEnabled = indicatorPlot->tracker()->enabled();
    if (configGroup.isEmpty())
    {
        startDrawing = true;
        endDrawing = false;
        indicatorPlot->tracker()->setEnabled(false);
        loadDefaultRules();
    }
    else
    {
        startDrawing = false;
        endDrawing = true;
    }

    indicatorPlot->canvas()->installEventFilter(this);
    connect(indicatorPlot->picker(), SIGNAL(curveMoved(PlotCurve &)), this, SLOT(curveMoved(PlotCurve &)));
}

Line::~Line(void)
{
}

void Line::readSettingsPrivate(void)
{
    lineMode = static_cast<Line::Mode>(settings.value("Mode", lineMode).toInt());
    sticky = settings.value("Sticky", false).toBool();
    stickTo = static_cast<Line::Sticky>(settings.value("StickTo", Line::StickToFree).toInt());
    startPoint = settings.value("StartPoint", QPointF()).toPointF();
    endPoint = settings.value("EndPoint", QPointF()).toPointF();
    startDateTime = settings.value("StartDateTime", QDateTime()).toDateTime();
    endDateTime = settings.value("EndDateTime", QDateTime()).toDateTime();
    timeScale = settings.value("TimeScale", -1).toInt();
    isin = settings.value("Isin", "").toString();

    QVector<QPointF> data;
    data << startPoint << endPoint;
    lineCurve->setData(data);

    switch (stickTo)
    {
        case StickToFree:
            sticky = false;
            break;

        case StickToOpen:
            sticky = true;
            indicatorSourceCurve = priceChart->averagePriceOpen;
            break;

        case StickToHigh:
            sticky = true;
            indicatorSourceCurve = priceChart->averagePriceHigh;
            break;

        case StickToLow:
            sticky = true;
            indicatorSourceCurve = priceChart->averagePriceLow;
            break;

        case StickToClose:
            sticky = true;
            indicatorSourceCurve = priceChart->averagePriceClose;
            break;

        default:
            sticky = false;
            break;
    }

    if (sticky)
        indicatorPlot->picker()->linkCurves(lineCurve, indicatorSourceCurve);
    else
        indicatorPlot->picker()->linkCurves(lineCurve, NULL);

    Indicator::readSettingsPrivate();
}

void Line::writeSettingsPrivate(void)
{
    if (lineCurve->dataSize() == 0)
        return;

    settings.setValue("Type", TrendLineIndicator);
    settings.setValue("Mode", lineMode);
    settings.setValue("Sticky", sticky);
    settings.setValue("StickTo", stickTo);
    settings.setValue("StartPoint", startPoint);
    settings.setValue("EndPoint", endPoint);
    settings.setValue("StartDateTime", startDateTime);
    settings.setValue("EndDateTime", endDateTime);
    settings.setValue("TimeScale", timeScale);
    settings.setValue("Isin", isin);

    Indicator::writeSettingsPrivate();
}

void Line::loadDefaultRules(void)
{
    clearRuleLists();

    ruleList(Long)->addRule("SourceIndicator=NULL;SourceCurve=Price Close;ReferenceIndicator=this;ReferenceCurve="  + lineCurve->title().text() + ";ReferenceValue=0;Offset=0;CompareType=Kruist omhoog;LogicalOperator=En;PreviousTransactionType=Exit;Enabled=1");
    ruleList(Short)->addRule("SourceIndicator=NULL;SourceCurve=Price Close;ReferenceIndicator=this;ReferenceCurve="  + lineCurve->title().text() + ";ReferenceValue=0;Offset=0;CompareType=Kruist omlaag;LogicalOperator=En;PreviousTransactionType=Exit;Enabled=1");
    ruleList(Exit)->addRule("SourceIndicator=NULL;SourceCurve=Price Close;ReferenceIndicator=this;ReferenceCurve="  + lineCurve->title().text() + ";ReferenceValue=0;Offset=0;CompareType=Kruist omhoog;LogicalOperator=Of;PreviousTransactionType=Short;Enabled=1");
    ruleList(Exit)->addRule("SourceIndicator=NULL;SourceCurve=Price Close;ReferenceIndicator=this;ReferenceCurve="  + lineCurve->title().text() + ";ReferenceValue=0;Offset=0;CompareType=Kruist omlaag;LogicalOperator=Of;PreviousTransactionType=Long;Enabled=1");

    Indicator::loadDefaultRules();
}

void Line::plotCurve(void)
{
    if (timeScale != priceChart->timeScale())
    {
        QVector<QPointF> data;
        data << QPointF(-SHRT_MAX, -SHRT_MAX) << QPointF(-SHRT_MAX, -SHRT_MAX);
        lineCurve->setData(data);
        lineCurve->detach();
        return Indicator::plotCurve();
    }

    if (isin != priceChart->isinInfo().isin)
    {
        QVector<QPointF> data;
        data << QPointF(-SHRT_MAX, -SHRT_MAX) << QPointF(-SHRT_MAX, -SHRT_MAX);
        lineCurve->setData(data);
        lineCurve->detach();
        return Indicator::plotCurve();
    }

    if (priceChart->indexPlot->timeScaleData().size() != priceChart->indexPlot->periods)
    {
        QVector<QPointF> data;
        data << QPointF(-SHRT_MAX, -SHRT_MAX) << QPointF(-SHRT_MAX, -SHRT_MAX);
        lineCurve->setData(data);
        lineCurve->detach();
        return Indicator::plotCurve();
    }

    if (lineMode == Line::Horizontal)
    {
        QVector<QPointF> data;
        data << startPoint << endPoint;
        lineCurve->setData(data);
        lineCurve->attach(indicatorPlot);
        return Indicator::plotCurve();
    }

    for (int i = 0; i < priceChart->indexPlot->periods; i++)
    {
        if (priceChart->indexPlot->timeScaleData().at(i) == startDateTime)
        {
            qreal range = endPoint.x() - startPoint.x();
            qreal offset = ceil(startPoint.x()) - static_cast<qreal>(i);
            startPoint.setX(startPoint.x() - offset);
            endPoint.setX(startPoint.x() + range);

            QVector<QPointF> data;
            data << startPoint << endPoint;
            lineCurve->setData(data);
            lineCurve->attach(indicatorPlot);
            return Indicator::plotCurve();;
        }
        else if (priceChart->indexPlot->timeScaleData().at(i) == endDateTime)
        {
            qreal range = startPoint.x() - endPoint.x();
            qreal offset = ceil(endPoint.x()) - static_cast<qreal>(i);
            endPoint.setX(endPoint.x() - offset);
            startPoint.setX(endPoint.x() + range);

            QVector<QPointF> data;
            data << startPoint << endPoint;
            lineCurve->setData(data);
            lineCurve->attach(indicatorPlot);
            return Indicator::plotCurve();;
        }
    }

    QVector<QPointF> data;
    data << QPointF(-SHRT_MAX, -SHRT_MAX) << QPointF(-SHRT_MAX, -SHRT_MAX);
    lineCurve->setData(data);
    lineCurve->detach();

    Indicator::plotCurve();
}

void Line::updateGUI(void)
{
    if (lineMode == Line::Free)
    {
        bool block = blockSettingsGUISignals();
        setBlockSettingsGUISignals(true);

        form->stComboBox->setCurrentIndex(stickTo);

        setBlockSettingsGUISignals(block);
    }

    Indicator::updateGUI();
}

QWidget *Line::customIndicatorWidget(void)
{
    QWidget *ciWidget = NULL;
    if (lineMode == Line::Free)
    {
        ciWidget = new QWidget;
        form = new Ui::LineUI();
        form->setupUi(ciWidget);
        form->stComboBox->setCurrentIndex(stickTo);

        connect(form->stComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setProperties()));
    }

    lineCurve->setTitle(tr("Lijn"));
    setTitle(tr("Lijn"));

    return ciWidget;
}

void Line::setProperties(void)
{
    if (blockSettingsGUISignals())
        return;

    if (lineMode == Line::Free)
    {
        stickTo = static_cast<Line::Sticky>(form->stComboBox->currentIndex());
        switch (stickTo)
        {
            case StickToFree:
                sticky = false;
                break;

            case StickToOpen:
                sticky = true;
                indicatorSourceCurve = priceChart->averagePriceOpen;
                break;

            case StickToHigh:
                sticky = true;
                indicatorSourceCurve = priceChart->averagePriceHigh;
                break;

            case StickToLow:
                sticky = true;
                indicatorSourceCurve = priceChart->averagePriceLow;
                break;

            case StickToClose:
                sticky = true;
                indicatorSourceCurve = priceChart->averagePriceClose;
                break;

            default:
                sticky = false;
                break;
        }

        if (sticky)
            indicatorPlot->picker()->linkCurves(lineCurve, indicatorSourceCurve);
        else
            indicatorPlot->picker()->linkCurves(lineCurve, NULL);
    }

    lineCurve->setTitle(tr("Lijn"));
    setTitle(tr("Lijn"));

    Indicator::setProperties();
}

void Line::curveMoved(PlotCurve &curve)
{
    if (&curve != lineCurve)
        return;

    executeTransactionRules();
    if (lineCurve->dataSize() != 2)
        return;

    startPoint = lineCurve->data().first();
    endPoint = lineCurve->data().last();

    int address = ceil(startPoint.x());
    if (address >= 0 && address < lineCurve->pricePlot()->timeScaleData().size())
        startDateTime = lineCurve->pricePlot()->timeScaleData().at(address);
    else
        startDateTime = QDateTime();

    address = ceil(endPoint.x());
    if (address >= 0 && address < lineCurve->pricePlot()->timeScaleData().size())
        endDateTime = lineCurve->pricePlot()->timeScaleData().at(address);
    else
        endDateTime = QDateTime();

    timeScale = priceChart->timeScale();
    isin = priceChart->isinInfo().isin;
}

bool Line::eventFilter(QObject *object, QEvent *event)
{
    if (object != indicatorPlot->canvas())
        return Indicator::eventFilter(object, event);

    if (endDrawing == true)
        return Indicator::eventFilter(object, event);

    switch (event->type())
    {
        case QEvent::MouseButtonPress:
        {
            if (startDrawing)
            {
                double x;
                double y;

                if (sticky)
                {
                    x = lround(indicatorPlot->invTransform(lineCurve->xAxis(), ((QMouseEvent *) event)->pos().x()));
                    y = indicatorSourceCurve->y(x);
                }
                else
                {
                    x = indicatorPlot->invTransform(lineCurve->xAxis(), ((QMouseEvent *) event)->pos().x());
                    y = indicatorPlot->invTransform(lineCurve->yAxis(), ((QMouseEvent *) event)->pos().y());
                }

                QPolygonF curvePolygon;
                if (lineMode == Line::Horizontal)
                    curvePolygon << QPointF(SHRT_MIN, y) << QPointF(SHRT_MAX, y);
                else
                    curvePolygon << QPointF(x, y) << QPointF(x, y);

                timeScale = priceChart->timeScale();
                isin = priceChart->isinInfo().isin;
                lineCurve->attach(indicatorPlot);
                lineCurve->setData(curvePolygon);

                QPointF capturePoint;
                if (sticky)
                {
                    capturePoint.setX(indicatorPlot->transform(lineCurve->xAxis(), x));
                    capturePoint.setY(indicatorPlot->transform(lineCurve->yAxis(), y));
                }
                else
                {
                    capturePoint.setX(((QMouseEvent *) event)->pos().x());
                    capturePoint.setY(((QMouseEvent *) event)->pos().y());
                }

                indicatorPlot->picker()->select(capturePoint.toPoint());
                startDrawing = false;
            }

            return true;
        }

        case QEvent::MouseButtonRelease:
        {
            if (!startDrawing && !endDrawing)
            {
                indicatorPlot->picker()->unselect();
                indicatorPlot->tracker()->setEnabled(trackerEnabled);
                endDrawing = true;

                return true;
            }

            break;
        }

        default:
            break;
    }

    return Indicator::eventFilter(object, event);
}
