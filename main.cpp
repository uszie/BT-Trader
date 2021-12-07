#include "bttrader.h"
#include "plot.h"
#include "indicator.h"
#include "main.h"
#include "rule.h"
#include "takeprofitoptions.h"
#include "stoplossoptions.h"
#include "simulationproperties.h"
#include <QApplication>
#include <QLayout>
#include <QLabel>
#include <QPainter>
#include <QDateEdit>
#include <QPushButton>
#include <QMenuBar>
#include <QDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QCalendarWidget>
#include <QDir>
#include <QToolButton>
#include <QStylePainter>
#include <QFile>
#include <QTextStream>
#include <QColorDialog>
#include <qwt_plot_layout.h>
#include <qwt_plot_curve.h>
#include <qwt_scale_draw.h>
#include <qwt_scale_widget.h>
#include <qwt_legend.h>
#include <qwt_legend_item.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_svgitem.h>

ColorPool::ColorPool(void)
{
    colorsList.append(Qt::darkMagenta);
    colorsList.append(Qt::darkRed);
    colorsList.append(Qt::darkGreen);
    colorsList.append(Qt::darkCyan);
    colorsList.append(Qt::darkBlue);
    colorsList.append(Qt::darkYellow);
    colorsList.append(Qt::magenta);
    colorsList.append(Qt::red);
    colorsList.append(Qt::green);
    colorsList.append(Qt::cyan);
    colorsList.append(Qt::blue);
    colorsList.append(Qt::yellow);
    colorsList.append(Qt::white);
    colorsList.append(Qt::lightGray);
    colorsList.append(Qt::gray);
    colorsList.append(Qt::darkGray);
    colorsList.append(Qt::black);

    for (int i = 0; i < colorsList.size(); i++)
        colorsUsedList.append(0);
}

const QColor &ColorPool::takeColor(const QColor &color)
{
    int leastUsed = 1000000;
    int cnt = 0;

    if (color != Qt::transparent)
    {
        for (int i = 0; i < colorsList.size(); i++)
        {
            if (colorsList[i] != color)
                continue;

            colorsUsedList[i] += 1;
            break;
        }

        return color;
    }

    for (int i = 0; i < colorsList.size(); i++)
    {
        if (colorsUsedList[i] < leastUsed)
        {
            leastUsed = colorsUsedList[i];
            cnt = i;
        }
    }

    colorsUsedList[cnt] += 1;

    return colorsList[cnt];
}

void ColorPool::putColor(const QColor &color)
{
    for (int i = 0; i < 12; ++i)
    {
        if (color == colorsList[i])
        {
            if (colorsUsedList[i] > 0)
                colorsUsedList[i] -= 1;
        }
    }
}

QList<QColor> ColorPool::listColors(void)
{
    QList<QColor> list;
    list.append(Qt::darkMagenta);
    list.append(Qt::darkRed);
    list.append(Qt::darkGreen);
    list.append(Qt::darkCyan);
    list.append(Qt::darkBlue);
    list.append(Qt::darkYellow);
    list.append(Qt::magenta);
    list.append(Qt::red);
    list.append(Qt::green);
    list.append(Qt::cyan);
    list.append(Qt::blue);
    list.append(Qt::yellow);
    list.append(Qt::white);
    list.append(Qt::lightGray);
    list.append(Qt::gray);
    list.append(Qt::darkGray);
    list.append(Qt::black);

    return list;
}

int ColorPool::indexOfColor(const QColor &color)
{
    QList<QColor> list;
    list.append(Qt::darkMagenta);
    list.append(Qt::darkRed);
    list.append(Qt::darkGreen);
    list.append(Qt::darkCyan);
    list.append(Qt::darkBlue);
    list.append(Qt::darkYellow);
    list.append(Qt::magenta);
    list.append(Qt::red);
    list.append(Qt::green);
    list.append(Qt::cyan);
    list.append(Qt::blue);
    list.append(Qt::yellow);
    list.append(Qt::white);
    list.append(Qt::lightGray);
    list.append(Qt::gray);
    list.append(Qt::darkGray);
    list.append(Qt::black);

    int index;
    for (index = 0; index < list.size(); index++)
    {
        if (list.at(index) == color)
            break;
    }

    if (index >= list.size())
        return 0;

    return index;
}

MainWindow *mainWindow;
DataConnection *dataConnection;
DataBrowser *dataBrowser;
SettingsDialog *settingsDialog;
SimulationProperties *simulationProperties;
QSettings settings("Usarin Inc.", "BT Trader");
QChar euroSign(8364);

class Widget::PrivateData
{
    public:
        QWidget *dockWidget;
        QPropertyAnimation *moveAnimation;
        QStateMachine moveMachine;
        QState moveInState;
        QState moveOutState;
        QPoint startPos;
};

