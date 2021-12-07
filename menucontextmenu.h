#ifndef MENUCONTEXTMENU_H
#define MENUCONTEXTMENU_H

#include <QMenu>

class MenuContextMenu : public QMenu
{
        Q_OBJECT

    public:
        explicit MenuContextMenu(QMenu *menu, QAction *action);
        QAction *action(void) const { return monitoredAction; }

    protected:
        virtual bool eventFilter(QObject *object, QEvent *event);

    private:
        QAction *monitoredAction;
        QMenu *monitoredMenu;
};

#endif // MENUCONTEXTMENU_H
