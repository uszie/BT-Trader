#ifndef DATABROWSER_H
#define DATABROWSER_H

#include "dataconnection.h"
#include <QWidget>
#include <QTreeWidgetItem>

namespace Ui
{
class DataBrowser;
}

class DataBrowser : public QWidget
{
        Q_OBJECT

    public:
        explicit DataBrowser(QWidget *parent = 0);
        ~DataBrowser(void);

    public slots:
        void fillTree(void);

    signals:
        void actionCurrent(const IsinInfo &info);
        void actionNew(const IsinInfo &info);
        void actionComposition(const QString &title);

    private:
        Ui::DataBrowser *ui;

    private slots:
        void processDoubleClick(QTreeWidgetItem *item, int column);
        void processActionCurrent(void);
        void processActionNew(void);
        void processActionComposition(void);
        void showContextMenu(const QPoint &pos);
};

#endif // DATABROWSER_H