Widget::Widget(QWidget *parent) : QWidget(parent)
{
    privateData = new PrivateData;
    privateData->moveInState.addTransition(this, SIGNAL(animatedMoveOut()), &privateData->moveOutState);
    privateData->moveMachine.addState(&privateData->moveInState);

    privateData->moveOutState.addTransition(this, SIGNAL(animatedMoveIn()), &privateData->moveInState);
    privateData->moveMachine.addState(&privateData->moveOutState);

    privateData->moveAnimation = new QPropertyAnimation(this, "pos");
    privateData->moveAnimation->setDuration(300);
    connect(privateData->moveAnimation, SIGNAL(finished()), this, SLOT(animationFinished()));
    privateData->moveMachine.addDefaultAnimation(privateData->moveAnimation);
    privateData->moveMachine.setInitialState(&privateData->moveInState);
    privateData->moveMachine.start();
}

Widget::~Widget(void)
{
    delete privateData->moveAnimation;
    delete privateData;
}

void Widget::setWidget(QWidget *widget)
{
    privateData->dockWidget->layout()->addWidget(widget);
}

void Widget::setVisible(bool visible)
{
    if (visible)
    {
        QPoint currentPos = pos();
        QWidget::setVisible(false);
        QWidget::setVisible(true);
        privateData->startPos = pos();
        privateData->moveInState.assignProperty(this, "pos", pos());
        move(currentPos);

        emit animatedMoveIn();
    }
    else
    {
        QPoint currentPos = pos();
        QWidget::setVisible(false);
        QWidget::setVisible(true);
        privateData->startPos = QPoint(pos().x(), pos().y() - size().height());
        privateData->moveOutState.assignProperty(this, "pos", QPoint(pos().x(), pos().y() - size().height()));
        move(currentPos);

        emit animatedMoveOut();
    }
}

void Widget::moveEvent(QMoveEvent *moveEvent)
{
    (void) moveEvent;

    QSet<QAbstractState *> set = privateData->moveMachine.configuration();
    if (set.contains(&privateData->moveOutState))
    {
        QPoint difference = privateData->startPos - pos();
        int height = abs(difference.y());
        QRect widgetRect = rect();
        widgetRect.setTop(widgetRect.bottom() - height);
        QRegion mask = QRegion(widgetRect);
        setMask(mask);
    }
    else if (set.contains(&privateData->moveInState))
    {
        QPoint difference = pos() - privateData->startPos;
        int height = abs(difference.y());
        QRect widgetRect = rect();
        widgetRect.setTop(widgetRect.top() + height);
        QRegion mask = QRegion(widgetRect);
        setMask(mask);
    }
    else
        clearMask();

    repaint();
}

void Widget::animationFinished()
{
    QSet<QAbstractState *> set = privateData->moveMachine.configuration();
    if (set.contains(&privateData->moveOutState))
        QWidget::setVisible(false);
}

bool hyperThreadingEnabled;

void cpuID(unsigned i, unsigned int regs[4])
{
#ifdef _WIN32
    __cpuid((int *)regs, (int)i);
#else
    asm volatile
    ("cpuid" : "=a"(regs[0]), "=b"(regs[1]), "=c"(regs[2]), "=d"(regs[3]) : "a"(i), "c"(0));
#endif
}

bool cpuHasHyperThreading(void)
{
    unsigned int regs[4];

    char vendor[13];
    cpuID(0, regs);

    memset(vendor, '\0', 13);
    mempcpy(vendor, regs + 1, sizeof(unsigned int));
    mempcpy(vendor + 4, regs + 3, sizeof(unsigned int));
    mempcpy(vendor + 8, regs + 2, sizeof(unsigned int));

    QString cpuVendor = QString(vendor);
    cpuID(1, regs);

    unsigned int cpuFeatures = regs[3];

    cpuID(1, regs);
    unsigned logical = (regs[1] >> 16) & 0xff;
    unsigned cores = logical;

    if (cpuVendor == "GenuineIntel")
    {
        cpuID(4, regs);
        cores = ((regs[0] >> 26) & 0x3f) + 1;

    }
    else if (cpuVendor == "AuthenticAMD")
    {
        cpuID(0x80000008, regs);
        cores = ((unsigned)(regs[2] & 0xff)) + 1;
    }
    else
        return false;

    bool hyperThreads = cpuFeatures & (1 << 28) && cores < logical;

    return hyperThreads;
}

