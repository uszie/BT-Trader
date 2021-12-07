#ifndef __MAIN_H
#define __MAIN_H

#include <qmainwindow.h>
#include <qsplitter.h>
#include <qlist.h>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_svgitem.h>
#include <qwt_plot_marker.h>
#include <qwt_event_pattern.h>
#include <qwt_symbol.h>
#include <QMouseEvent>
#include <QPushButton>
#include <QStyleOptionButton>
#include "scrollbar.h"
#include "scrollzoomer.h"
#include "indicator.h"
#include "movingaverage.h"
#include "strategie.h"
#include "ui_MovingAverage.h"
#include "ui_transactionstable.h"
#include "databrowser.h"

extern DataBrowser *dataBrowser;

class Widget : public QWidget
{
        Q_OBJECT

    public:
        explicit Widget(QWidget *parent = 0);
        ~Widget(void);

        void setWidget(QWidget *widget);
        virtual void setVisible(bool visible);
        virtual void moveEvent(QMoveEvent *moveEvent);

    private slots:
        void animationFinished(void);

    signals:
        void animatedMoveIn(void);
        void animatedMoveOut(void);

    private:
        class PrivateData;
        PrivateData *privateData;
};

class ColorPool
{
    public:
        ColorPool(void);

        const QColor &takeColor(const QColor &color = Qt::transparent);
        void putColor(const QColor &color);
        static QList<QColor> listColors(void) ;
        static int indexOfColor(const QColor &color) ;

    private:
        QList<QColor> colorsList;
        QList<int> colorsUsedList;
};

#endif
