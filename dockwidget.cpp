#include "main.h"
#include "blur.h"
#include "dockwidget.h"
#include <QGLWidget>
#include <iostream>
#include <cmath>
#include <time.h>
#include<unistd.h> //sleep

#define SLIDE_EFFECT_TIME 400
#define FADE_EFFECT_TIME 2000
#define TAB_SIZE 25
#define BLUR_RADIUS 18//18 //12
#define GRAB_OFFSET ( ( ( ( ( ( 2 * BLUR_RADIUS ) - 1 ) / 2 ) + 1 ) * 2 ) + 6 )
#define CORNER_RADIUS 3
#define HAVE_TBB 1
#define REFRESH_RATE 16

class TransparentDock::PrivateData
{
    public:
        QDockWidget *friendDock;
        QWidget *dockWidget;
        QWidget *labelWidget;
        OrientationButton *labelButton;
        OrientationButton *floatButton;
        OrientationButton *autoHideButton;
        bool autoHide;
        QTimer hideTimer;
        QTimer showTimer;
        Qt::DockWidgetArea dockWidgetArea;
        QPointer<QWidget> alignWidget;
        QStateMachine slideMachine;
        QState slideInState;
        QPoint slideInPoint;
        QState slideOutState;
        QPoint slideOutPoint;
        QPoint upAnimationStartPoint;
        QPropertyAnimation *slideAnimation;
        QStateMachine fadeMachine;
        QState fadeInState;
        QState fadeOutState;
        QPropertyAnimation *fadeAnimation;
        qreal widgetOpacity;
        int windowToDockSpacing;
        int topSpacing;
        int rightSpacing;
        int bottomSpacing;
        int leftSpacing;
        QImage backgroundImage;
#ifdef DOCK_USE_PIXMAPS
        QPixmap fullBackgroundPixmap;
#else
        QImage fullBackgroundImage;
#endif
        bool backgroundRefreshingEnabled;
        bool opaqueFlag;
        QLinearGradient outLineGradients[6];
        QPainterPath outLinePaths[7];
        QRegion defaultMask;
        QTimer updateBackgroundTimer;
        QList<QRegion> pendingRepaintRegionsList;
        bool processPendingRepaintRegions;
        double renderTimeRequiered;
        double blurTimeRequiered;
        QTime backgroundRenderTimeStamp;
};

