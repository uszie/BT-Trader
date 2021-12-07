#include "menucontextmenu.h"
#include <QEvent>
#include <QMouseEvent>
#include <QDebug>

MenuContextMenu::MenuContextMenu(QMenu *menu, QAction *action) :
    QMenu(menu)
{
    monitoredMenu = menu;
    monitoredAction = action;
    monitoredMenu->installEventFilter(this);
}

bool MenuContextMenu::eventFilter(QObject *object, QEvent *event)
{
    if ((object == monitoredMenu) && (event->type() == QEvent::ContextMenu) && (monitoredMenu->activeAction() == monitoredAction))
    {
        monitoredMenu->setActiveAction(monitoredAction);
        exec(QCursor::pos());
        monitoredMenu->setActiveAction(monitoredAction);
        return true;
    }

    return QObject::eventFilter(object, event);
}
