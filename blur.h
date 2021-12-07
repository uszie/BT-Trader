#ifndef BLUR_H
#define BLUR_H

#include <QtGui>

void vectorInplaceImageScale0_5(QImage &imageIn, QImage &imageOut, bool &oddX, bool &oddY, bool useThreads, int maximumThreads, int chunkSize);
void vectorImageScale0_5(QImage &imageIn, QImage &imageOut, bool &oddX, bool &oddY, bool useThreads);
void vectorAlphaBlend(QImage &image, QColor color, bool useThreads);
void vectorTranspose(QImage &image , bool useThreads);
void vectorInplaceImageScale2(QImage &imageIn, QImage &imageOut, bool oddX, bool oddY, bool useThreads, int maximumThreads, int chunkSize);
void vectorImageScale2(QImage &imageIn, QImage &imageOut, bool oddX, bool oddY, bool useThreads);
void vectorExponentialBlur(QImage &image, int radius , bool useThreads = true , const QColor &color = Qt::transparent);

class GraphicsBlurEffect : public QGraphicsEffect
{
        Q_OBJECT
        Q_PROPERTY(qreal blurRadius READ blurRadius WRITE setBlurRadius NOTIFY blurRadiusChanged)

    public:
        GraphicsBlurEffect(QObject *parent = 0);
        ~GraphicsBlurEffect();

        qreal blurRadius() const;
        QRectF boundingRectFor(const QRectF &rect) const;

    public slots:
        void setBlurRadius(qreal blurRadius);
        void setBlurRadius(int blurRadius);
        void fakeSlot(int fake) { (void) fake; }

    signals:
        void blurRadiusChanged(qreal blurRadius);

    protected:
        void draw(QPainter *painter);

    private:
        qreal radius;
};

#endif // BLUR_H