static bool globalDisableBackgroundUpdating = false;
TransparentDock::TransparentDock(QDockWidget *dock) :
    QWidget(dock->parentWidget())
{
    privateData = new PrivateData;
    privateData->friendDock = dock;
    privateData->alignWidget = dock->parentWidget();
    privateData->dockWidget = NULL;
    privateData->windowToDockSpacing = 0;
    privateData->topSpacing = 0;
    privateData->rightSpacing = 0;
    privateData->bottomSpacing = 0;
    privateData->leftSpacing = 0;
    privateData->autoHide = false;
    privateData->backgroundRefreshingEnabled = true;
    privateData->processPendingRepaintRegions = false;
    privateData->updateBackgroundTimer.setInterval(REFRESH_RATE);
    privateData->updateBackgroundTimer.setSingleShot(true);
    connect(&privateData->updateBackgroundTimer, SIGNAL(timeout()), this, SLOT(renderBackGround()));

    privateData->slideInState.addTransition(this, SIGNAL(slideOut()), &privateData->slideOutState);
    privateData->slideMachine.addState(&privateData->slideInState);

    privateData->slideOutState.addTransition(this, SIGNAL(slideIn()), &privateData->slideInState);
    privateData->slideMachine.addState(&privateData->slideOutState);

    privateData->slideAnimation = new QPropertyAnimation(this, "pos");
    privateData->slideAnimation->setDuration(0);
    privateData->slideAnimation->setEasingCurve(QEasingCurve::OutBack);
    privateData->slideMachine.addDefaultAnimation(privateData->slideAnimation);
    privateData->slideMachine.setInitialState(&privateData->slideOutState);
    privateData->slideMachine.setAnimated(false);
    privateData->slideMachine.start();
    connect(privateData->slideAnimation, SIGNAL(finished()), this, SLOT(animationFinished()));

    setWidgetOpacity(0.7);
    privateData->fadeInState.addTransition(this, SIGNAL(fadeOut()), &privateData->fadeOutState);
    privateData->fadeInState.assignProperty(this, "widgetOpacity", 0.7);
    privateData->fadeMachine.addState(&privateData->fadeInState);

    privateData->fadeOutState.addTransition(this, SIGNAL(fadeIn()), &privateData->fadeInState);
    privateData->fadeOutState.assignProperty(this, "widgetOpacity", 0.3);
    privateData->fadeMachine.addState(&privateData->fadeOutState);

    privateData->fadeAnimation = new QPropertyAnimation(this, "widgetOpacity");
    privateData->fadeAnimation->setDuration(FADE_EFFECT_TIME);
    privateData->fadeMachine.addDefaultAnimation(privateData->fadeAnimation);
    privateData->fadeMachine.setInitialState(&privateData->fadeOutState);
    privateData->fadeMachine.setAnimated(true);
    privateData->fadeMachine.start();

    privateData->dockWidget = new QWidget(this);
    privateData->dockWidget->setLayout(new QVBoxLayout);

    privateData->labelWidget = new QWidget(this);
    privateData->labelWidget->setLayout(new QHBoxLayout);

    privateData->labelButton = new OrientationButton(windowTitle());
    privateData->labelButton->setToolTip(tr("Toon/verberg."));
    privateData->labelButton->setFlat(true);
    privateData->labelButton->setFont(QFont(privateData->labelButton->font().family(), privateData->labelButton->font().pointSize(), QFont::Bold, privateData->labelButton->font().italic()));
    privateData->labelWidget->layout()->addWidget(privateData->labelButton);
    connect(privateData->labelButton, SIGNAL(clicked()), this, SLOT(toggleVisibility()));

    privateData->floatButton = new OrientationButton;
    privateData->floatButton->setToolTip(tr("Zwevend/ingebed."));
    privateData->floatButton->setCheckable(true);
    privateData->floatButton->setFlat(true);
    privateData->floatButton->setIcon(qApp->style()->standardIcon(QStyle::SP_TitleBarNormalButton));
    privateData->floatButton->setChecked(true);
    privateData->labelWidget->layout()->addWidget(privateData->floatButton);
    connect(privateData->floatButton, SIGNAL(clicked(bool)), this, SLOT(setFloating(bool)));

    privateData->autoHideButton = new OrientationButton;
    privateData->autoHideButton->setToolTip(tr("Automatisch verbergen aan/uit."));
    privateData->autoHideButton->setCheckable(true);
    privateData->autoHideButton->setFlat(true);
    privateData->autoHideButton->setIcon(qApp->style()->standardIcon(QStyle::SP_TitleBarShadeButton));
    privateData->autoHideButton->setChecked(privateData->autoHide);
    privateData->labelWidget->layout()->addWidget(privateData->autoHideButton);
    connect(privateData->autoHideButton, SIGNAL(clicked(bool)), this, SLOT(setAutoHide(bool)));

    privateData->showTimer.setInterval(300);
    privateData->showTimer.setSingleShot(true);
    connect(&privateData->showTimer, SIGNAL(timeout(void)), this, SLOT(toggleVisibility(void)));

    privateData->hideTimer.setInterval(1000);
    privateData->hideTimer.setSingleShot(true);
    connect(&privateData->hideTimer, SIGNAL(timeout(void)), this, SLOT(toggleVisibility(void)));
    if (autoHide() && isVisible() && isSlidIn())
        privateData->hideTimer.start();

    setDockLocation(Qt::BottomDockWidgetArea);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    //    privateData->opaqueFlag = true;
    //    setAttribute(Qt::WA_OpaquePaintEvent, privateData->opaqueFlag);

    installEventFilter(this);

    if (dock->parentWidget())
        dock->parentWidget()->installEventFilter(this);

    privateData->labelWidget->installEventFilter(this);

    updatePalette();

    slideInOut(true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
}

TransparentDock::~TransparentDock(void)
{
    delete privateData;
}

void TransparentDock::updatePalette(void)
{
    QPalette palette;
    QColor color = QApplication::palette().color(QPalette::Window);
    int alpha = 127;
    color.setAlpha(static_cast<int>(alpha));
    palette.setColor(QPalette::Window, color);
    alpha = 50;
    color = palette.color(QPalette::Button);
    color.setAlpha(alpha);
    palette.setColor(QPalette::Button, color);
    color = palette.color(QPalette::Base);
    color.setAlpha(20);
    palette.setColor(QPalette::Base, color);
    color = palette.color(QPalette::AlternateBase);
    color.setAlpha(20);
    palette.setColor(QPalette::AlternateBase, color);

    setPalette(palette);
}

void TransparentDock::testSlot(void)
{
}

void TransparentDock::paintEvent(QPaintEvent *paintEvent)
{
    QPainter widgetPainter(this);
    widgetPainter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform, false);
    widgetPainter.setCompositionMode(QPainter::CompositionMode_Source);

    if (privateData->backgroundRefreshingEnabled)
    {
        if (!privateData->processPendingRepaintRegions && !globalDisableBackgroundUpdating)
        {
            privateData->pendingRepaintRegionsList.append(paintEvent->region());
            int interval = static_cast<int>(privateData->renderTimeRequiered * 1.2);
            if (interval < 16)
                renderBackGround(false);
            else
            {
                if (!privateData->updateBackgroundTimer.isActive())
                {
                    privateData->updateBackgroundTimer.setInterval(interval);
                    privateData->updateBackgroundTimer.start();
                }
            }
        }

#ifdef DOCK_USE_PIXMAPS
        widgetPainter.drawPixmap(paintEvent->rect(), privateData->fullBackgroundPixmap, paintEvent->rect());
#else
        widgetPainter.drawImage(paintEvent->rect(), privateData->fullBackgroundImage, paintEvent->rect(), Qt::ThresholdDither | Qt::NoOpaqueDetection | Qt::AvoidDither);
#endif
    }
    else
    {
        int yOffset = (pos().y() - privateData->slideInPoint.y()) + (paintEvent->rect().y() - rect().y());
        int xOffset = (pos().x() - privateData->slideInPoint.x()) + (paintEvent->rect().x() - rect().x());
        widgetPainter.drawImage(paintEvent->rect().topLeft().x(), paintEvent->rect().topLeft().y(), privateData->backgroundImage, GRAB_OFFSET + xOffset, GRAB_OFFSET + yOffset, paintEvent->rect().width(), paintEvent->rect().height(), Qt::ThresholdDither | Qt::NoOpaqueDetection | Qt::AvoidDither);
    }

    widgetPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    QPen pen;
    pen.setWidth(4);

    if (dockLocation() == Qt::TopDockWidgetArea)
    {
        if (privateData->labelWidget->geometry().left() != 0)
        {
            if (privateData->outLinePaths[0].intersects(paintEvent->rect()))
            {
                pen.setBrush(privateData->outLineGradients[1]);
                widgetPainter.strokePath(privateData->outLinePaths[0], pen);
            }
        }

        if (privateData->outLinePaths[1].intersects(paintEvent->rect()))
        {
            pen.setBrush(privateData->outLineGradients[2]);
            widgetPainter.strokePath(privateData->outLinePaths[1], pen);
        }

        if (privateData->outLinePaths[2].intersects(paintEvent->rect()))
        {
            pen.setBrush(privateData->outLineGradients[1]);
            widgetPainter.strokePath(privateData->outLinePaths[2], pen);
        }

        if (privateData->outLinePaths[3].intersects(paintEvent->rect()))
        {
            pen.setBrush(privateData->outLineGradients[0]);
            widgetPainter.strokePath(privateData->outLinePaths[3], pen);
        }

        if (privateData->outLinePaths[4].intersects(paintEvent->rect()))
        {
            pen.setBrush(privateData->outLineGradients[3]);
            widgetPainter.strokePath(privateData->outLinePaths[4], pen);
        }

        if (privateData->outLinePaths[5].intersects(paintEvent->rect()))
        {
            pen.setBrush(privateData->outLineGradients[3]);
            widgetPainter.strokePath(privateData->outLinePaths[5], pen);
        }
    }
    else if (dockLocation() == Qt::BottomDockWidgetArea)
    {
        if (privateData->labelWidget->geometry().left() != 0)
        {
            if (privateData->outLinePaths[0].intersects(paintEvent->rect()))
            {
                pen.setBrush(privateData->outLineGradients[1]);
                widgetPainter.strokePath(privateData->outLinePaths[0], pen);
            }
        }

        if (privateData->outLinePaths[1].intersects(paintEvent->rect()))
        {
            pen.setBrush(privateData->outLineGradients[2]);
            widgetPainter.strokePath(privateData->outLinePaths[1], pen);
        }

        if (privateData->outLinePaths[2].intersects(paintEvent->rect()))
        {
            pen.setBrush(privateData->outLineGradients[1]);
            widgetPainter.strokePath(privateData->outLinePaths[2], pen);
        }

        if (privateData->outLinePaths[3].intersects(paintEvent->rect()))
        {
            pen.setBrush(privateData->outLineGradients[0]);
            widgetPainter.strokePath(privateData->outLinePaths[3], pen);
        }

        if (privateData->outLinePaths[4].intersects(paintEvent->rect()))
        {
            pen.setBrush(privateData->outLineGradients[3]);
            widgetPainter.strokePath(privateData->outLinePaths[4], pen);
        }

        if (privateData->outLinePaths[5].intersects(paintEvent->rect()))
        {
            pen.setBrush(privateData->outLineGradients[3]);
            widgetPainter.strokePath(privateData->outLinePaths[5], pen);
        }
    }
    else if (dockLocation() == Qt::LeftDockWidgetArea)
    {
        if (privateData->labelWidget->geometry().top() != 0)
        {
            if (privateData->outLinePaths[0].intersects(paintEvent->rect()))
            {
                pen.setBrush(privateData->outLineGradients[4]);
                widgetPainter.strokePath(privateData->outLinePaths[0], pen);
            }
        }

        if (privateData->outLinePaths[1].intersects(paintEvent->rect()))
        {
            pen.setBrush(privateData->outLineGradients[5]);
            widgetPainter.strokePath(privateData->outLinePaths[1], pen);
        }

        if (privateData->outLinePaths[2].intersects(paintEvent->rect()))
        {
            pen.setBrush(privateData->outLineGradients[4]);
            widgetPainter.strokePath(privateData->outLinePaths[2], pen);
        }

        if (privateData->outLinePaths[3].intersects(paintEvent->rect()))
        {
            pen.setBrush(privateData->outLineGradients[3]);
            widgetPainter.strokePath(privateData->outLinePaths[3], pen);
        }

        if (privateData->outLinePaths[4].intersects(paintEvent->rect()))
        {
            pen.setBrush(privateData->outLineGradients[0]);
            widgetPainter.strokePath(privateData->outLinePaths[4], pen);
        }
        if (privateData->outLinePaths[5].intersects(paintEvent->rect()))
        {
            pen.setBrush(privateData->outLineGradients[0]);
            widgetPainter.strokePath(privateData->outLinePaths[5], pen);
        }
    }
    else if (dockLocation() == Qt::RightDockWidgetArea)
    {
        if (privateData->labelWidget->geometry().top() != 0)
        {
            if (privateData->outLinePaths[0].intersects(paintEvent->rect()))
            {
                pen.setBrush(privateData->outLineGradients[4]);
                widgetPainter.strokePath(privateData->outLinePaths[0], pen);
            }
        }

        if (privateData->outLinePaths[1].intersects(paintEvent->rect()))
        {
            pen.setBrush(privateData->outLineGradients[5]);
            widgetPainter.strokePath(privateData->outLinePaths[1], pen);
        }

        if (privateData->outLinePaths[2].intersects(paintEvent->rect()))
        {
            pen.setBrush(privateData->outLineGradients[4]);
            widgetPainter.strokePath(privateData->outLinePaths[2], pen);
        }

        if (privateData->outLinePaths[3].intersects(paintEvent->rect()))
        {
            pen.setBrush(privateData->outLineGradients[3]);
            widgetPainter.strokePath(privateData->outLinePaths[3], pen);
        }

        if (privateData->outLinePaths[4].intersects(paintEvent->rect()))
        {
            pen.setBrush(privateData->outLineGradients[0]);
            widgetPainter.strokePath(privateData->outLinePaths[4], pen);
        }

        if (privateData->outLinePaths[5].intersects(paintEvent->rect()))
        {
            pen.setBrush(privateData->outLineGradients[0]);
            widgetPainter.strokePath(privateData->outLinePaths[5], pen);
        }
    }

    QColor windowColor = palette().color(QPalette::Window);
    pen.setColor(windowColor);
    pen.setWidth(2);
    widgetPainter.strokePath(privateData->outLinePaths[6], pen);
}

