#include "verticalscrollarea.h"
#include <QVBoxLayout>
#include <QEvent>
#include <QScrollBar>
#include <QDebug>

VerticalScrollArea::VerticalScrollArea(QWidget *parent) :
    QScrollArea(parent)
{
}

bool VerticalScrollArea::eventFilter(QObject *o, QEvent *e)
{
    if (o == widget() && e->type() == QEvent::Resize)
        setMinimumWidth(widget()->sizeHint().width() + (verticalScrollBar()->isVisible() ? verticalScrollBar()->width() : 0) + contentsMargins().left() + contentsMargins().right() + 0);

    return false;
}

QSize VerticalScrollArea::sizeHint(void) const
{
    QSize hint = QScrollArea::sizeHint();
    if (widget())
        hint.setWidth(widget()->sizeHint().width() + (verticalScrollBar()->isVisible() ? verticalScrollBar()->width() : 0) + contentsMargins().left() + contentsMargins().right() + 0);

    return hint;
}
