#ifndef PLOTIMAGEBACKGROUND_H
#define PLOTIMAGEBACKGROUND_H

#include <qwt_plot_item.h>

class PlotImageBackground : public QwtPlotItem
{
    public:
        PlotImageBackground(void);
        ~PlotImageBackground(void);
        void setImage(const QImage &image);
        void setAlphaEnabled(int alpha);
        void setGrayScaleEnabled(bool value);
        virtual int rtti(void) const;
        virtual void draw(QPainter *painter, const QwtScaleMap &, const QwtScaleMap &yMap, const QRect &rect) const;
        void updateImage(void);

    private:
        class PrivateData;
        PrivateData *privateData;
};

#endif // PLOTIMAGEBACKGROUND_H