void TransparentDock::enterEvent(QEvent *enterEvent)
{
    (void) enterEvent;

    raise();

    emit fadeIn();

    if (!privateData->autoHide || !isVisible() || privateData->slideMachine.isAnimated() || !isEnabled())
        return;

    if (isSlidIn())
    {
        privateData->hideTimer.stop();
        return;
    }

    privateData->hideTimer.stop();
    privateData->showTimer.start();
}

void TransparentDock::leaveEvent(QEvent *leaveEvent)
{
    (void) leaveEvent;

    if (mask().contains(mapFromGlobal(QCursor::pos())))
        return;

    emit fadeOut();

    if (!privateData->autoHide || !isVisible() || privateData->slideMachine.isAnimated() || !isEnabled())
        return;

    if (!isSlidIn())
    {
        privateData->showTimer.stop();
        return;
    }

    privateData->showTimer.stop();
    privateData->hideTimer.start();
}

static QTime lastPaintTime = QTime::currentTime();

void TransparentDock::resizeEvent(QResizeEvent *resizeEvent)
{
    (void) resizeEvent;

    alignToParent();
}

void TransparentDock::moveEvent(QMoveEvent *moveEvent)
{
    if (!privateData->slideMachine.isAnimated())
        return;

    QRect maskRect = rect();
    bool moveTopLeft = false;
    Qt::DockWidgetArea area = dockLocation();
    if (area == Qt::LeftDockWidgetArea && moveEvent->pos().x() < privateData->slideInPoint.x())
        moveTopLeft = true;
    else if (area == Qt::RightDockWidgetArea && moveEvent->pos().x() > privateData->slideInPoint.x())
        moveTopLeft = true;
    else if (area == Qt::BottomDockWidgetArea && moveEvent->pos().y() > privateData->slideInPoint.y())
        moveTopLeft = true;
    else if (area == Qt::TopDockWidgetArea && moveEvent->pos().y() < privateData->slideInPoint.y())
        moveTopLeft = true;

    if (moveTopLeft)
        maskRect.moveTopLeft(privateData->slideInPoint - moveEvent->pos());

    QRegion maskRegion = QRegion(maskRect).intersected(privateData->defaultMask);
    QSize regionSize = maskRegion.boundingRect().size();
    if (regionSize.height() < 1 || regionSize.width() < 1)
        maskRegion = QRegion(0, 0, 1, 1);

    setMask(maskRegion);
    repaint();
}

bool TransparentDock::eventFilter(QObject *object, QEvent *event)
{
    if (object == alignWidget() && event->type() == QEvent::Resize)
    {
        bool retval = QObject::eventFilter(object, event);
        alignToParent();
        return retval;
    }

    if (object == privateData->labelWidget && (event->type() == QEvent::Resize || event->type() == QEvent::Move) /*&& isVisible() && updatesEnabled()*/)
    {
        bool retval = QObject::eventFilter(object, event);
        updateOutliningGeometry();
        return retval;
    }

    return QObject::eventFilter(object, event);
}

QWidget *TransparentDock::widget(void) const
{
    QLayoutItem *layoutItem = privateData->dockWidget->layout()->itemAt(0);
    if (!layoutItem)
        return NULL;

    return layoutItem->widget();
}

void TransparentDock::setWidget(QWidget *widget)
{
    QLayoutItem *child;
    while ((child = privateData->dockWidget->layout()->takeAt(0)))
    {
        if (child->widget())
            child->widget()->setVisible(false);
    }

    if (widget)
    {
        privateData->dockWidget->layout()->addWidget(widget);
        widget->setVisible(true);
    }
}

void TransparentDock::alignToParent(void)
{
    if (!alignWidget())
        return;

    if (!parentWidget())
        return;

    if (privateData->dockWidgetArea == Qt::LeftDockWidgetArea)
    {
        QSize size = QSize(0, alignWidget()->height() - privateData->topSpacing - privateData->bottomSpacing);
        if (size.isValid())
            setMinimumSize(size);

        size = QSize(QWIDGETSIZE_MAX, alignWidget()->height() - privateData->topSpacing - privateData->bottomSpacing);
        if (size.isValid())
            setMaximumSize(size);

        privateData->slideInPoint = parentWidget()->mapFrom(window(), alignWidget()->mapTo(window(), QPoint(privateData->leftSpacing, alignWidget()->pos().y() + privateData->topSpacing)));
        privateData->slideOutPoint = parentWidget()->mapFrom(window(), alignWidget()->mapTo(window(), QPoint(-width() + TAB_SIZE + privateData->leftSpacing, alignWidget()->pos().y() + privateData->topSpacing)));
    }
    else if (privateData->dockWidgetArea == Qt::RightDockWidgetArea)
    {
        QSize size = QSize(minimumSize().width(), alignWidget()->height() - privateData->topSpacing - privateData->bottomSpacing);
        if (size.isValid())
            setMinimumSize(size);

        size = QSize(maximumSize().width(), alignWidget()->height() - privateData->topSpacing - privateData->bottomSpacing);
        if (size.isValid())
            setMaximumSize(size);

        privateData->slideInPoint = parentWidget()->mapFrom(window(), alignWidget()->mapTo(window(), QPoint(alignWidget()->width() - width() - privateData->rightSpacing, alignWidget()->pos().y() + privateData->topSpacing)));
        privateData->slideOutPoint = parentWidget()->mapFrom(window(), alignWidget()->mapTo(window(), QPoint(alignWidget()->width() - TAB_SIZE - privateData->rightSpacing, alignWidget()->pos().y() + privateData->topSpacing)));

    }
    else if (privateData->dockWidgetArea == Qt::TopDockWidgetArea)
    {
        privateData->slideInPoint = parentWidget()->mapFrom(window(), alignWidget()->mapTo(window(), QPoint((alignWidget()->width() /  2) - (width() / 2), privateData->topSpacing)));
        privateData->slideOutPoint = parentWidget()->mapFrom(window(), alignWidget()->mapTo(window(), QPoint((alignWidget()->width() /  2) - (width() / 2), -height() + TAB_SIZE + privateData->topSpacing)));
    }
    else if (privateData->dockWidgetArea == Qt::BottomDockWidgetArea)
    {
        privateData->slideInPoint = parentWidget()->mapFrom(window(), alignWidget()->mapTo(window(), QPoint((alignWidget()->width() /  2) - (width() / 2), alignWidget()->height() - height() - privateData->bottomSpacing)));
        privateData->slideOutPoint = parentWidget()->mapFrom(window(), alignWidget()->mapTo(window(), QPoint((alignWidget()->width() /  2) - (width() / 2), alignWidget()->height() - TAB_SIZE - privateData->bottomSpacing)));
    }

    privateData->slideInState.assignProperty(this, "pos", privateData->slideInPoint);
    privateData->slideOutState.assignProperty(this, "pos", privateData->slideOutPoint);

    QSet<QAbstractState *> set = privateData->slideMachine.configuration();
    if (set.contains(&privateData->slideInState))
        move(privateData->slideInPoint);
    else if (set.contains(&privateData->slideOutState))
        move(privateData->slideOutPoint);
    else
        move(privateData->slideOutPoint);

    updateOutliningGeometry();
}

