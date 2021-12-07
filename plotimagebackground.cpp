#include "plotimagebackground.h"
#include <QPainter>

class PlotImageBackground::PrivateData
{
    public:
        int alpha;
        bool grayScale;
        bool fullScreen;
        QImage backgroundImage;
        QImage originalImage;
};

PlotImageBackground::PlotImageBackground(void) : QwtPlotItem()
{
    privateData = new PrivateData;
    privateData->alpha = 75;
    privateData->grayScale = false;
    privateData->fullScreen = true;

    setZ(0.0);
}

PlotImageBackground::~PlotImageBackground(void)
{
}

void PlotImageBackground::setImage(const QImage &image)
{
    privateData->originalImage = image;
    updateImage();
}

void PlotImageBackground::setAlphaEnabled(int alpha)
{
    privateData->alpha = alpha;
    updateImage();
}

void PlotImageBackground::setGrayScaleEnabled(bool value)
{
    privateData->grayScale = value;
    updateImage();
}

void PlotImageBackground::updateImage(void)
{
    if (privateData->originalImage.isNull())
        return;

    privateData->backgroundImage = privateData->originalImage;

    if (privateData->grayScale)
    {
        QVector <QRgb> colorTable;
        for (int i = 0; i < 256; i++)
        {
            if (i < 1)
                colorTable.append(qRgba(255, 255, 255, 0));
            else
                colorTable.append(qRgba(i, i, i, privateData->alpha));
        }

        privateData->backgroundImage = privateData->backgroundImage.convertToFormat(QImage::Format_Indexed8, colorTable);
    }
    else
    {
        privateData->backgroundImage = privateData->backgroundImage.convertToFormat(QImage::Format_ARGB32_Premultiplied);

        QPainter p;
        p.begin(&privateData->backgroundImage);
        p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        p.fillRect(privateData->backgroundImage.rect(), QColor(0, 0, 0, privateData->alpha));
        p.end();
    }
}

int PlotImageBackground::rtti() const
{
    return QwtPlotItem::Rtti_PlotUserItem;
}

void PlotImageBackground::draw(QPainter *painter, const QwtScaleMap &, const QwtScaleMap &yMap, const QRect &rect) const
{
    (void) yMap;

    if (privateData->backgroundImage.isNull())
        return;

    QRect r = rect;
    if (privateData->fullScreen)
    {
        ;
    }
    else
    {
        int offset = r.height() / 10;
        r.setTop(r.top() + (r.height() / 2) - offset);
        r.setBottom(r.bottom() - offset);
        float ratio = privateData->backgroundImage.rect().width() / privateData->backgroundImage.rect().height();
        r.setRight(r.left() + (ratio * r.height()));
    }

    painter->drawImage(r, privateData->backgroundImage);
}
