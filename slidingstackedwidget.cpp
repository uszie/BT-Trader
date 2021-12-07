#include "slidingstackedwidget.h"
#include "main.h"
#include <iostream>

class SlidingStackedWidget::PrivateData
{
    public:
        QStackedWidget *stackWidget;
        OrientationButton *previousButton;
        OrientationButton *nextButton;
        int speed;
        enum QEasingCurve::Type animationType;
        bool vertical;
        int now;
        int next;
        bool wrap;
        QPoint pnow;
        bool active;
};

SlidingStackedWidget::SlidingStackedWidget(QWidget *parent)
    : QWidget(parent)
{
    privateData = new PrivateData;
    privateData->vertical = false;
    privateData->speed = 800;
    privateData->animationType = QEasingCurve::OutBack;
    privateData->now = 0;
    privateData->next = 0;
    privateData->wrap = false;
    privateData->pnow = QPoint(0, 0);
    privateData->active = false;

    QGridLayout *gridLayout = new QGridLayout;
    setLayout(gridLayout);

    privateData->stackWidget = new QStackedWidget;
    gridLayout->addWidget(privateData->stackWidget, 1, 1);

    privateData->previousButton = new OrientationButton(tr("Vorige"));
    privateData->previousButton->setOrientation(Qt::Vertical);
    privateData->previousButton->setMirrored(true);
    privateData->previousButton->setFlat(true);
    privateData->previousButton->setMaximumWidth(25);
    privateData->previousButton->setIcon(qApp->style()->standardIcon(QStyle::SP_ArrowUp));
    privateData->previousButton->setIconSize(QSize(80, 25));
    privateData->previousButton->setStyleSheet("QPushButton { font-size: " + QString::number(privateData->previousButton->font().pointSize() + 1) + "pt; font-weight: bold; }");
    gridLayout->addWidget(privateData->previousButton, 1, 0);
    connect(privateData->previousButton, SIGNAL(clicked()), this, SLOT(slideInPrev()));
    connect(this, SIGNAL(startReached(bool)), privateData->previousButton, SLOT(setDisabled(bool)));

    privateData->nextButton = new OrientationButton(tr("Volgende"));
    privateData->nextButton->setOrientation(Qt::Vertical);
    privateData->nextButton->setMirrored(true);
    privateData->nextButton->setFlat(true);
    privateData->nextButton->setMaximumWidth(25);
    privateData->nextButton->setIcon(qApp->style()->standardIcon(QStyle::SP_ArrowDown));
    privateData->nextButton->setIconSize(QSize(80, 25));
    privateData->nextButton->setStyleSheet("QPushButton { font-size: " + QString::number(privateData->previousButton->font().pointSize() + 1) + "pt; font-weight: bold; }");
    gridLayout->addWidget(privateData->nextButton, 1, 2);
    connect(privateData->nextButton, SIGNAL(clicked()), this, SLOT(slideInNext()));
    connect(this, SIGNAL(endReached(bool)), privateData->nextButton, SLOT(setDisabled(bool)));

    setAttribute(Qt::WA_NoSystemBackground);

    connect(privateData->stackWidget, SIGNAL(currentChanged(int)), this, SLOT(updateIndexStatus()));
    connect(privateData->stackWidget, SIGNAL(currentChanged(int)), this, SIGNAL(currentChanged(int)));
    connect(privateData->stackWidget, SIGNAL(widgetRemoved(int)), this, SLOT(updateIndexStatus()));
    connect(privateData->stackWidget, SIGNAL(widgetRemoved(int)), this, SIGNAL(widgetRemoved(int)));
}


SlidingStackedWidget::~SlidingStackedWidget()
{
}

void SlidingStackedWidget::setNavigationButtonsEnabled(bool enabled)
{
    privateData->previousButton->setVisible(enabled);
    privateData->nextButton->setVisible(enabled);
}

int SlidingStackedWidget::addWidget(QWidget *widget)
{
    return privateData->stackWidget->addWidget(widget);
}

int SlidingStackedWidget::count(void) const
{
    return privateData->stackWidget->count();
}

int SlidingStackedWidget::currentIndex(void) const
{
    return privateData->stackWidget->currentIndex();
}

QWidget *SlidingStackedWidget::currentWidget(void) const
{
    return privateData->stackWidget->currentWidget();
}

int SlidingStackedWidget::indexOf(QWidget *widget) const
{
    return privateData->stackWidget->indexOf(widget);
}

int SlidingStackedWidget::insertWidget(int index, QWidget *widget)
{
    return privateData->stackWidget->insertWidget(index, widget);
}

void SlidingStackedWidget::removeWidget(QWidget *widget)
{
    privateData->stackWidget->removeWidget(widget);
}

QWidget *SlidingStackedWidget::widget(int index) const
{
    return privateData->stackWidget->widget(index);
}