void TransparentDock::toggleVisibility(void)
{
    if (isSlidIn())
        slideInOut(false);
    else
        slideInOut(true);
}

void TransparentDock::setVisible(bool visible)
{
    raise();

    if (visible && !isSlidIn())
    {
        privateData->showTimer.stop();
        privateData->slideMachine.setAnimated(false);
        emit slideIn();
        updateOutliningGeometry();
        if (autoHide() && !underMouse())
            privateData->hideTimer.start();

        emit visibilityChanged(visible);
    }
    else if (!visible && isSlidIn())
    {
        privateData->hideTimer.stop();
        privateData->slideMachine.setAnimated(false);
        emit slideOut();
        updateOutliningGeometry();
        emit visibilityChanged(visible);
    }
}

bool TransparentDock::isSlidIn(void) const
{
    if (!privateData->slideMachine.isAnimated())
    {
        if (pos() == privateData->slideInPoint)
            return true;
    }
    else
    {
        QSet<QAbstractState *> set = privateData->slideMachine.configuration();
        if (set.contains(&privateData->slideInState))
            return true;
        else if (set.contains(&privateData->slideOutState))
            return false;
    }

    return false;
}

qreal TransparentDock::widgetOpacity() const
{
    return privateData->widgetOpacity;
}

void TransparentDock::setWidgetOpacity(qreal opacity)
{
    privateData->widgetOpacity = opacity;

    adjustComboColors(privateData->dockWidget);
    update();
}

const QTime &TransparentDock::backgroundRenderTimeStamp() const
{
    return privateData->backgroundRenderTimeStamp;
}

#ifdef DOCK_USE_PIXMAPS
const QPixmap &TransparentDock::backgroundPixmap() const
{
    return privateData->fullBackgroundPixmap;
}
#else
const QImage &TransparentDock::backgroundImage() const
{
    return privateData->fullBackgroundImage;
}
#endif

void TransparentDock::adjustComboColors(QObject *parent)
{
    if (!parent)
        return;

    for (int i = 0; i < parent->children().size(); i++)
    {
        QObject *object = parent->children().at(i);
        adjustComboColors(object);
        if (!object->inherits("QComboBox"))
            continue;

        QComboBox *comboBox = qobject_cast<QComboBox *>(object);
        if (!comboBox)
            continue;

        QPalette palette = comboBox->palette();
        QColor color = QApplication::palette().color(QPalette::Base);
        palette.setColor(QPalette::Base, color);
        comboBox->setPalette(palette);
    }
}

void TransparentDock::slideInOut(bool visible)
{
    raise();
    toggleBackgroundRefreshingEnabled(false);

    QApplication::processEvents(); //animatie snelheid is afhankelijk van de event queue, dus zorg ervoor dat deze leeg is om een soepele animatie te verkrijgen

    if (visible && !isSlidIn())
    {
        privateData->showTimer.stop();
        privateData->slideMachine.setAnimated(true);
        emit slideIn();
        emit visibilityChanged(visible);
    }
    else if (!visible && isSlidIn())
    {
        privateData->hideTimer.stop();
        privateData->slideMachine.setAnimated(true);
        emit slideOut();
        emit visibilityChanged(visible);
    }

    privateData->slideAnimation->setDuration(SLIDE_EFFECT_TIME);
}

void TransparentDock::animationFinished(void)
{
    privateData->slideMachine.setAnimated(false);
    toggleBackgroundRefreshingEnabled(true);

    if (isSlidIn() && autoHide() && !underMouse() && isVisible())
        privateData->hideTimer.start();
}

