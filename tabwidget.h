#ifndef TABWIDGET_H
#define TABWIDGET_H

#include <QTabWidget>

class TabWidget : public QTabWidget
{
        Q_OBJECT

    public:
        explicit TabWidget(QWidget *parent = 0);
        virtual QSize sizeHint(void) const;
};

#endif // TABWIDGET_H
