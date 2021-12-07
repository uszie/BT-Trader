#include "transparentblurredwidget.h"
#include "bttrader.h"
#include "blur.h"

#include <QApplication>
#include <QDebug>

#define BLUR_RADIUS 18 //12
#define GRAB_OFFSET ( ( ( ( ( ( 2 * BLUR_RADIUS ) - 1 ) / 2 ) + 1 ) * 2 ) + 6 )

#if defined(Q_WS_X11)
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <QX11Info>

static void enableBlurBehindWindow(WId window, bool enable, const QRegion &region = QRegion())
{
    Display *dpy = QX11Info::display();
    Atom atom = XInternAtom(dpy, "_KDE_NET_WM_BLUR_BEHIND_REGION", False);

    if (enable)
    {
        QVector<QRect> rects = region.rects();
        QVector<quint32> data;
        for (int i = 0; i < rects.count(); i++)
        {
            const QRect r = rects[i];
            data << r.x() << r.y() << r.width() << r.height();
        }

        XChangeProperty(dpy, window, atom, XA_CARDINAL, 32, PropModeReplace, reinterpret_cast<const unsigned char *>(data.constData()), data.size());
    }
    else
    {
        XDeleteProperty(dpy, window, atom);
    }
}
#endif

class TransparentBlurredWidget::PrivateData
{
    public:
        qreal widgetOpacity;
        QImage backgroundImage;
        bool windowMode;
};

TransparentBlurredWidget::TransparentBlurredWidget(QWidget *parent) :
    QWidget(parent)
{
    privateData = new PrivateData;
    privateData->windowMode = parent ? false : true;

    setWidgetOpacity(0.6);
    setAttribute(Qt::WA_NoSystemBackground);
    if (privateData->windowMode)
        setAttribute(Qt::WA_TranslucentBackground);
}

TransparentBlurredWidget::~TransparentBlurredWidget()
{
    delete privateData;
}

void TransparentBlurredWidget::paintEvent(QPaintEvent *paintEvent)
{
    QPainter widgetPainter(this);
    widgetPainter.save();
    widgetPainter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform, false);

    enableBlurBehindWindow(winId(), privateData->windowMode);
    if (privateData->windowMode)
        widgetPainter.fillRect(paintEvent->rect(), palette().window());
    else
    {
        renderBackGround(paintEvent->rect());
        QRect imageRect = privateData->backgroundImage.rect().adjusted(GRAB_OFFSET, GRAB_OFFSET, -GRAB_OFFSET, -GRAB_OFFSET);
        widgetPainter.drawImage(paintEvent->rect(), privateData->backgroundImage, imageRect, Qt::ThresholdDither | Qt::NoOpaqueDetection | Qt::AvoidDither);
    }

    widgetPainter.restore();
}

qreal TransparentBlurredWidget::widgetOpacity() const
{
    return privateData->widgetOpacity;
}

void TransparentBlurredWidget::setWidgetOpacity(qreal opacity)
{
    privateData->widgetOpacity = opacity;

    QPalette palette;
    QColor color = QApplication::palette().color(QPalette::Window);
    int alpha = static_cast<int>(opacity * 255.0);
    color.setAlpha(static_cast<int>(alpha));
    palette.setColor(QPalette::Window, color);
    alpha = 50;
    color = palette.color(QPalette::Button);
    color.setAlpha(alpha);
    palette.setColor(QPalette::Button, color);
    palette.setColor(QPalette::Base, QColor(0, 0, 0, 0));
    palette.setColor(QPalette::AlternateBase, QColor(0, 0, 0, 0));
    color = palette.color(QPalette::Highlight);
    color.setAlpha(alpha);
    palette.setColor(QPalette::Highlight, color);
    color = palette.color(QPalette::Light);
    color.setAlpha(alpha);
    palette.setColor(QPalette::Light, color);
    color = palette.color(QPalette::Midlight);
    color.setAlpha(alpha);
    palette.setColor(QPalette::Midlight, color);
    color = palette.color(QPalette::Mid);
    color.setAlpha(alpha);
    palette.setColor(QPalette::Mid, color);
    color = palette.color(QPalette::Dark);
    color.setAlpha(alpha);
    palette.setColor(QPalette::Dark, color);
    color = palette.color(QPalette::BrightText);
    color.setAlpha(alpha);
    palette.setColor(QPalette::Dark, QPalette::BrightText);
    setPalette(palette);

}

void TransparentBlurredWidget::renderBackGround(const QRect &rect)
{
    QRect grabRect = rect;
    grabRect.moveTopLeft(mapTo(window(), grabRect.topLeft()));
    grabRect = grabRect.adjusted(-GRAB_OFFSET, -GRAB_OFFSET, +GRAB_OFFSET, +GRAB_OFFSET);
    if (grabRect.isValid())
    {
        privateData->backgroundImage = QImage(grabRect.size(), QImage::Format_ARGB32_Premultiplied);

        bool updatesDisabled = testAttribute(Qt::WA_UpdatesDisabled);
        setAttribute(Qt::WA_UpdatesDisabled, true);
        bool opaquePaintEvent = testAttribute(Qt::WA_OpaquePaintEvent);
        setAttribute(Qt::WA_OpaquePaintEvent, false);

        window()->render(&privateData->backgroundImage, QPoint(), QRegion(grabRect));

        setAttribute(Qt::WA_OpaquePaintEvent, opaquePaintEvent);
        setAttribute(Qt::WA_UpdatesDisabled, updatesDisabled);

        vectorExponentialBlur(privateData->backgroundImage, BLUR_RADIUS, true, palette().color(QPalette::Window));
    }
    else
        qDebug() << "TransparentDock::renderBackGround(): invalid grab rect " << grabRect;
}