void TransparentDock::updateOutliningGeometry(void)
{
    privateData->outLinePaths[6] = QPainterPath();
    privateData->outLinePaths[6].setFillRule(Qt::WindingFill);

    switch ((int) dockLocation())
    {
        case Qt::TopDockWidgetArea:
            privateData->outLinePaths[6].addRoundedRect(0, 0, rect().width(), rect().height() - TAB_SIZE, CORNER_RADIUS, CORNER_RADIUS);
            privateData->outLinePaths[6].addRoundedRect(privateData->labelWidget->geometry().left(), privateData->labelWidget->geometry().top() - CORNER_RADIUS, privateData->labelWidget->width(), TAB_SIZE + (2 * CORNER_RADIUS), CORNER_RADIUS, CORNER_RADIUS);
            break;

        case Qt::BottomDockWidgetArea:
            privateData->outLinePaths[6].addRoundedRect(0, TAB_SIZE, rect().width(), rect().height() - TAB_SIZE, CORNER_RADIUS, CORNER_RADIUS);
            privateData->outLinePaths[6].addRoundedRect(privateData->labelWidget->geometry().left(), 0, privateData->labelWidget->width(), TAB_SIZE + CORNER_RADIUS, CORNER_RADIUS, CORNER_RADIUS);
            break;

        case Qt::LeftDockWidgetArea:
            privateData->outLinePaths[6].addRoundedRect(0, 0, rect().width() - TAB_SIZE, rect().height(), CORNER_RADIUS, CORNER_RADIUS);
            privateData->outLinePaths[6].addRoundedRect(rect().width() - TAB_SIZE - CORNER_RADIUS, privateData->labelWidget->geometry().top(), TAB_SIZE + CORNER_RADIUS, privateData->labelWidget->height(), CORNER_RADIUS, CORNER_RADIUS);
            break;

        case Qt::RightDockWidgetArea:
            privateData->outLinePaths[6].addRoundedRect(rect().left() + TAB_SIZE, 0, rect().width() - TAB_SIZE, rect().height(), CORNER_RADIUS, CORNER_RADIUS);
            privateData->outLinePaths[6].addRoundedRect(0, privateData->labelWidget->geometry().top(), TAB_SIZE + CORNER_RADIUS, privateData->labelWidget->height(), CORNER_RADIUS, CORNER_RADIUS);
            break;
    }

    privateData->outLinePaths[6] = privateData->outLinePaths[6].simplified().intersected(privateData->outLinePaths[6].translated(-1, -1).simplified());
    QRegion maskregion = QRegion(privateData->outLinePaths[6].toFillPolygon().toPolygon());
    privateData->defaultMask = maskregion;
    QRect widgetRect = rect();
    widgetRect.moveTopLeft(privateData->slideInPoint - pos());
    setMask(maskregion.intersected(QRegion(widgetRect)));

    privateData->outLineGradients[0] = QLinearGradient(0, 0, rect().width(), 0);
    privateData->outLineGradients[0].setColorAt(0.0, Qt::transparent);
    privateData->outLineGradients[0].setColorAt(0.5, qApp->palette().color(QPalette::Text));
    privateData->outLineGradients[0].setColorAt(1.0, Qt::transparent);

    privateData->outLineGradients[1] = QLinearGradient(0, 0, rect().width(), 0);
    privateData->outLineGradients[1].setColorAt(0.0, Qt::transparent);
    privateData->outLineGradients[1].setColorAt(0.15, qApp->palette().color(QPalette::Text));
    privateData->outLineGradients[1].setColorAt(0.5, Qt::transparent);
    privateData->outLineGradients[1].setColorAt(0.85, qApp->palette().color(QPalette::Text));
    privateData->outLineGradients[1].setColorAt(1.0, Qt::transparent);

    privateData->outLineGradients[2] = QLinearGradient();
    privateData->outLineGradients[2].setColorAt(0.0, Qt::transparent);
    privateData->outLineGradients[2].setColorAt(0.5, qApp->palette().color(QPalette::Text));
    privateData->outLineGradients[2].setColorAt(1.0, Qt::transparent);

    privateData->outLineGradients[3] = QLinearGradient(0, 0, 0, rect().height());
    privateData->outLineGradients[3].setColorAt(0.0, Qt::transparent);
    privateData->outLineGradients[3].setColorAt(0.5, qApp->palette().color(QPalette::Text));
    privateData->outLineGradients[3].setColorAt(1.0, Qt::transparent);

    privateData->outLineGradients[4] = QLinearGradient(0, 0, 0, rect().height());
    privateData->outLineGradients[4].setColorAt(0.0, Qt::transparent);
    privateData->outLineGradients[4].setColorAt(0.15, qApp->palette().color(QPalette::Text));
    privateData->outLineGradients[4].setColorAt(0.5, Qt::transparent);
    privateData->outLineGradients[4].setColorAt(0.85, qApp->palette().color(QPalette::Text));
    privateData->outLineGradients[4].setColorAt(1.0, Qt::transparent);

    privateData->outLineGradients[5] = QLinearGradient();
    privateData->outLineGradients[5].setColorAt(0.0, Qt::transparent);
    privateData->outLineGradients[5].setColorAt(0.5, qApp->palette().color(QPalette::Text));
    privateData->outLineGradients[5].setColorAt(1.0, Qt::transparent);

    if (dockLocation() == Qt::TopDockWidgetArea)
    {
        if (privateData->labelWidget->geometry().left() != 0)
        {
            privateData->outLinePaths[0] = QPainterPath();
            privateData->outLinePaths[0].moveTo(rect().left(), rect().bottom() - TAB_SIZE);
            privateData->outLinePaths[0].lineTo(privateData->labelWidget->geometry().left(), rect().bottom() - TAB_SIZE);
            privateData->outLinePaths[0].lineTo(privateData->labelWidget->geometry().left(), rect().bottom());
        }

        privateData->outLinePaths[1] = QPainterPath();
        privateData->outLinePaths[1].moveTo(privateData->labelWidget->geometry().left(), rect().bottom());
        privateData->outLinePaths[1].lineTo(privateData->labelWidget->geometry().right(), rect().bottom());
        privateData->outLineGradients[2].setStart(privateData->labelWidget->geometry().left(), 0);
        privateData->outLineGradients[2].setFinalStop(privateData->labelWidget->geometry().right(), 0);

        privateData->outLinePaths[2] = QPainterPath();
        privateData->outLinePaths[2].moveTo(privateData->labelWidget->geometry().right(), rect().bottom());
        privateData->outLinePaths[2].lineTo(privateData->labelWidget->geometry().right(), rect().bottom() - TAB_SIZE);
        privateData->outLinePaths[2].lineTo(rect().right(), rect().bottom() - TAB_SIZE);

        privateData->outLinePaths[3] = QPainterPath();
        privateData->outLinePaths[3].moveTo(rect().topLeft());
        privateData->outLinePaths[3].lineTo(rect().topRight());

        privateData->outLinePaths[4] = QPainterPath();
        privateData->outLinePaths[4].moveTo(rect().topLeft());
        privateData->outLinePaths[4].lineTo(rect().bottomLeft());

        privateData->outLinePaths[5] = QPainterPath();
        privateData->outLinePaths[5].moveTo(rect().topRight());
        privateData->outLinePaths[5].lineTo(rect().bottomRight());
    }
    else if (dockLocation() == Qt::BottomDockWidgetArea)
    {
        if (privateData->labelWidget->geometry().left() != 0)
        {
            privateData->outLinePaths[0] = QPainterPath();
            privateData->outLinePaths[0].moveTo(rect().left(), rect().top() + TAB_SIZE);
            privateData->outLinePaths[0].lineTo(privateData->labelWidget->geometry().left(), rect().top() + TAB_SIZE);
            privateData->outLinePaths[0].lineTo(privateData->labelWidget->geometry().left(), rect().top());
        }

        privateData->outLinePaths[1] = QPainterPath();
        privateData->outLinePaths[1].moveTo(privateData->labelWidget->geometry().left(), rect().top());
        privateData->outLinePaths[1].lineTo(privateData->labelWidget->geometry().right(), rect().top());
        privateData->outLineGradients[2].setStart(privateData->labelWidget->geometry().left(), 0);
        privateData->outLineGradients[2].setFinalStop(privateData->labelWidget->geometry().right(), 0);

        privateData->outLinePaths[2] = QPainterPath();
        privateData->outLinePaths[2].moveTo(privateData->labelWidget->geometry().right(), rect().top());
        privateData->outLinePaths[2].lineTo(privateData->labelWidget->geometry().right(), rect().top() + TAB_SIZE);
        privateData->outLinePaths[2].lineTo(rect().right(), rect().top() + TAB_SIZE);

        privateData->outLinePaths[3] = QPainterPath();
        privateData->outLinePaths[3].moveTo(rect().bottomLeft());
        privateData->outLinePaths[3].lineTo(rect().bottomRight());

        privateData->outLinePaths[4] = QPainterPath();
        privateData->outLinePaths[4].moveTo(rect().topLeft());
        privateData->outLinePaths[4].lineTo(rect().bottomLeft());

        privateData->outLinePaths[5] = QPainterPath();
        privateData->outLinePaths[5].moveTo(rect().topRight());
        privateData->outLinePaths[5].lineTo(rect().bottomRight());
    }
    else if (dockLocation() == Qt::LeftDockWidgetArea)
    {
        if (privateData->labelWidget->geometry().top() != 0)
        {
            privateData->outLinePaths[0] = QPainterPath();
            privateData->outLinePaths[0].moveTo(rect().right() - TAB_SIZE, rect().top());
            privateData->outLinePaths[0].lineTo(rect().right() - TAB_SIZE, privateData->labelWidget->geometry().top());
            privateData->outLinePaths[0].lineTo(rect().right(), privateData->labelWidget->geometry().top());
        }

        privateData->outLinePaths[1] = QPainterPath();
        privateData->outLinePaths[1].moveTo(rect().right(), privateData->labelWidget->geometry().top());
        privateData->outLinePaths[1].lineTo(rect().right(), privateData->labelWidget->geometry().bottom());
        privateData->outLineGradients[5].setStart(rect().right(), privateData->labelWidget->geometry().top());
        privateData->outLineGradients[5].setFinalStop(rect().right(), privateData->labelWidget->geometry().bottom());

        privateData->outLinePaths[2] = QPainterPath();
        privateData->outLinePaths[2].moveTo(rect().right(), privateData->labelWidget->geometry().bottom());
        privateData->outLinePaths[2].lineTo(rect().right() - TAB_SIZE, privateData->labelWidget->geometry().bottom());
        privateData->outLinePaths[2].lineTo(rect().right() - TAB_SIZE, rect().bottom());

        privateData->outLinePaths[3] = QPainterPath();
        privateData->outLinePaths[3].moveTo(rect().topLeft());
        privateData->outLinePaths[3].lineTo(rect().bottomLeft());

        privateData->outLinePaths[4] = QPainterPath();
        privateData->outLinePaths[4].moveTo(rect().topLeft());
        privateData->outLinePaths[4].lineTo(rect().topRight());

        privateData->outLinePaths[5] = QPainterPath();
        privateData->outLinePaths[5].moveTo(rect().bottomLeft());
        privateData->outLinePaths[5].lineTo(rect().bottomRight());
    }
    else if (dockLocation() == Qt::RightDockWidgetArea)
    {
        if (privateData->labelWidget->geometry().top() != 0)
        {
            privateData->outLinePaths[0] = QPainterPath();
            privateData->outLinePaths[0].moveTo(rect().left() + TAB_SIZE, rect().top());
            privateData->outLinePaths[0].lineTo(rect().left() + TAB_SIZE, privateData->labelWidget->geometry().top());
            privateData->outLinePaths[0].lineTo(rect().left(), privateData->labelWidget->geometry().top());
        }

        privateData->outLinePaths[1] = QPainterPath();
        privateData->outLinePaths[1].moveTo(rect().left(), privateData->labelWidget->geometry().top());
        privateData->outLinePaths[1].lineTo(rect().left(), privateData->labelWidget->geometry().bottom());
        privateData->outLineGradients[5].setStart(rect().left(), privateData->labelWidget->geometry().top());
        privateData->outLineGradients[5].setFinalStop(rect().left(), privateData->labelWidget->geometry().bottom());

        privateData->outLinePaths[2] = QPainterPath();
        privateData->outLinePaths[2].moveTo(rect().left(), privateData->labelWidget->geometry().bottom());
        privateData->outLinePaths[2].lineTo(rect().left() + TAB_SIZE, privateData->labelWidget->geometry().bottom());
        privateData->outLinePaths[2].lineTo(rect().left() + TAB_SIZE, rect().bottom());

        privateData->outLinePaths[3] = QPainterPath();
        privateData->outLinePaths[3].moveTo(rect().topRight());
        privateData->outLinePaths[3].lineTo(rect().bottomRight());

        privateData->outLinePaths[4] = QPainterPath();
        privateData->outLinePaths[4].moveTo(rect().topLeft());
        privateData->outLinePaths[4].lineTo(rect().topRight());

        privateData->outLinePaths[5] = QPainterPath();
        privateData->outLinePaths[5].moveTo(rect().bottomLeft());
        privateData->outLinePaths[5].lineTo(rect().bottomRight());
    }
}

