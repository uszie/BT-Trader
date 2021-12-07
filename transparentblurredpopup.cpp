#include "transparentblurredpopup.h"
#include <QPainter>
#include <QApplication>
#include <QPropertyAnimation>
#include <QStateMachine>

#define CORNER_RADIUS 3

class TransparentBlurredPopup::PrivateData
{
    public:

        QPainterPath outlinePath;
        QLinearGradient horizontalGradient;
        QLinearGradient verticalGradient;
        QState showState;
        QState hideState;
        QStateMachine showMachine;
        QPropertyAnimation *showAnimation;
};

TransparentBlurredPopup::TransparentBlurredPopup(QWidget *parent) : TransparentBlurredWidget(parent)
{
    privateData = new PrivateData;

    if (!parent)
        setWindowFlags(Qt::ToolTip);

    privateData->showState.addTransition(this, SIGNAL(hidden()), &privateData->hideState);
    privateData->showMachine.addState(&privateData->showState);

    privateData->hideState.addTransition(this, SIGNAL(shown()), &privateData->showState);
    privateData->showMachine.addState(&privateData->hideState);

    privateData->showAnimation = new QPropertyAnimation(this, "geometry");
    privateData->showAnimation->setDuration(1000);
    privateData->showMachine.addDefaultAnimation(privateData->showAnimation);
    privateData->showMachine.setInitialState(&privateData->hideState);
    privateData->showMachine.setAnimated(true);
    privateData->showMachine.start();

}

TransparentBlurredPopup::~TransparentBlurredPopup()
{
    delete privateData;
}

void TransparentBlurredPopup::paintEvent(QPaintEvent *paintEvent)
{
    TransparentBlurredWidget::paintEvent(paintEvent);

    QPainter widgetPainter(this);
    widgetPainter.save();
    widgetPainter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform, false);

    QPen pen;
    pen.setWidth(4);
    pen.setBrush(privateData->horizontalGradient);
    widgetPainter.strokePath(privateData->outlinePath, pen);

    pen.setBrush(privateData->verticalGradient);
    widgetPainter.strokePath(privateData->outlinePath, pen);

    pen.setColor(palette().color(QPalette::Window));
    pen.setWidth(2);
    widgetPainter.strokePath(privateData->outlinePath, pen);

    widgetPainter.restore();
}

void TransparentBlurredPopup::resizeEvent(QResizeEvent *resizeEvent)
{
    (void) resizeEvent;

    privateData->outlinePath = QPainterPath();
    privateData->outlinePath.setFillRule(Qt::WindingFill);
    privateData->outlinePath.addRoundedRect(0, 0, rect().width(), rect().height(), CORNER_RADIUS, CORNER_RADIUS);

    privateData->outlinePath = privateData->outlinePath.simplified();
    QRegion maskregion = QRegion(privateData->outlinePath.toFillPolygon().toPolygon());
    setMask(maskregion);
    privateData->outlinePath = privateData->outlinePath.intersected(privateData->outlinePath.translated(-1, -1)).simplified();

    privateData->horizontalGradient = QLinearGradient(0, 0, rect().width(), 0);
    privateData->horizontalGradient.setColorAt(0.0, Qt::transparent);
    privateData->horizontalGradient.setColorAt(0.5, QApplication::palette().color(QPalette::Text));
    privateData->horizontalGradient.setColorAt(1.0, Qt::transparent);

    privateData->verticalGradient = QLinearGradient(0, 0, 0, rect().height());
    privateData->verticalGradient.setColorAt(0.0, Qt::transparent);
    privateData->verticalGradient.setColorAt(0.5, QApplication::palette().color(QPalette::Text));
    privateData->verticalGradient.setColorAt(1.0, Qt::transparent);
}

void TransparentBlurredPopup::setVisible(bool visible)
{
    return TransparentBlurredWidget::setVisible(visible);
    privateData->hideState.assignProperty(this, "geometry", QRect(QCursor::pos(), QCursor::pos()));
    privateData->showState.assignProperty(this, "geometry", QRect(QCursor::pos(), sizeHint()));
    if (visible)
        TransparentBlurredWidget::setVisible(true);

}
