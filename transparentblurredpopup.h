#ifndef TRANSPARENTBLURREDPOPUP_H
#define TRANSPARENTBLURREDPOPUP_H

#include "transparentblurredwidget.h"

class TransparentBlurredPopup : public TransparentBlurredWidget
{
        Q_OBJECT

    public:
        TransparentBlurredPopup(QWidget *parent = 0);
        ~TransparentBlurredPopup();

    protected:
        virtual void paintEvent(QPaintEvent *paintEvent);
        virtual void resizeEvent(QResizeEvent *resizeEvent);
        virtual void setVisible(bool visible);

    signals:
        void shown(void);
        void hidden(void);

    private:
        class PrivateData;
        PrivateData *privateData;
};

#endif // TRANSPARENTBLURREDPOPUP_H