void TransparentDock::setFloating(bool floating)
{
    privateData->friendDock->setFloating(floating);
    privateData->floatButton->setChecked(!floating);
}

void TransparentDock::prepareToRaise()
{
    mainWindow->setUpdatesEnabled(false);
    QTimer::singleShot(0, this, SLOT(doRaise()));
}

void TransparentDock::doRaise()
{
    raise();
    mainWindow->setUpdatesEnabled(true);
}

void TransparentDock::renderBackGround(bool forceRepaint)
{
    QRegion updateRegion;
    for (int i = 0; i < privateData->pendingRepaintRegionsList.size(); i++)
        updateRegion = updateRegion.united(privateData->pendingRepaintRegionsList.at(i).boundingRect());

    privateData->pendingRepaintRegionsList.clear();

    globalDisableBackgroundUpdating = true;

    QRect grabRect = updateRegion.boundingRect();
    grabRect.moveTopLeft(mapTo(window(), grabRect.topLeft()));
    grabRect = grabRect.adjusted(-GRAB_OFFSET, -GRAB_OFFSET, +GRAB_OFFSET, +GRAB_OFFSET);
    if (grabRect.isValid())
    {
        struct timespec startTime;
        clock_gettime(CLOCK_REALTIME, &startTime);

        bool updatesDisabled = testAttribute(Qt::WA_UpdatesDisabled);
        setAttribute(Qt::WA_UpdatesDisabled, true);
        bool opaquePaintEvent = testAttribute(Qt::WA_OpaquePaintEvent);
        setAttribute(Qt::WA_OpaquePaintEvent, false);

        privateData->backgroundImage = QImage(grabRect.size(), QImage::Format_RGB32);
        window()->render(&privateData->backgroundImage, QPoint(), QRegion(grabRect), QWidget::DrawChildren);

        setAttribute(Qt::WA_OpaquePaintEvent, opaquePaintEvent);
        setAttribute(Qt::WA_UpdatesDisabled, updatesDisabled);

        QColor color = palette().color(QPalette::Window);
        color.setAlpha(static_cast<int>(widgetOpacity() * 255.0));
        struct timespec blurStartTime;
        clock_gettime(CLOCK_REALTIME, &blurStartTime);

        vectorExponentialBlur(privateData->backgroundImage, BLUR_RADIUS, true, color);

        struct timespec blurEndTime;
        clock_gettime(CLOCK_REALTIME, &blurEndTime);
        privateData->blurTimeRequiered = static_cast<double>((((blurEndTime.tv_sec - blurStartTime.tv_sec) * 1000000000) + (blurEndTime.tv_nsec - blurStartTime.tv_nsec)) / 1000000.0);

#ifdef DOCK_USE_PIXMAPS
        if (privateData->fullBackgroundPixmap.size() != rect().size())
            privateData->fullBackgroundPixmap = QPixmap(rect().size());

        QPainter imagePainter(&privateData->fullBackgroundPixmap);
#else
        if (privateData->fullBackgroundImage.size() != rect().size())
            privateData->fullBackgroundImage = QImage(rect().size(), QImage::Format_RGB32);

        QPainter imagePainter(&privateData->fullBackgroundImage);
#endif

        imagePainter.setCompositionMode(QPainter::CompositionMode_Source);
        imagePainter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform, false);
        imagePainter.drawImage(updateRegion.boundingRect(), privateData->backgroundImage, privateData->backgroundImage.rect().adjusted(GRAB_OFFSET, GRAB_OFFSET, -GRAB_OFFSET, -GRAB_OFFSET), Qt::ThresholdDither | Qt::NoOpaqueDetection | Qt::AvoidDither);

        privateData->backgroundRenderTimeStamp = QTime::currentTime();

        struct timespec endTime;
        clock_gettime(CLOCK_REALTIME, &endTime);
        privateData->renderTimeRequiered = static_cast<double>((((endTime.tv_sec - startTime.tv_sec) * 1000000000) + (endTime.tv_nsec - startTime.tv_nsec)) / 1000000.0);
    }

    globalDisableBackgroundUpdating = false;

    if (forceRepaint)
    {
        privateData->processPendingRepaintRegions = true;
        repaint(updateRegion);
        privateData->processPendingRepaintRegions = false;
    }
}

void TransparentDock::toggleBackgroundRefreshingEnabled(bool on)
{
    if (privateData->backgroundRefreshingEnabled && on)
        return;
    else if (!privateData->backgroundRefreshingEnabled && !on)
        return;

    int xOffset = privateData->slideInPoint.x() - pos().x();
    int yOffset = privateData->slideInPoint.y() - pos().y();
    QRect grabRect = rect();
    grabRect.adjust(xOffset, yOffset, xOffset, yOffset);

    privateData->updateBackgroundTimer.stop();
    privateData->pendingRepaintRegionsList.clear();
    privateData->pendingRepaintRegionsList << QRegion(grabRect);
    renderBackGround(false);

    if (!on)
    {
        privateData->opaqueFlag = testAttribute(Qt::WA_OpaquePaintEvent);
        setAttribute(Qt::WA_OpaquePaintEvent, true);
        privateData->backgroundRefreshingEnabled = false;
    }
    else
    {
        setAttribute(Qt::WA_OpaquePaintEvent, privateData->opaqueFlag);
        privateData->backgroundRefreshingEnabled = true;
    }
}

