#ifndef ANIMATEDSLIDEWIDGET_H
#define ANIMATEDSLIDEWIDGET_H

#include <QWidget>

class AnimatedSlideWidget2 : public QWidget
{
        Q_OBJECT

    public:
        explicit AnimatedSlideWidget2(QWidget *parent = 0, Qt::WindowFlags flags = 0);
        ~AnimatedSlideWidget2(void);

    public slots:
        virtual void setVisible(bool visible);

    signals:
        void animatedMoveIn(void);
        void animatedMoveOut(void);

    protected:
        virtual void moveEvent(QMoveEvent *moveEvent);

    private slots:
        void animationFinished(void);

    private:
        class PrivateData;
        PrivateData *privateData;
};

class AnimatedSlideWidget : public QWidget
{
        Q_OBJECT

    public:
        explicit AnimatedSlideWidget(QWidget *parent = 0, Qt::WindowFlags flags = 0);
        ~AnimatedSlideWidget(void);

        void setWidget(QWidget *widget);

    public slots:
        virtual void setVisible(bool visible);

    signals:
        void animatedMoveIn(void);
        void animatedMoveOut(void);

    private slots:
        void animationFinished(void);

    private:
        class PrivateData;
        PrivateData *privateData;
};

#endif // ANIMATEDSLIDEWIDGET_H