void SlidingStackedWidget::setVerticalMode(bool vertical)
{
    privateData->vertical = vertical;
    QGridLayout *gridLayout = reinterpret_cast<QGridLayout *>(layout());
    gridLayout->removeWidget(privateData->previousButton);
    gridLayout->removeWidget(privateData->nextButton);

    if (privateData->vertical)
    {
        privateData->previousButton->setOrientation(Qt::Horizontal);
        privateData->previousButton->setMirrored(false);
        privateData->previousButton->setMaximumWidth(QWIDGETSIZE_MAX);
        privateData->previousButton->setMaximumHeight(25);
        gridLayout->addWidget(privateData->previousButton, 0, 1);

        privateData->nextButton->setOrientation(Qt::Horizontal);
        privateData->nextButton->setMirrored(false);
        privateData->nextButton->setMaximumWidth(QWIDGETSIZE_MAX);
        privateData->nextButton->setMaximumHeight(25);
        gridLayout->addWidget(privateData->nextButton, 2, 1);
    }
    else
    {
        privateData->previousButton->setOrientation(Qt::Vertical);
        privateData->previousButton->setMirrored(true);
        privateData->previousButton->setMaximumWidth(25);
        privateData->previousButton->setMaximumHeight(QWIDGETSIZE_MAX);
        gridLayout->addWidget(privateData->previousButton, 1, 0);

        privateData->nextButton->setOrientation(Qt::Vertical);
        privateData->nextButton->setMirrored(true);
        privateData->nextButton->setMaximumWidth(25);
        privateData->nextButton->setMaximumHeight(QWIDGETSIZE_MAX);
        gridLayout->addWidget(privateData->nextButton, 1, 2);
    }
}

void SlidingStackedWidget::setCurrentIndex(int index)
{
    privateData->stackWidget->setCurrentIndex(index);
}

void SlidingStackedWidget::setCurrentWidget(QWidget *widget)
{
    privateData->stackWidget->setCurrentWidget(widget);
}

void SlidingStackedWidget::setSpeed(int speed)
{
    privateData->speed = speed;
}

void SlidingStackedWidget::setAnimation(enum QEasingCurve::Type animationType)
{
    privateData->animationType = animationType;
}

void SlidingStackedWidget::setWrap(bool wrap)
{
    privateData->wrap = wrap;
}

void SlidingStackedWidget::slideInNext()
{
    int now = currentIndex();
    if (privateData->wrap || (now < count() - 1))
        slideInIdx(now + 1);
}


void SlidingStackedWidget::slideInPrev()
{
    int now = currentIndex();
    if (privateData->wrap || (now > 0))
        slideInIdx(now - 1);
}

void SlidingStackedWidget::slideInIdx(int index, enum t_direction direction)
{
    //int idx, t_direction direction=AUTOMATIC
    if (index > count() - 1)
    {
        direction = privateData->vertical ? TOP2BOTTOM : RIGHT2LEFT;
        index = (index) % count();
    }
    else if (index < 0)
    {
        direction = privateData->vertical ? BOTTOM2TOP : LEFT2RIGHT;
        index = (index + count()) % count();
    }

    widget(index)->setAttribute(Qt::WA_NoSystemBackground);
    slideInWgt(widget(index), direction);
}

void SlidingStackedWidget::updateIndexStatus(void)
{
    if (currentIndex() == count() - 1)
        emit endReached(true);
    else
        emit endReached(false);

    if (currentIndex() == 0)
        emit startReached(true);
    else
        emit startReached(false);
}

void SlidingStackedWidget::slideInWgt(QWidget *newwidget, enum t_direction  direction)
{
    if (privateData->active)
        return;
    else
        privateData->active = true;

    enum t_direction directionhint;
    int now = currentIndex();
    int next = indexOf(newwidget);
    if (now == next)
    {
        privateData->active = false;
        return;
    }
    else if (now < next)
        directionhint = privateData->vertical ? TOP2BOTTOM : RIGHT2LEFT;
    else
        directionhint = privateData->vertical ? BOTTOM2TOP : LEFT2RIGHT;

    if (direction == AUTOMATIC)
        direction = directionhint;

    int offsetx = privateData->stackWidget->frameRect().width();
    int offsety = privateData->stackWidget->frameRect().height();

    widget(next)->setGeometry(0, 0, offsetx, offsety);

    if (direction == BOTTOM2TOP)
    {
        offsetx = 0;
        offsety = -offsety;
    }
    else if (direction == TOP2BOTTOM)
    {
        offsetx = 0;
    }
    else if (direction == RIGHT2LEFT)
    {
        offsetx = -offsetx;
        offsety = 0;
    }
    else if (direction == LEFT2RIGHT)
    {
        offsety = 0;
    }

    QPoint pnext = widget(next)->pos();
    QPoint pnow = widget(now)->pos();
    privateData->pnow = pnow;

    widget(next)->move(pnext.x() - offsetx, pnext.y() - offsety);
    widget(next)->show();
    widget(next)->raise();

    QPropertyAnimation *animnow = new QPropertyAnimation(widget(now), "pos");
    animnow->setDuration(privateData->speed);
    animnow->setEasingCurve(privateData->animationType);
    animnow->setStartValue(QPoint(pnow.x(), pnow.y()));
    animnow->setEndValue(QPoint(offsetx + pnow.x(), offsety + pnow.y()));
    QPropertyAnimation *animnext = new QPropertyAnimation(widget(next), "pos");
    animnext->setDuration(privateData->speed);
    animnext->setEasingCurve(privateData->animationType);
    animnext->setStartValue(QPoint(-offsetx + pnext.x(), -offsety + pnext.y()));
    animnext->setEndValue(QPoint(pnext.x(), pnext.y()));

    QParallelAnimationGroup *animgroup = new QParallelAnimationGroup;
    animgroup->addAnimation(animnow);
    animgroup->addAnimation(animnext);

    QObject::connect(animgroup, SIGNAL(finished()), this, SLOT(animationDoneSlot()));
    privateData->next = next;
    privateData->now = now;
    privateData->active = true;
    animgroup->start();
}

void SlidingStackedWidget::animationDoneSlot(void)
{
    setCurrentIndex(privateData->next);
    widget(privateData->now)->hide();
    widget(privateData->now)->move(privateData->pnow);
    privateData->active = false;

    emit animationFinished();
}