Qt::DockWidgetArea TransparentDock::dockLocation(void) const
{
    return privateData->dockWidgetArea;
}

void TransparentDock::setTitle(const QString &title)
{
    setWindowTitle(title);
    privateData->labelButton->setText(title);
}

QWidget *TransparentDock::alignWidget(void) const
{
    return privateData->alignWidget;
}

void TransparentDock::setAlignWidget(QWidget *widget)
{
    if (alignWidget() == widget)
        return;

    QPoint oldPos = QPoint();
    QSize oldSize = QSize();
    if (alignWidget())
    {
        oldPos = alignWidget()->pos();
        oldSize = alignWidget()->size();
        alignWidget()->removeEventFilter(this);
    }

    widget->installEventFilter(this);
    privateData->alignWidget = widget;

    if (oldPos != alignWidget()->pos() || oldSize != alignWidget()->size())
        alignToParent();
}

bool TransparentDock::autoHide(void) const
{
    return privateData->autoHide;
}

void TransparentDock::setAutoHide(bool autoHide)
{
    privateData->autoHide = autoHide;
    if (autoHide && !privateData->autoHideButton->isChecked())
        privateData->autoHideButton->setChecked(true);
    else if (!autoHide && privateData->autoHideButton->isChecked())
        privateData->autoHideButton->setChecked(false);

    if (autoHide && !underMouse() && isVisible() && isSlidIn())
        privateData->hideTimer.start();
}

void TransparentDock::setWindowToDockSpacing(int top, int right, int bottom, int left)
{
    privateData->topSpacing = top;
    privateData->rightSpacing = right;
    privateData->bottomSpacing = bottom;
    privateData->leftSpacing = left;

    alignToParent();
}

void TransparentDock::setDockLocation(Qt::DockWidgetArea area)
{
    privateData->dockWidgetArea = area;
    if (dockLocation() == Qt::LeftDockWidgetArea)
    {
        privateData->labelButton->setOrientation(Qt::Vertical);
        privateData->labelButton->setMirrored(true);
        privateData->labelButton->setMinimumSize(TAB_SIZE, 0);
        privateData->labelButton->setMaximumSize(TAB_SIZE, QWIDGETSIZE_MAX);
        privateData->autoHideButton->setOrientation(Qt::Vertical);
        privateData->autoHideButton->setMirrored(false);

        delete privateData->labelWidget->layout();
        privateData->labelWidget->setLayout(new QVBoxLayout);
        privateData->labelWidget->layout()->setContentsMargins(0, 3, 0, 3);
        privateData->labelWidget->layout()->addWidget(privateData->labelButton);
        privateData->labelWidget->layout()->addWidget(privateData->floatButton);
        privateData->labelWidget->layout()->addWidget(privateData->autoHideButton);
        privateData->labelWidget->setMaximumWidth(TAB_SIZE);
        privateData->labelWidget->setMaximumHeight(QWIDGETSIZE_MAX);

        delete layout();

        QGridLayout *hLayout = new QGridLayout;
        hLayout->addWidget(privateData->dockWidget, 0, 0, -1, 1);
        hLayout->addWidget(privateData->labelWidget, 1, 1, 1, 1);
        hLayout->setRowStretch(0, 1);
        hLayout->setRowStretch(2, 1);
        hLayout->setSizeConstraint(QLayout::SetMinimumSize);
        setLayout(hLayout);
    }
    else if (dockLocation() == Qt::RightDockWidgetArea)
    {
        privateData->labelButton->setOrientation(Qt::Vertical);
        privateData->labelButton->setMirrored(true);
        privateData->labelButton->setMinimumSize(TAB_SIZE, 0);
        privateData->labelButton->setMaximumSize(TAB_SIZE, QWIDGETSIZE_MAX);
        privateData->autoHideButton->setOrientation(Qt::Vertical);
        privateData->autoHideButton->setMirrored(true);

        delete privateData->labelWidget->layout();
        privateData->labelWidget->setLayout(new QVBoxLayout);
        privateData->labelWidget->layout()->setContentsMargins(0, 3, 0, 3);
        privateData->labelWidget->layout()->addWidget(privateData->labelButton);
        privateData->labelWidget->layout()->addWidget(privateData->floatButton);
        privateData->labelWidget->layout()->addWidget(privateData->autoHideButton);
        privateData->labelWidget->setMaximumWidth(TAB_SIZE);
        privateData->labelWidget->setMaximumHeight(QWIDGETSIZE_MAX);

        delete layout();

        QGridLayout *hLayout = new QGridLayout;
        hLayout->addWidget(privateData->labelWidget, 1, 0, 1, 1);
        hLayout->setRowStretch(0, 1);
        hLayout->setRowStretch(2, 1);
        hLayout->addWidget(privateData->dockWidget, 0, 1, -1, 1);
        hLayout->setSizeConstraint(QLayout::SetMinimumSize);
        setLayout(hLayout);
    }
    else if (dockLocation() == Qt::TopDockWidgetArea)
    {
        privateData->labelButton->setOrientation(Qt::Horizontal);
        privateData->labelButton->setMirrored(false);
        privateData->labelButton->setMinimumSize(0, TAB_SIZE);
        privateData->labelButton->setMaximumSize(QWIDGETSIZE_MAX, TAB_SIZE);
        privateData->autoHideButton->setOrientation(Qt::Horizontal);
        privateData->autoHideButton->setMirrored(true);

        delete privateData->labelWidget->layout();
        privateData->labelWidget->setLayout(new QHBoxLayout);
        privateData->labelWidget->layout()->setContentsMargins(3, 0, 3, 0);
        privateData->labelWidget->layout()->addWidget(privateData->labelButton);
        privateData->labelWidget->layout()->addWidget(privateData->floatButton);
        privateData->labelWidget->layout()->addWidget(privateData->autoHideButton);
        privateData->labelWidget->setMaximumWidth(QWIDGETSIZE_MAX);
        privateData->labelWidget->setMaximumHeight(TAB_SIZE);

        delete layout();

        QGridLayout *vLayout = new QGridLayout;
        vLayout->addWidget(privateData->dockWidget, 0, 0, 1, -1);
        vLayout->addWidget(privateData->labelWidget, 1, 1, 1, 1);
        vLayout->setColumnStretch(0, 1);
        vLayout->setColumnStretch(2, 1);
        vLayout->setSizeConstraint(QLayout::SetMinimumSize);
        setLayout(vLayout);
    }
    else if (dockLocation() == Qt::BottomDockWidgetArea)
    {
        privateData->labelButton->setOrientation(Qt::Horizontal);
        privateData->labelButton->setMirrored(false);
        privateData->labelButton->setMinimumSize(0, TAB_SIZE);
        privateData->labelButton->setMaximumSize(QWIDGETSIZE_MAX, TAB_SIZE);
        privateData->autoHideButton->setOrientation(Qt::Horizontal);
        privateData->autoHideButton->setMirrored(false);

        delete privateData->labelWidget->layout();
        privateData->labelWidget->setLayout(new QHBoxLayout);
        privateData->labelWidget->layout()->setContentsMargins(3, 0, 3, 0);
        privateData->labelWidget->layout()->addWidget(privateData->labelButton);
        privateData->labelWidget->layout()->addWidget(privateData->floatButton);
        privateData->labelWidget->layout()->addWidget(privateData->autoHideButton);
        privateData->labelWidget->setMaximumWidth(QWIDGETSIZE_MAX);
        privateData->labelWidget->setMaximumHeight(TAB_SIZE);

        delete layout();

        QGridLayout *vLayout = new QGridLayout;
        vLayout->addWidget(privateData->labelWidget, 0, 1, 1, 1);
        vLayout->setColumnStretch(0, 1);
        vLayout->setColumnStretch(2, 1);
        vLayout->addWidget(privateData->dockWidget, 1, 0, 1, -1);
        vLayout->setSizeConstraint(QLayout::SetMinimumSize);
        setLayout(vLayout);
    }

    privateData->labelWidget->layout()->setSpacing(0);
    privateData->labelWidget->setContentsMargins(0, 0, 0, 0);
    layout()->setContentsMargins(0, 0, 0, 0);
    layout()->setSpacing(0);
    setContentsMargins(3, 3, 3, 3);

    alignToParent();
}

