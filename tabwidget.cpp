#include "tabwidget.h"
#include <QDebug>

TabWidget::TabWidget(QWidget *parent) :
    QTabWidget(parent)
{
}

QSize TabWidget::sizeHint(void) const
{
    QSize hint(0, 0);
    for (int i = 0; i < count(); i++)
    {
        QSize s = widget(i)->sizeHint();

        if (s.width() > hint.width())
            hint.setWidth(s.width());
        if (s.height() > hint.height())
            hint.setHeight(s.height());
    }

    return hint;
}
