#ifndef STRATEGIEOVERVIEW_H
#define STRATEGIEOVERVIEW_H

#include "bttrader.h"
#include "strategie.h"
#include <QWidget>

class GraphicsPixmapItem : public QObject, public QGraphicsPixmapItem
{
        Q_OBJECT
        Q_PROPERTY(qreal rotation READ rotation WRITE setRotation)
        Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)

    public:
        GraphicsPixmapItem(QObject *parent = 0) :
            QObject(parent),
            QGraphicsPixmapItem(0)
        {

        }
};

namespace Ui
{
class StrategieOverview;
}

class StrategieOverview : public QDockWidget
{
        Q_OBJECT

    public:
        explicit StrategieOverview(PriceChart *priceChart, QWidget *parent = 0);
        ~StrategieOverview();

        PriceChart *priceChart(void) const;

    public slots:
        void updateTitle(void);
        void updatePriceQuote(PriceEntry &priceEntry);
        void rotateIconUp(void);
        void rotateIconDown(void);
        void triggerAlarm(void);
        void resetAlarm(void);

    signals:
        void iconUpRotationRequested(void);
        void iconDownRotationRequested(void);

    protected:
        virtual bool eventFilter(QObject *object, QEvent *event);
        virtual void closeEvent(QCloseEvent *event);

    private slots:
        void setupToolTip(void);
        void switchToThisStrategie(void);
        void doUpdatePriceQuote(QList<PriceEntry> &priceEntryList);
        void pulseIcon(void);
        void updatePalette();
        void updateGraphicsViewBackground();

    private:
        class PrivateData;
        PrivateData *privateData;
};

#endif // STRATEGIEOVERVIEW_H
