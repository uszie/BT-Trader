#include "strategieoverview.h"
#include "blur.h"
#include "animatedslidewidget.h"
#include "ui_strategieoverview.h"
#include <QTimeLine>
#include <qgl.h>

class StrategieOverview::PrivateData
{
    public:
        Ui::StrategieOverview *ui;
        PriceChart *priceChart;
        PriceEntry currentPrice;
        PriceEntry referencePrice;
        QStateMachine *rotateMachine;
        QState *upState;
        QState *downState;
        GraphicsBlurEffect *blurEffect;
        QPropertyAnimation *blurPulse;
        QPropertyAnimation *colorizeDirectionIndicator;
        QGraphicsPixmapItem *backgroundItem;
        QTime previousRenderTime;
        QTimer repaintTimer;
        AnimatedSlideWidget *curvePropertiesWidget;
        QTime backGroundRenderTimeStamp;
};

#define INNER_BLUR_RADIUS 10.0
#define OUTER_BLUR_RADIUS 10.0

QTimeLine *pulseTimeLine = NULL;
QList<StrategieOverview *> hasActiveTimerList;
QTimer *backgroundUpdateTimer = NULL;

StrategieOverview::StrategieOverview(PriceChart *priceChart, QWidget *parent) :
    QDockWidget(parent)
{
    privateData = new PrivateData;
    privateData->ui = new Ui::StrategieOverview;
    QWidget *quoteWidget = new QWidget;
    privateData->ui->setupUi(quoteWidget);
    QFont dockFont = font();
    dockFont.setPointSize(dockFont.pointSize() - 1);
    setFont(dockFont);

    setWidget(quoteWidget);
    setWindowTitle(priceChart->isinInfo().name + " " + dataConnection->timeScaleIntToString(priceChart->timeScale()));
    setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    setAllowedAreas(Qt::LeftDockWidgetArea);
    setFloating(false);
    priceChart->strategie->quotesDockWindow->addDockWidget(Qt::LeftDockWidgetArea, this);
    installEventFilter(this);

    privateData->curvePropertiesWidget = new AnimatedSlideWidget();
    privateData->curvePropertiesWidget->setWidget(priceChart->priceCurvePropertiesWidget());
    privateData->ui->verticalLayout->addWidget(privateData->curvePropertiesWidget);
    privateData->curvePropertiesWidget->setVisible(false);

    privateData->priceChart = priceChart;
    privateData->backgroundItem = NULL;
    privateData->ui->showPushButton->setMouseTracking(true);
    privateData->ui->showPushButton->installEventFilter(this);
    connect(privateData->ui->showPushButton, SIGNAL(clicked()), this, SLOT(switchToThisStrategie()));

    privateData->ui->settingsPushButton->setIcon(QIcon(":/images/package_settings"));
    connect(privateData->ui->settingsPushButton, SIGNAL(toggled(bool)), privateData->curvePropertiesWidget, SLOT(setVisible(bool)));
    connect(privateData->priceChart, SIGNAL(timeScaleChanged(int, int)), this, SLOT(updateTitle()));         //er moet nog een source changed komen

    privateData->ui->tableWidget->resizeRowsToContents();
    privateData->ui->tableWidget->setMaximumHeight(privateData->ui->tableWidget->horizontalHeader()->height() + privateData->ui->tableWidget->rowHeight(0));

    if (!pulseTimeLine)
    {
        pulseTimeLine = new QTimeLine(500);
        pulseTimeLine->setLoopCount(0);
        pulseTimeLine->setFrameRange(0, OUTER_BLUR_RADIUS);
        pulseTimeLine->setCurveShape(QTimeLine::CosineCurve);
        pulseTimeLine->setCurrentTime(0);
        pulseTimeLine->setUpdateInterval(50);
    }

    if (!backgroundUpdateTimer)
    {
        backgroundUpdateTimer = new QTimer;
        backgroundUpdateTimer->setInterval(100);
        backgroundUpdateTimer->setSingleShot(true);
    }

    connect(backgroundUpdateTimer, SIGNAL(timeout()), this, SLOT(updateGraphicsViewBackground()));

    QGraphicsScene *scene = new QGraphicsScene;
    scene->setItemIndexMethod(QGraphicsScene::NoIndex);
    scene->setSceneRect(0, 0, 40, 40);
    privateData->ui->graphicsView->setScene(scene);
    privateData->ui->graphicsView->viewport()->setAttribute(Qt::WA_NoSystemBackground);
    privateData->ui->graphicsView->viewport()->setAttribute(Qt::WA_OpaquePaintEvent);

    privateData->rotateMachine = new QStateMachine(this);
    privateData->upState = new QState(privateData->rotateMachine);
    privateData->downState = new QState(privateData->rotateMachine);

    privateData->blurEffect = new GraphicsBlurEffect(this);
    privateData->blurEffect->setBlurRadius(0);
    privateData->blurPulse = new QPropertyAnimation(privateData->blurEffect, "blurRadius", this);
    privateData->blurPulse->setDuration(1500);
    privateData->blurPulse->setKeyValueAt(0.0, 0.0);
    privateData->blurPulse->setKeyValueAt(0.5, OUTER_BLUR_RADIUS);
    privateData->blurPulse->setKeyValueAt(1.0, 0.0);

    GraphicsPixmapItem *contrastItem = new GraphicsPixmapItem(this);
    contrastItem->setGraphicsEffect(privateData->blurEffect);
    contrastItem->setCacheMode(QGraphicsItem::ItemCoordinateCache);
    QPixmap imagePixmap = QPixmap::fromImage(QImage(":/images/go-up.svg").scaled(QSize(25, 25), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    QPixmap contrastPixmap = imagePixmap;
    contrastPixmap.fill(Qt::white);
    contrastPixmap.setAlphaChannel(imagePixmap.alphaChannel());

    contrastItem->setPixmap(contrastPixmap);
    contrastItem->setTransformOriginPoint(contrastItem->boundingRect().center());
    contrastItem->moveBy(20.0 / 2.0, 20.0 / 2.0);
    scene->addItem(contrastItem);

    privateData->rotateMachine->addDefaultAnimation(new QPropertyAnimation(contrastItem, "rotation", this));
    privateData->upState->assignProperty(contrastItem, "rotation", 0);
    privateData->downState->assignProperty(contrastItem, "rotation", 180);

    QGraphicsColorizeEffect *colorizeDownEffect = new QGraphicsColorizeEffect(this);
    colorizeDownEffect->setColor(Qt::red);
    privateData->colorizeDirectionIndicator = new QPropertyAnimation(colorizeDownEffect, "strength", this);
    privateData->rotateMachine->addDefaultAnimation(privateData->colorizeDirectionIndicator);
    privateData->upState->assignProperty(colorizeDownEffect, "strength", 0.0);
    privateData->downState->assignProperty(colorizeDownEffect, "strength", 1.0);

    GraphicsPixmapItem *directionItem = new GraphicsPixmapItem(this);
    directionItem->setGraphicsEffect(colorizeDownEffect);
    directionItem->setCacheMode(QGraphicsItem::ItemCoordinateCache);
    directionItem->setPixmap(QPixmap::fromImage(QImage(":/images/go-up.svg").scaled(QSize(25, 25), Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
    directionItem->setTransformOriginPoint(directionItem->boundingRect().center());
    directionItem->moveBy(20.0 / 2.0, 20.0 / 2.0);
    scene->addItem(directionItem);

    privateData->rotateMachine->addDefaultAnimation(new QPropertyAnimation(directionItem, "rotation", this));
    privateData->upState->assignProperty(directionItem, "rotation", 0);
    privateData->downState->assignProperty(directionItem, "rotation", 180);

    privateData->upState->addTransition(this, SIGNAL(iconDownRotationRequested()), privateData->downState);
    privateData->downState->addTransition(this, SIGNAL(iconUpRotationRequested()), privateData->upState);
    privateData->rotateMachine->setInitialState(privateData->upState);
    QList<QAbstractAnimation *> defaultAnimations = privateData->rotateMachine->defaultAnimations();
    for (int i = 0; i < defaultAnimations.size(); i++)
    {
        QPropertyAnimation *animation = static_cast<QPropertyAnimation *>(defaultAnimations.at(i));
        animation->setDuration(1000);
    }

    privateData->rotateMachine->start();

    connect(privateData->upState, SIGNAL(entered()), this, SLOT(pulseIcon()));
    connect(privateData->downState, SIGNAL(entered()), this, SLOT(pulseIcon()));
    connect(mainWindow->ui->quotesDockWidget, SIGNAL(topLevelChanged(bool)), this, SLOT(updatePalette()));

    updatePalette();
}

StrategieOverview::~StrategieOverview()
{
    resetAlarm();

    privateData->curvePropertiesWidget->setWidget(NULL);
    delete privateData->curvePropertiesWidget;
    delete privateData->ui;
    delete privateData;
}

PriceChart *StrategieOverview::priceChart() const
{
    return privateData->priceChart;
}

void StrategieOverview::updateGraphicsViewBackground(void)
{
    QPoint offset =  privateData->ui->graphicsView->mapToGlobal(QPoint(0, 0)) - mainWindow->ui->quotesDockWidget->floatWidget()->mapToGlobal(QPoint(0, 0));
    QRect sourceRect = QRect(offset, privateData->ui->graphicsView->size());
    const QImage &floatDockBackgroundImage = mainWindow->ui->quotesDockWidget->floatWidget()->backgroundImage();
    privateData->ui->graphicsView->setBackgroundBrush(QBrush(floatDockBackgroundImage.copy(sourceRect)));
}

bool StrategieOverview::eventFilter(QObject *object, QEvent *event)
{
    if (object == privateData->ui->showPushButton && event->type() == QEvent::ToolTip)
    {
        setupToolTip();
    }

    if (object == this && event->type() == QEvent::Paint)
    {
        QPaintEvent *paintEvent = static_cast<QPaintEvent *>(event);
        QRect rect = privateData->ui->graphicsView->viewport()->rect();
        rect.moveTo(privateData->ui->graphicsView->viewport()->mapTo(this, QPoint(0, 0)));
        if (paintEvent->rect().intersects(rect) && !testAttribute(Qt::WA_UpdatesDisabled))
        {
            QTime reference = privateData->backGroundRenderTimeStamp;
            if (reference.addMSecs(17) <= mainWindow->ui->quotesDockWidget->floatWidget()->backgroundRenderTimeStamp())
            {
                backgroundUpdateTimer->stop();
                updateGraphicsViewBackground();
                privateData->backGroundRenderTimeStamp = mainWindow->ui->quotesDockWidget->floatWidget()->backgroundRenderTimeStamp();
            }
            else
            {
                backgroundUpdateTimer->stop();
                backgroundUpdateTimer->start();
            }
        }
    }

    return QDockWidget::eventFilter(object, event);
}

void StrategieOverview::closeEvent(QCloseEvent *event)
{
    (void)event;

    deleteLater();
}

void StrategieOverview::switchToThisStrategie(void)
{
    strategie->switchToPriceChart(privateData->priceChart, true);
    QPixmapCache::clear();
}

void StrategieOverview::setupToolTip(void)
{
    privateData->priceChart->indexPlot->enableAxis(QwtPlot::xBottom, false);
    privateData->priceChart->indexPlot->enableAxis(QwtPlot::yLeft, false);
    privateData->priceChart->indexPlot->legend()->contentsWidget()->hide();

    privateData->priceChart->indexPlot->setMaximumSize(250, 175);
    QPixmap screenShot = QPixmap::grabWidget(privateData->priceChart->indexPlot);
    privateData->priceChart->indexPlot->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    QBitmap mask = screenShot.createMaskFromColor(palette().window().color(), Qt::MaskInColor);
    screenShot.setMask(mask);

    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    screenShot.save(&buffer, "PNG");
    QString text = QString("<html><h1>" + windowTitle() + "</h1> <p style=\"margin-left: 4em;\"> </p> <img src=\"data:image/png;base64,") + byteArray.toBase64() + "\"/></html>";
    privateData->ui->showPushButton->setToolTip(text);

    privateData->priceChart->indexPlot->enableAxis(QwtPlot::xBottom, true);
    privateData->priceChart->indexPlot->enableAxis(QwtPlot::yLeft, true);
    privateData->priceChart->indexPlot->legend()->contentsWidget()->show();
}

void StrategieOverview::updateTitle(void)
{
    privateData->referencePrice = PriceEntry();
    setWindowTitle(privateData->priceChart->isinInfo().name + " " + dataConnection->timeScaleIntToString(privateData->priceChart->timeScale()));
}

void StrategieOverview::doUpdatePriceQuote(QList<PriceEntry> &priceEntryList)
{
    if (!priceEntryList.isEmpty())
        privateData->referencePrice = priceEntryList.at(0);

    if (!privateData->referencePrice.dateTime.isValid())
    {
        qWarning("StrategieOverview::updatePriceQuote(): failed to retreive period open price from database");
        return;
    }

    setUpdatesEnabled(false);
    updatePalette();
    if (privateData->currentPrice.closeAverage < privateData->referencePrice.openAverage)
        rotateIconDown();
    else if (privateData->currentPrice.closeAverage > privateData->referencePrice.openAverage || qFuzzyCompare(privateData->currentPrice.closeAverage, privateData->referencePrice.openAverage))
        rotateIconUp();

    privateData->ui->tableWidget->item(0, 0)->setText(QString::number(privateData->referencePrice.openAverage, 'g'));
    privateData->ui->tableWidget->item(0, 1)->setText(QString::number(privateData->currentPrice.closeAverage, 'g'));
    if (privateData->priceChart->isinInfo().market == "FOREX")
    {
        if (privateData->priceChart->isinInfo().symbol.contains("JPY", Qt::CaseInsensitive) && !privateData->priceChart->isinInfo().symbol.startsWith("JPY", Qt::CaseInsensitive))
            privateData->ui->tableWidget->item(0, 2)->setText(QString::number((privateData->currentPrice.closeAverage - privateData->referencePrice.openAverage) * 100.0f, 'g'));
        else
            privateData->ui->tableWidget->item(0, 2)->setText(QString::number((privateData->currentPrice.closeAverage - privateData->referencePrice.openAverage) * 10000.0f, 'g'));
    }
    else
        privateData->ui->tableWidget->item(0, 2)->setText(QString::number(privateData->currentPrice.closeAverage - privateData->referencePrice.openAverage, 'g'));

    privateData->ui->tableWidget->item(0, 3)->setText(QString::number(((privateData->currentPrice.closeAverage / privateData->referencePrice.openAverage) * 100.0f) - 100.0f, 'g'));
    privateData->ui->tableWidget->resizeRowsToContents();
    privateData->ui->tableWidget->setFixedHeight(privateData->ui->tableWidget->horizontalHeader()->height() + privateData->ui->tableWidget->rowHeight(0));

    setMinimumWidth(sizeHint().width());
    setMaximumWidth(sizeHint().width());

    setUpdatesEnabled(true);
}

void StrategieOverview::updatePriceQuote(PriceEntry &priceEntry)
{
    privateData->currentPrice = priceEntry;

    bool newDay = false;
    if (priceEntry.dateTime.date().day() != privateData->referencePrice.dateTime.date().day())
    {
        newDay = true;
        privateData->referencePrice = PriceEntry();
    }

    if (newDay || !privateData->referencePrice.dateTime.isValid())
    {
        TimeScale timeScale = DayByDay;
        switch (privateData->priceChart->timeScale())
        {
            case TickByTick:
            case MinuteByMinute:
            case HourByHour:
            case DayByDay:
                timeScale = DayByDay;
                break;

            case WeekByWeek:
                timeScale = WeekByWeek;
                break;

            case MonthByMonth:
                timeScale = MonthByMonth;
                break;
        }

        dataConnection->getPriceEntries(this, SLOT(doUpdatePriceQuote(QList<PriceEntry> &)), privateData->priceChart->isinInfo(), QDateTime(), QDateTime(), timeScale, 1);
    }
    else
    {
        QList<PriceEntry> emptyList;
        doUpdatePriceQuote(emptyList);
    }
}

void StrategieOverview::updatePalette(void)
{
    int alpha = 255;
    QColor positiveTextColor = Qt::black;
    if (mainWindow->ui->quotesDockWidget->isFloating())
    {
        alpha = 80;
        positiveTextColor = Qt::white;
    }

    QColor red = Qt::red;
    red.setAlpha(alpha);
    QColor green = Qt::green;
    green.setAlpha(alpha);
    QPalette newPalette = privateData->ui->tableWidget->palette();
    if (privateData->currentPrice.closeAverage < privateData->referencePrice.openAverage)
    {
        newPalette.setColor(QPalette::Base, red);
        newPalette.setColor(QPalette::Text, Qt::white);
    }
    else if (privateData->currentPrice.closeAverage > privateData->referencePrice.openAverage || qFuzzyCompare(privateData->currentPrice.closeAverage, privateData->referencePrice.openAverage))
    {
        newPalette.setColor(QPalette::Base, green);
        newPalette.setColor(QPalette::Text, positiveTextColor);
    }

    privateData->ui->tableWidget->setPalette(newPalette);
}

void StrategieOverview::rotateIconUp(void)
{
    QSet<QAbstractState *> set = privateData->rotateMachine->configuration();
    if (set.contains(privateData->upState))
        pulseIcon();
    else
        emit iconUpRotationRequested();
}

void StrategieOverview::rotateIconDown(void)
{
    return;
    QSet<QAbstractState *> set = privateData->rotateMachine->configuration();
    if (set.contains(privateData->downState))
        pulseIcon();
    else
        emit iconDownRotationRequested();
}

void StrategieOverview::pulseIcon(void)
{
    if (hasActiveTimerList.contains(this))
        return;

    privateData->blurPulse->start();
}

void StrategieOverview::triggerAlarm(void)
{
    if (!hasActiveTimerList.contains(this))
        hasActiveTimerList << this;

    if (privateData->blurPulse->state() == QPropertyAnimation::Running)
    {
        QTimer::singleShot(100, this, SLOT(triggerAlarm()));
        return;
    }

    connect(pulseTimeLine, SIGNAL(frameChanged(int)), privateData->blurEffect, SLOT(setBlurRadius(int)), Qt::UniqueConnection);

    if (pulseTimeLine->state() != QTimeLine::Running)
    {
        pulseTimeLine->setCurrentTime(0);
        pulseTimeLine->start();
    }
}

void StrategieOverview::resetAlarm(void)
{
    hasActiveTimerList.removeAll(this);

    if (hasActiveTimerList.isEmpty())
        pulseTimeLine->stop();

    disconnect(pulseTimeLine, SIGNAL(frameChanged(int)), privateData->blurEffect, SLOT(setBlurRadius(int)));
    privateData->blurEffect->setBlurRadius(0);
}