int main(int argc, char **argv)
{
    QApplication::setGraphicsSystem("raster");
    QApplication a(argc, argv);
    a.setApplicationName("BT Trader");
    a.setOrganizationName("Usarin Heininga");
    a.setOrganizationDomain("heininga.nl");
    QTranslator translator;
    a.installTranslator(&translator);

    hyperThreadingEnabled = cpuHasHyperThreading();

    QSettings paletteSettings(":/recources/palette", QSettings::IniFormat);
    a.setPalette(paletteSettings.value("MainWindow/Palette", a.palette()).value<QPalette>());

    dataConnection = new DataConnection;
    dataConnection->connectToDatabase();

    qRegisterMetaType<SimulationPropertiesData>("SimulationPropertiesData");
    qRegisterMetaTypeStreamOperators<SimulationPropertiesData>("SimulationPropertiesData");
    qRegisterMetaType<PeriodOptions>("PeriodOptions");
    qRegisterMetaTypeStreamOperators<PeriodOptions>("PeriodOptions");
    qRegisterMetaType<MovingAverageOptions>("MovingAverageOptions");
    qRegisterMetaTypeStreamOperators<MovingAverageOptions>("MovingAverageOptions");
    qRegisterMetaType<IsinInfo>("IsinInfo");
    qRegisterMetaTypeStreamOperators<IsinInfo>("IsinInfo");
    qRegisterMetaType<TakeProfitOptions>("TakeProfitOptions");
    qRegisterMetaTypeStreamOperators<TakeProfitOptions>("TakeProfitOptions");
    qRegisterMetaType<StopLossOptions>("StopLossOptions");
    qRegisterMetaTypeStreamOperators<StopLossOptions>("StopLossOptions");
    qRegisterMetaType<Rule *>("Rule");
    qRegisterMetaTypeStreamOperators<Rule *>("Rule");

    mainWindow = new MainWindow;
    mainWindow->setDocumentMode(true);
    mainWindow->setAnimated(false);
    mainWindow->setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    mainWindow->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
    mainWindow->setWindowTitle("BT Trader");

    settingsDialog = new SettingsDialog(mainWindow);

    simulationProperties = new SimulationProperties(mainWindow);
    simulationProperties->setVisible(false);
    SimulationPropertiesData simulationPropertiesData = settings.value("SimulationPropertiesData", qVariantFromValue(SimulationPropertiesData::defaultOptions())).value<SimulationPropertiesData>();
    simulationProperties->setProperties(simulationPropertiesData);

    strategie = new Strategie(mainWindow);
    mainWindow->setCentralWidget(strategie);

    dataBrowser = new DataBrowser;
    dataBrowser->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    dataBrowser->layout()->setSizeConstraint(QLayout::SetMinimumSize);
    QObject::connect(dataBrowser, SIGNAL(actionNew(IsinInfo)), strategie, SLOT(addPriceChart(IsinInfo)));
    QObject::connect(dataBrowser, SIGNAL(actionCurrent(IsinInfo)), strategie, SLOT(replacePriceChart(IsinInfo)));
    mainWindow->ui->dataBrowserDockWidget->setWidget(dataBrowser);
    mainWindow->tabifyDockWidget(mainWindow->ui->quotesDockWidget, mainWindow->ui->dataBrowserDockWidget);

    while (!settings.group().isEmpty())
        settings.endGroup();

    QRect defaultGeometryRect = QApplication::desktop()->screenGeometry();
    defaultGeometryRect.setWidth((defaultGeometryRect.width() / 4) * 3);
    defaultGeometryRect.setHeight((defaultGeometryRect.height() / 4) * 3);
    QRect geometry = settings.value("MainWindow/Geometry", defaultGeometryRect).toRect();
    mainWindow->setGeometry(geometry);
    mainWindow->show();
    a.processEvents();
    QByteArray windowState = settings.value("MainWindow/WindowState").toByteArray();
    mainWindow->restoreState(windowState);

    mainWindow->setUpdatesEnabled(false);

    bool dataBrowserDockFloating = settings.value("MainWindow/DataBrowserDockFloating", false).toBool();
    mainWindow->ui->dataBrowserDockWidget->setFloating(dataBrowserDockFloating);
    bool dataBrowserDockAutoHide = settings.value("MainWindow/DataBrowserDockAutoHide", false).toBool();
    mainWindow->ui->dataBrowserDockWidget->setAutoHide(dataBrowserDockAutoHide);
    bool dataBrowserDockEnabled = settings.value("MainWindow/DataBrowserDockEnabled", true).toBool();
    mainWindow->ui->dataBrowserDockWidget->setVisible(dataBrowserDockEnabled);
    mainWindow->ui->dataBrowserDockWidget->floatWidget()->adjustSize();
    mainWindow->ui->dataBrowserDockWidget->floatWidget()->raise();

    bool quotesDockFloating = settings.value("MainWindow/QuotesDockFloating", true).toBool();
    mainWindow->ui->quotesDockWidget->setFloating(quotesDockFloating);
    bool quotesDockAutoHide = settings.value("MainWindow/QuotesDockAutoHide", false).toBool();
    mainWindow->ui->quotesDockWidget->setAutoHide(quotesDockAutoHide);
    bool quotesDockEnabled = settings.value("MainWindow/QuotesDockEnabled", true).toBool();
    mainWindow->ui->quotesDockWidget->setVisible(quotesDockEnabled);
    mainWindow->ui->quotesDockWidget->floatWidget()->adjustSize();
    mainWindow->ui->quotesDockWidget->floatWidget()->raise();

    bool indicatorsDockFloating = settings.value("MainWindow/IndicatorsDockFloating", true).toBool();
    mainWindow->ui->indicatorDockWidget->setFloating(indicatorsDockFloating);
    bool indicatorsDockAutoHide = settings.value("MainWindow/IndicatorsDockAutoHide", true).toBool();
    mainWindow->ui->indicatorDockWidget->setAutoHide(indicatorsDockAutoHide);
    bool indicatorsDockEnabled = settings.value("MainWindow/IndicatorsDockEnabled", true).toBool();
    mainWindow->ui->indicatorDockWidget->setVisible(indicatorsDockEnabled);
    mainWindow->ui->indicatorDockWidget->floatWidget()->adjustSize();
    mainWindow->ui->indicatorDockWidget->floatWidget()->raise();

    bool transactionsDockFloating = settings.value("MainWindow/TransactionsDockFloating", true).toBool();
    mainWindow->ui->transactionsDockWidget->setFloating(transactionsDockFloating);
    bool transactionsDockAutoHide = settings.value("MainWindow/TransactionsDockAutoHide", true).toBool();
    mainWindow->ui->transactionsDockWidget->setAutoHide(transactionsDockAutoHide);
    bool transactionsDockEnabled = settings.value("MainWindow/TransactionsDockEnabled", true).toBool();
    mainWindow->ui->transactionsDockWidget->setVisible(transactionsDockEnabled);
    mainWindow->ui->transactionsDockWidget->floatWidget()->adjustSize();
    mainWindow->ui->transactionsDockWidget->floatWidget()->raise();

    mainWindow->setUpdatesEnabled(true);

    strategie->restorePriceCharts();
    strategie->updateDockStatus();
    strategie->updateDockSpacing();

    int retval = a.exec();

    while (!settings.group().isEmpty())
        settings.endGroup();

    settings.setValue("MainWindow/QuotesDockEnabled", mainWindow->ui->showQuotesDockButton->isChecked());
    settings.setValue("Mainwindow/QuotesDockFloating", mainWindow->ui->quotesDockWidget->isFloating());
    settings.setValue("MainWindow/QuotesDockAutoHide", mainWindow->ui->quotesDockWidget->autoHide());
    settings.setValue("MainWindow/DataBrowserDockEnabled", mainWindow->ui->showDataBrowserDockButton->isChecked());
    settings.setValue("Mainwindow/DataBrowserDockFloating", mainWindow->ui->dataBrowserDockWidget->isFloating());
    settings.setValue("MainWindow/DataBrowserDockAutoHide", mainWindow->ui->dataBrowserDockWidget->autoHide());
    settings.setValue("MainWindow/IndicatorsDockEnabled", mainWindow->ui->showIndicatorsDockButton->isChecked());
    settings.setValue("Mainwindow/IndicatorsDockFloating", mainWindow->ui->indicatorDockWidget->isFloating());
    settings.setValue("MainWindow/IndicatorsDockAutoHide", mainWindow->ui->indicatorDockWidget->autoHide());
    settings.setValue("MainWindow/TransactionsDockEnabled", mainWindow->ui->showTransactionsDockButton->isChecked());
    settings.setValue("Mainwindow/TransactionsDockFloating", mainWindow->ui->transactionsDockWidget->isFloating());
    settings.setValue("MainWindow/TransactionsDockAutoHide", mainWindow->ui->transactionsDockWidget->autoHide());
    settings.setValue("MainWindow/Geometry", mainWindow->geometry());
    settings.setValue("MainWindow/WindowState", mainWindow->saveState());
    settings.setValue("SimulationPropertiesData", qVariantFromValue(simulationProperties->properties()));
    //    settings.setValue("MainWindow/Palette", a.palette());

    mainWindow->hide();

    delete strategie;
    delete mainWindow;
    dataConnection->disconnectFromDatabase();
    delete dataConnection;

    settings.sync();

    return retval;
}