class DockWidget::PrivateData
{
    public:
        TransparentDock *floatWidget;
        Qt::DockWidgetArea dockWidgetArea;
        bool autoHide;
        QTimer hideTimer;
        QTimer showTimer;
};

DockWidget::DockWidget(QWidget *parent) :
    QDockWidget(parent)
{
    privateData = new PrivateData;
    privateData->autoHide = false;
    privateData->dockWidgetArea = Qt::BottomDockWidgetArea;
    privateData->floatWidget = new TransparentDock(this);

    connect(privateData->floatWidget, SIGNAL(visibilityChanged(bool)), this, SIGNAL(dockVisibilityChanged(bool)));
    connect(this, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)), this, SLOT(setDockLocation(Qt::DockWidgetArea)));
    connect(this, SIGNAL(topLevelChanged(bool)), this, SLOT(handleTopLevelChangedSignal(bool)));

    handleTopLevelChangedSignal(false);
}

DockWidget::~DockWidget()
{
    delete privateData;
}

void DockWidget::updatePalette(void)
{
    QPalette palette;
    QColor color = QApplication::palette().color(QPalette::Window);
    int alpha = 255;
    color.setAlpha(alpha);
    palette.setColor(QPalette::Window, color);
    color = palette.color(QPalette::Button);
    color.setAlpha(alpha);
    palette.setColor(QPalette::Button, color);
    color = palette.color(QPalette::Base);
    color.setAlpha(alpha);
    palette.setColor(QPalette::Base, color);
    color = palette.color(QPalette::AlternateBase);
    color.setAlpha(alpha);
    palette.setColor(QPalette::AlternateBase, color);

    setPalette(palette);
}
Qt::DockWidgetArea DockWidget::dockLocation(void) const
{
    return privateData->dockWidgetArea;
}

TransparentDock *DockWidget::floatWidget(void) const
{
    return privateData->floatWidget;
}

void DockWidget::setFloatAlignWidget(QWidget *widget)
{
    floatWidget()->setAlignWidget(widget);
}

QWidget *DockWidget::widget(void) const
{
    if (isFloating())
        return floatWidget()->widget();
    else
        return QDockWidget::widget();
}

void DockWidget::setWidget(QWidget *widget)
{
    if (isFloating())
        floatWidget()->setWidget(widget);
    else
        QDockWidget::setWidget(widget);
}

bool DockWidget::autoHide(void) const
{
    return floatWidget()->autoHide();
}

void DockWidget::setAutoHide(bool autoHide)
{
    floatWidget()->setAutoHide(autoHide);
}

void DockWidget::setDockLocation(Qt::DockWidgetArea area)
{
    privateData->dockWidgetArea = area;
    privateData->floatWidget->setDockLocation(area);
    privateData->floatWidget->adjustSize();
    privateData->floatWidget->alignToParent();
}

void DockWidget::handleTopLevelChangedSignal(bool topLevel)
{
    if (topLevel && !isFloating())
        return;
    else if (!topLevel && isFloating())
        return;

    QDockWidget::setVisible(!topLevel);
    floatWidget()->QWidget::setVisible(topLevel);

    if (topLevel)
    {
        floatWidget()->setTitle(windowTitle());
        floatWidget()->updatePalette();
        floatWidget()->setWidget(QDockWidget::widget());
    }
    else
    {
        updatePalette();
        QDockWidget::setWidget(floatWidget()->widget());
    }
}

bool DockWidget::isVisible(void) const
{
    if (isFloating())
        return floatWidget()->isSlidIn() && floatWidget()->isVisible();

    return QDockWidget::isVisible();
}

void DockWidget::setVisible(bool visible)
{
    if (isFloating())
    {
        if (QDockWidget::isVisible())
            QDockWidget::setVisible(false);

        if (!floatWidget()->isVisible())
            floatWidget()->QWidget::setVisible(true);

        if ((visible && !floatWidget()->isSlidIn()) || (!visible && floatWidget()->isSlidIn()))
        {
            floatWidget()->setTitle(windowTitle());
            floatWidget()->setVisible(visible);
            emit dockVisibilityChanged(visible);
        }

    }
    else
    {
        if (dockLocation() == Qt::RightDockWidgetArea || dockLocation() == Qt::BottomDockWidgetArea)
        {
            if (floatWidget()->isVisible())
            {
                floatWidget()->setVisible(false);
                floatWidget()->QWidget::setVisible(false);
            }

            if ((visible && !isVisible()) || (!visible && isVisible()))
            {
                QDockWidget::setVisible(visible);
                emit dockVisibilityChanged(visible);
            }

            return;
        }


        bool mainWindowUpdatesEnabled = mainWindow->updatesEnabled();
        mainWindow->setUpdatesEnabled(false);

        QSize maxSize = maximumSize();
        setMaximumSize(size());

        QList<DockWidget *> dockWidgetList;
        QList<TransparentDock *> floatDockWidgetList;
        QList<QSize> maximumDockWidgetSizeList;
        for (int i = 0; i < mainWindow->children().size(); i++)
        {
            DockWidget *dockWidget =  qobject_cast<DockWidget *>(mainWindow->children().at(i));
            if (!dockWidget || dockWidget == this)
                continue;

            if (dockWidget->isVisible())
            {
                dockWidgetList << dockWidget;
                maximumDockWidgetSizeList << dockWidget->maximumSize();
                dockWidget->setMaximumSize(dockWidget->size());
                if (dockLocation() != dockWidget->dockLocation() && !dockWidget->isFloating())
                    dockWidget->QDockWidget::setVisible(false);
            }

            if (dockWidget->floatWidget()->isVisible())
            {
                floatDockWidgetList << dockWidget->floatWidget();
                dockWidget->floatWidget()->QWidget::setVisible(false);
            }
        }

        if (mainWindow->centralWidget())
            mainWindow->centralWidget()->hide();

        if (floatWidget()->isVisible())
        {
            floatWidget()->setVisible(false);
            floatWidget()->QWidget::setVisible(false);
        }

        if ((visible && !isVisible()) || (!visible && isVisible()))
        {
            QDockWidget::setVisible(visible);
            emit dockVisibilityChanged(visible);
        }

        if (mainWindow->centralWidget())
            mainWindow->centralWidget()->show();


        for (int i = 0; i < dockWidgetList.size(); i++)
        {
            dockWidgetList.at(i)->setMaximumSize(maximumDockWidgetSizeList.at(i));
            if (dockLocation() != dockWidgetList.at(i)->dockLocation() && !dockWidgetList.at(i)->isFloating())
                dockWidgetList.at(i)->QDockWidget::setVisible(true);
        }

        for (int i = 0; i < floatDockWidgetList.size(); i++)
        {
            floatDockWidgetList.at(i)->QWidget::setVisible(true);
        }

        setMaximumSize(maxSize);

        mainWindow->setUpdatesEnabled(mainWindowUpdatesEnabled);
    }
}

void DockWidget::slideInOut(bool visible)
{
    if (isFloating())
        floatWidget()->slideInOut(visible);
    else
        setVisible(visible);
}
