#ifndef SLIDINGSTACKEDWIDGET_H
#define SLIDINGSTACKEDWIDGET_H

#include <QStackedWidget>

#include <QtGui>
#include <QWidget>
#include <QDebug>
#include <QEasingCurve>

class SlidingStackedWidget : public QWidget
{
        Q_OBJECT

    public:
        enum t_direction
        {
            LEFT2RIGHT,
            RIGHT2LEFT,
            TOP2BOTTOM,
            BOTTOM2TOP,
            AUTOMATIC
        };

        SlidingStackedWidget(QWidget *parent);
        ~SlidingStackedWidget(void);

        void setNavigationButtonsEnabled(bool enabled);
        int addWidget(QWidget *widget);
        int count(void) const;
        int currentIndex(void) const;
        QWidget *currentWidget(void) const;
        int indexOf(QWidget *widget) const;
        int insertWidget(int index, QWidget *widget);
        void removeWidget(QWidget *widget);
        QWidget *widget(int index) const;

    public slots:
        void setCurrentIndex(int index);
        void setCurrentWidget(QWidget *widget);
        void setSpeed(int speed);
        void setAnimation(enum QEasingCurve::Type animationtype);
        void setVerticalMode(bool vertical = true);
        void setWrap(bool wrap);
        void slideInNext();
        void slideInPrev();
        void slideInIdx(int idx, enum t_direction direction = AUTOMATIC);
        void updateIndexStatus(void);

    signals:
        void animationFinished(void);
        void startReached(bool value);
        void endReached(bool value);
        void currentChanged(int index);
        void widgetRemoved(int index);

    protected slots:
        void animationDoneSlot(void);

    protected:
        void slideInWgt(QWidget *widget, enum t_direction direction = AUTOMATIC);

    private:
        class PrivateData;
        PrivateData *privateData;
};

#endif // SLIDINGSTACKEDWIDGET_H
