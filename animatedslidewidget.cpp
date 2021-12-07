#include "animatedslidewidget.h"
#include <QStateMachine>
#include <QPropertyAnimation>
#include <QVBoxLayout>
#include <QDebug>
#include <QTimer>
#include <QApplication>

class AnimatedSlideWidget2::PrivateData
{
    public:
        QPropertyAnimation *moveAnimation;
        bool animating;
        QStateMachine moveMachine;
        QState moveInState;
        QState moveOutState;
        QPoint startPos;
};

AnimatedSlideWidget2::AnimatedSlideWidget2(QWidget *parent, Qt::WindowFlags flags) : QWidget(parent, flags)
{
    privateData = new PrivateData;
    privateData->moveInState.addTransition(this, SIGNAL(animatedMoveOut()), &privateData->moveOutState);
    privateData->moveInState.assignProperty(this, "pos", pos());
    privateData->moveMachine.addState(&privateData->moveInState);

    privateData->moveOutState.addTransition(this, SIGNAL(animatedMoveIn()), &privateData->moveInState);
    privateData->moveOutState.assignProperty(this, "pos", pos());
    privateData->moveMachine.addState(&privateData->moveOutState);

    privateData->animating = false;
    privateData->moveAnimation = new QPropertyAnimation(this, "pos");
    privateData->moveAnimation->setDuration(200);
    connect(privateData->moveAnimation, SIGNAL(finished()), this, SLOT(animationFinished()));
    privateData->moveMachine.addDefaultAnimation(privateData->moveAnimation);
    privateData->moveMachine.setAnimated(false);
    privateData->moveMachine.setInitialState(&privateData->moveInState);
    privateData->moveMachine.start();
}

AnimatedSlideWidget2::~AnimatedSlideWidget2(void)
{
    delete privateData->moveAnimation;
    delete privateData;
}

void AnimatedSlideWidget2::setVisible(bool visible)
{
    QSet<QAbstractState *> set = privateData->moveMachine.configuration();
    if (set.contains(&privateData->moveOutState) && !visible)
        return QWidget::setVisible(false);
    else if (set.contains(&privateData->moveInState) && visible)
        return QWidget::setVisible(true);

    if (privateData->moveMachine.isRunning())
        privateData->moveMachine.setAnimated(true);
    else
        privateData->moveMachine.setAnimated(false);

    if (visible)
    {
        QPoint oldPos = pos();
        QWidget::setVisible(false);
        QWidget::setVisible(true);

        QWidget *p = parentWidget();
        if (p)
        {
            int height = p->height();
            if (p->sizeHint().height() > height)
                height = p->sizeHint().height();

            int width = p->width();
            if (p->sizeHint().width() > width)
                width = p->sizeHint().width();

            p->resize(width, height);
        }

        privateData->moveInState.assignProperty(this, "pos", pos());
        privateData->startPos = pos();
        if (oldPos.x() != pos().x() || oldPos.y() < (pos().y() - height()))
            move(QPoint(pos().x(), pos().y() - height()));
        else
            move(oldPos);
        privateData->moveAnimation->setStartValue(pos());
        privateData->animating = true;

        QRect maskRect = rect();
        maskRect.setBottom(maskRect.top() + 1);
        maskRect.setRight(maskRect.left() + 1);
        setMask(QRegion(maskRect));

        QTimer::singleShot(0, this, SIGNAL(animatedMoveIn()));
    }
    else
    {
        QPoint oldPos = pos();
        QWidget::setVisible(false);
        QWidget::setVisible(true);
        privateData->moveOutState.assignProperty(this, "pos", QPoint(pos().x(), pos().y() - size().height()));
        privateData->startPos = QPoint(pos().x(), pos().y() - size().height());
        move(oldPos);
        privateData->moveAnimation->setStartValue(pos());
        privateData->animating = true;

        QTimer::singleShot(0, this, SIGNAL(animatedMoveOut()));

        if (!privateData->moveMachine.isAnimated())
        {
            setMask(QRegion(0, 0, 1, 1));
            QWidget::setVisible(false);
        }
    }
}

void AnimatedSlideWidget2::moveEvent(QMoveEvent *moveEvent)
{
    (void) moveEvent;

    if (!privateData->animating)
        return;

    QSet<QAbstractState *> set = privateData->moveMachine.configuration();
    if (set.contains(&privateData->moveOutState))
    {
        QPoint difference = privateData->startPos - pos();
        int height = abs(difference.y());
        if (height >= size().height())
            height = size().height() - 1;
        QRect widgetRect = rect();
        widgetRect.setTop(widgetRect.bottom() - height);
        QRegion mask = QRegion(widgetRect);
        setMask(mask);
    }
    else if (set.contains(&privateData->moveInState))
    {
        QPoint difference = pos() - privateData->startPos;
        int height = abs(difference.y());
        if (height >= size().height())
            height = size().height() - 1;
        QRect widgetRect = rect();
        widgetRect.setTop(widgetRect.top() + height);
        QRegion mask = QRegion(widgetRect);
        setMask(mask);
    }
}

