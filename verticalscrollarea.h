#ifndef VERTICALSCROLLAREA_H
#define VERTICALSCROLLAREA_H

#include <QScrollArea>

class VerticalScrollArea : public QScrollArea
{
        Q_OBJECT
    public:
        explicit VerticalScrollArea(QWidget *parent = 0);

    protected:
        virtual bool eventFilter(QObject *o, QEvent *e);
        virtual QSize sizeHint(void) const;
};

#endif // VERTICALSCROLLAREA_H
