#include "databrowser.h"
#include "ui_databrowser.h"
#include <iostream>
#include <QMenu>

class TreeWidgetItem : public QTreeWidgetItem
{
    public:
        IsinInfo isinInfo;
};

DataBrowser::DataBrowser(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DataBrowser)
{
    ui->setupUi(this);
    QList<QTreeWidgetItem *> topLevelItemsList;
    TreeWidgetItem *valutaItem = new TreeWidgetItem;
    valutaItem->setText(0, tr("Valuta"));
    topLevelItemsList.append(valutaItem);
    TreeWidgetItem *indicesItem = new TreeWidgetItem;
    indicesItem->setText(0, tr("Indices"));
    topLevelItemsList.append(indicesItem);
    TreeWidgetItem *resourcesItem = new TreeWidgetItem;
    resourcesItem->setText(0, tr("Grondstoffen"));
    topLevelItemsList.append(resourcesItem);

    ui->dataTreeWidget->addTopLevelItems(topLevelItemsList);
    ui->dataTreeWidget->expandAll();
    ui->dataTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->dataTreeWidget, SIGNAL(customContextMenuRequested(const QPoint &)), SLOT(showContextMenu(const QPoint &)));
    fillTree();
    connect(ui->dataTreeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(processDoubleClick(QTreeWidgetItem *, int)));
}

DataBrowser::~DataBrowser(void)
{
    delete ui;
}

void DataBrowser::fillTree(void)
{
    QSqlQuery valutaQuery;
    bool ok = valutaQuery.exec("SELECT * FROM \"INFO\" WHERE type = 'VALUTA';");
    if (!ok)
        qWarning("DataBrowser::fillTree(): Failed to query database: %s", valutaQuery.lastError().text().toAscii().data());
    else
    {
        QList<QTreeWidgetItem *> list = ui->dataTreeWidget->findItems(tr("Valuta"), Qt::MatchFixedString);
        if (list.size() >= 1)
        {
            while (valutaQuery.next())
            {
                TreeWidgetItem *valutaItem = new TreeWidgetItem;
                valutaItem->setText(0, valutaQuery.value(1).toString());
                IsinInfo info;
                info.isin = valutaQuery.value(0).toString();
                info.name = valutaQuery.value(1).toString();
                info.symbol = valutaQuery.value(2).toString();
                info.market = valutaQuery.value(3).toString();
                info.type = valutaQuery.value(4).toString();
                info.parent = valutaQuery.value(5).toString();
                info.valuta = valutaQuery.value(10).toString();
                info.providesBidAsk = valutaQuery.value(11).toBool();
                info.availableTimeScales = 0xffffffff;
                valutaItem->isinInfo = info;
                list.at(0)->addChild(valutaItem);
            }
        }
    }

    QSqlQuery indiceQuery;
    ok = indiceQuery.exec("SELECT * FROM \"INFO\" WHERE type = 'INDICE';");
    if (!ok)
        qWarning("DataBrowser::fillTree(): Failed to query database: %s", indiceQuery.lastError().text().toAscii().data());
    else
    {
        QList<QTreeWidgetItem *> list = ui->dataTreeWidget->findItems(tr("Indices"), Qt::MatchFixedString);
        if (list.size() >= 1)
        {
            while (indiceQuery.next())
            {
                TreeWidgetItem *indiceItem = new TreeWidgetItem;
                indiceItem->setText(0, indiceQuery.value(1).toString());
                IsinInfo info;
                info.isin = indiceQuery.value(0).toString();
                info.name = indiceQuery.value(1).toString();
                info.symbol = indiceQuery.value(2).toString();
                info.market = indiceQuery.value(3).toString();
                info.type = indiceQuery.value(4).toString();
                info.parent = indiceQuery.value(5).toString();
                info.valuta = indiceQuery.value(10).toString();
                info.providesBidAsk = indiceQuery.value(11).toBool();
                info.availableTimeScales = 0xffffffff;
                indiceItem->isinInfo = info;
                list.at(0)->addChild(indiceItem);

                QSqlQuery stockQuery;
                ok = stockQuery.exec("SELECT * FROM \"INFO\" WHERE parent = '" + indiceQuery.value(0).toString() + "' AND type = 'STOCK';");
                if (!ok)
                    qWarning("DataBrowser::fillTree(): Failed to query database: %s", stockQuery.lastError().text().toAscii().data());
                else
                {
                    while (stockQuery.next())
                    {
                        TreeWidgetItem *stockItem = new TreeWidgetItem;
                        stockItem->setText(0, stockQuery.value(1).toString());
                        IsinInfo info;
                        info.isin = stockQuery.value(0).toString();
                        info.name = stockQuery.value(1).toString();
                        info.symbol = stockQuery.value(2).toString();
                        info.market = stockQuery.value(3).toString();
                        info.type = stockQuery.value(4).toString();
                        info.parent = stockQuery.value(5).toString();
                        info.valuta = stockQuery.value(10).toString();
                        info.providesBidAsk = stockQuery.value(11).toBool();
                        info.availableTimeScales = 0xffffffff;
                        stockItem->isinInfo = info;
                        indiceItem->addChild(stockItem);
                    }
                }
            }
        }
    }
}

void DataBrowser::processDoubleClick(QTreeWidgetItem *item, int column)
{
    (void) column;

    if (item->text(0) == tr("Indices") || item->text(0) == tr("Valuta") || item->text(0) == tr("Grondstoffen"))
        return;

    TreeWidgetItem *tmp = static_cast<TreeWidgetItem *>(item);
    if (!tmp)
        return;

    emit actionCurrent(tmp->isinInfo);
}

void DataBrowser::processActionCurrent(void)
{
    TreeWidgetItem *currentSelection = static_cast<TreeWidgetItem *>(ui->dataTreeWidget->currentItem());
    if (!currentSelection)
        return;

    emit actionCurrent(currentSelection->isinInfo);
}

void DataBrowser::processActionNew(void)
{
    TreeWidgetItem *currentSelection = static_cast<TreeWidgetItem *>(ui->dataTreeWidget->currentItem());
    if (!currentSelection)
        return;

    emit actionNew(currentSelection->isinInfo);
}

void DataBrowser::processActionComposition(void)
{

}

void DataBrowser::showContextMenu(const QPoint &pos)
{
    TreeWidgetItem *item = static_cast<TreeWidgetItem *>(ui->dataTreeWidget->itemAt(pos));
    if (!item)
        return;

    if (item->text(0) == tr("Indices") || item->text(0) == tr("Valuta") || item->text(0) == tr("Grondstoffen"))
        return;

    QMenu menu(tr("Context menu"), this);
    QAction *actionCurrent = new QAction(tr("&Toon in de huidige tab"), &menu) ;
    connect(actionCurrent, SIGNAL(triggered()), this, SLOT(processActionCurrent()));
    menu.addAction(actionCurrent);

    QAction *actionNew = new QAction(tr("&Toon in een nieuwe tab"), &menu);
    connect(actionNew, SIGNAL(triggered()), this, SLOT(processActionNew()));
    menu.addAction(actionNew);

    if (item->isinInfo.type == "INDICE" || item->text(0) == tr("Indices") || item->text(0) == tr("Valuta") || item->text(0) == tr("Grondstoffen"))
    {
        QAction *actionTable = new QAction(tr("&Toon in een nieuwe tab"), &menu);
        connect(actionTable, SIGNAL(triggered()), this, SLOT(processActionComposition()));
        menu.addAction(actionTable) ;
    }

    menu.exec(ui->dataTreeWidget->viewport()->mapToGlobal(pos));
}
