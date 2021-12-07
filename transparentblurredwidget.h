#ifndef TRANSPARENTBLURREDWIDGET_H
#define TRANSPARENTBLURREDWIDGET_H

#include <QWidget>

class TransparentBlurredWidget : public QWidget
{
        Q_OBJECT
        Q_PROPERTY(qreal widgetOpacity READ widgetOpacity WRITE setWidgetOpacity)

    public:
        explicit TransparentBlurredWidget(QWidget *parent = 0);
        ~TransparentBlurredWidget(void);

        qreal widgetOpacity(void) const;
        void setWidgetOpacity(qreal opacity);

    public slots:

    signals:

    protected:
        virtual void paintEvent(QPaintEvent *paintEvent);

    private:
        void renderBackGround(const QRect &rect);
        class PrivateData;
        PrivateData *privateData;
};

#endif // TRANSPARENTBLURREDWIDGET_H