void AnimatedSlideWidget2::animationFinished()
{
    privateData->animating = false;

    QSet<QAbstractState *> set = privateData->moveMachine.configuration();
    if (set.contains(&privateData->moveOutState))
        QWidget::setVisible(false);
    else if (set.contains(&privateData->moveInState))
        clearMask();

    if (layout())
        layout()->update();
}

class AnimatedSlideWidget::PrivateData
{
    public:
        QWidget *dockWidget;
        QPropertyAnimation *moveAnimation;
        QStateMachine moveMachine;
        QState moveInState;
        QState moveOutState;
};

AnimatedSlideWidget::AnimatedSlideWidget(QWidget *parent, Qt::WindowFlags flags) :
    QWidget(parent, flags)
{
    privateData = new PrivateData;
    privateData->dockWidget = new QWidget;
    privateData->dockWidget->setLayout(new QVBoxLayout);
    setLayout(new QVBoxLayout);
    layout()->setContentsMargins(0, 0, 0, 0);
    layout()->addWidget(privateData->dockWidget);

    privateData->moveInState.addTransition(this, SIGNAL(animatedMoveOut()), &privateData->moveOutState);
    privateData->moveInState.assignProperty(privateData->dockWidget, "pos", privateData->dockWidget->pos());
    privateData->moveMachine.addState(&privateData->moveInState);

    privateData->moveOutState.addTransition(this, SIGNAL(animatedMoveIn()), &privateData->moveInState);
    privateData->moveOutState.assignProperty(privateData->dockWidget, "pos", privateData->dockWidget->pos());
    privateData->moveMachine.addState(&privateData->moveOutState);

    privateData->moveAnimation = new QPropertyAnimation(privateData->dockWidget, "pos");
    privateData->moveAnimation->setDuration(200);
    connect(privateData->moveAnimation, SIGNAL(finished()), this, SLOT(animationFinished()));
    privateData->moveMachine.addDefaultAnimation(privateData->moveAnimation);
    privateData->moveMachine.setInitialState(&privateData->moveInState);
    privateData->moveMachine.setAnimated(false);
    privateData->moveMachine.start();
}

AnimatedSlideWidget::~AnimatedSlideWidget(void)
{
    delete privateData->moveAnimation;
    delete privateData;
}

void AnimatedSlideWidget::setWidget(QWidget *widget)
{
    QLayoutItem *child;
    while ((child = privateData->dockWidget->layout()->takeAt(0)))
    {
        if (child->widget())
            child->widget()->hide();
    }

    if (widget)
        privateData->dockWidget->layout()->addWidget(widget);
}

void AnimatedSlideWidget::setVisible(bool visible)
{
    QSet<QAbstractState *> set = privateData->moveMachine.configuration();
    if (set.contains(&privateData->moveOutState) && !visible)
        return QWidget::setVisible(false);
    else if (set.contains(&privateData->moveInState) && visible)
        return QWidget::setVisible(true);

    if (privateData->moveMachine.isRunning())
        privateData->moveMachine.setAnimated(true);
    else
        privateData->moveMachine.setAnimated(false);

    if (visible)
    {
        QPoint oldPos = privateData->dockWidget->pos();
        QWidget::setVisible(false);
        QWidget::setVisible(true);
        privateData->moveInState.assignProperty(privateData->dockWidget, "pos", QPoint(0, 0));
        if (oldPos.x() != privateData->dockWidget->pos().x() || oldPos.y() < -privateData->dockWidget->height() || oldPos == QPoint(0, 0))
            privateData->dockWidget->move(0, -privateData->dockWidget->height());
        else
            privateData->dockWidget->move(oldPos);
        privateData->moveAnimation->setStartValue(privateData->dockWidget->pos());

        QTimer::singleShot(0, this, SIGNAL(animatedMoveIn()));
    }
    else
    {
        QPoint oldPos = privateData->dockWidget->pos();
        QWidget::setVisible(false);
        QWidget::setVisible(true);
        privateData->moveOutState.assignProperty(privateData->dockWidget, "pos", QPoint(0, -privateData->dockWidget->size().height()));
        privateData->dockWidget->move(oldPos);
        privateData->moveAnimation->setStartValue(privateData->dockWidget->pos());

        QTimer::singleShot(0, this, SIGNAL(animatedMoveOut()));

        if (!privateData->moveMachine.isAnimated())
            QWidget::setVisible(false);
    }
}

void AnimatedSlideWidget::animationFinished()
{
    QSet<QAbstractState *> set = privateData->moveMachine.configuration();
    if (set.contains(&privateData->moveOutState))
        QWidget::setVisible(false);
}

