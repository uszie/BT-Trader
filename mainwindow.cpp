#include "mainwindow.h"
#include "bttrader.h"
#include "main.h"
#include <QEvent>
#include <QMouseEvent>
#include <QToolButton>

class MainWindow::PrivateData
{
    public:
        bool leftMouseButtonPressed;
        int direction;
        int handleSize;
};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    privateData = new PrivateData;
    privateData->leftMouseButtonPressed = false;
    privateData->direction = -1;
    privateData->handleSize = 4;

    QHBoxLayout *menuBarLayout = new QHBoxLayout;
    menuBarLayout->setContentsMargins(0, 0, 0, 0);
    menuBarLayout->addWidget(ui->menubar);
    menuBarLayout->addStretch(10);

    QToolButton *minimize = new QToolButton;
    QString iconPath = ":/images/minimize.png";
    minimize->setIcon(QIcon(iconPath));
    minimize->setAutoRaise(true);
    minimize->setToolTip(tr("Minimaliseren"));
    minimize->setMaximumWidth(minimize->height());
    connect(minimize, SIGNAL(clicked()), this,  SLOT(showMinimized()));
    menuBarLayout->addWidget(minimize);

    QToolButton *maximize = new QToolButton;
    iconPath.replace("minimize.png", "maximize.png");
    maximize->setIcon(QIcon(iconPath));
    maximize->setAutoRaise(true);
    maximize->setToolTip(tr("Maximaliseren"));
    maximize->setMaximumWidth(maximize->height());
    connect(maximize, SIGNAL(clicked()), this,  SLOT(toggleWindowNormalMaximized()));
    menuBarLayout->addWidget(maximize);

    QToolButton *close = new QToolButton;
    iconPath.replace("maximize.png", "close.png");
    close->setIcon(QIcon(iconPath));
    close->setAutoRaise(true);
    close->setToolTip(tr("Sluiten"));
    close->setMaximumWidth(close->height());
    connect(close, SIGNAL(clicked()), qApp,  SLOT(quit()));
    menuBarLayout->addWidget(close);

    QWidget *menuBarWidget = new QWidget;
    menuBarWidget->setLayout(menuBarLayout);
    setMenuWidget(menuBarWidget);

    ui->showQuotesDockButton->setOrientation(Qt::Vertical);
    ui->showQuotesDockButton->setMirrored(true);
    connect(ui->showQuotesDockButton, SIGNAL(clicked(bool)), ui->quotesDockWidget, SLOT(slideInOut(bool)));
    connect(ui->quotesDockWidget, SIGNAL(dockVisibilityChanged(bool)), ui->showQuotesDockButton, SLOT(setChecked(bool)));

    ui->showDataBrowserDockButton->setOrientation(Qt::Vertical);
    ui->showDataBrowserDockButton->setMirrored(true);
    connect(ui->showDataBrowserDockButton, SIGNAL(clicked(bool)), ui->dataBrowserDockWidget, SLOT(slideInOut(bool)));
    connect(ui->dataBrowserDockWidget, SIGNAL(dockVisibilityChanged(bool)), ui->showDataBrowserDockButton, SLOT(setChecked(bool)));

    ui->showIndicatorsDockButton->setOrientation(Qt::Vertical);
    ui->showIndicatorsDockButton->setMirrored(true);
    connect(ui->showIndicatorsDockButton, SIGNAL(clicked(bool)), ui->indicatorDockWidget, SLOT(slideInOut(bool)));
    connect(ui->indicatorDockWidget, SIGNAL(dockVisibilityChanged(bool)), ui->showIndicatorsDockButton, SLOT(setChecked(bool)));

    ui->showTransactionsDockButton->setOrientation(Qt::Vertical);
    ui->showTransactionsDockButton->setMirrored(true);
    connect(ui->showTransactionsDockButton, SIGNAL(clicked(bool)), ui->transactionsDockWidget, SLOT(slideInOut(bool)));
    connect(ui->transactionsDockWidget, SIGNAL(dockVisibilityChanged(bool)), ui->showTransactionsDockButton, SLOT(setChecked(bool)));

    ui->toolBar->addWidget(ui->buttonWidget);
    addToolBar(Qt::LeftToolBarArea, ui->toolBar);

    installEventFilter(this);
    setWindowFlags(Qt::CustomizeWindowHint);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::toggleWindowNormalMaximized()
{
    if (windowState() == Qt::WindowMaximized)
        showNormal();
    else
        showMaximized();
}

bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
    if (!object->inherits("MainWindow"))
        return QObject::eventFilter(object, event);

    if (event->type() == QEvent::MouseButtonRelease)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton)
            privateData->leftMouseButtonPressed = false;
    }
    else if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton)
            privateData->leftMouseButtonPressed = true;
    }
    else if (event->type() == QEvent::HoverLeave)
    {
        if (cursor().shape() != Qt::ArrowCursor)
            setCursor(Qt::ArrowCursor);
        privateData->direction = -1;
        privateData->leftMouseButtonPressed = false;
    }
    else if (event->type() == QEvent::HoverMove)
    {
        QHoverEvent *hoverEvent = static_cast<QHoverEvent *>(event);

        if (privateData->leftMouseButtonPressed && privateData->direction != -1)
        {
            QRect geometry;
            switch (privateData->direction)
            {
                case 0:
                    geometry = frameGeometry();
                    geometry.setTopLeft(mapToGlobal(hoverEvent->pos()));
                    setGeometry(geometry);
                    break;

                case 1:
                    geometry = frameGeometry();
                    geometry.setTop(mapToGlobal(hoverEvent->pos()).y());
                    setGeometry(geometry);
                    break;

                case 2:
                    geometry = frameGeometry();
                    geometry.setTopRight(mapToGlobal(hoverEvent->pos()));
                    setGeometry(geometry);
                    break;

                case 3:
                    geometry = frameGeometry();
                    geometry.setRight(mapToGlobal(hoverEvent->pos()).x());
                    setGeometry(geometry);
                    break;

                case 4:
                    geometry = frameGeometry();
                    geometry.setBottomRight(mapToGlobal(hoverEvent->pos()));
                    setGeometry(geometry);
                    break;

                case 5:
                    geometry = frameGeometry();
                    geometry.setBottom(mapToGlobal(hoverEvent->pos()).y());
                    setGeometry(geometry);
                    break;

                case 6:
                    geometry = frameGeometry();
                    geometry.setBottomLeft(mapToGlobal(hoverEvent->pos()));
                    setGeometry(geometry);
                    break;

                case 7:
                    geometry = frameGeometry();
                    geometry.setLeft(mapToGlobal(hoverEvent->pos()).x());
                    setGeometry(geometry);
                    break;

                default:
                    break;
            }
        }
        else if (hoverEvent->pos().y() <= rect().top() + privateData->handleSize &&
                 hoverEvent->pos().x() <= rect().left() + privateData->handleSize)
        {
            setCursor(Qt::SizeFDiagCursor);
            privateData->direction = 0;
        }
        else if (hoverEvent->pos().y() >= rect().bottom() - privateData->handleSize &&
                 hoverEvent->pos().x() <= rect().left() + privateData->handleSize)
        {
            setCursor(Qt::SizeBDiagCursor);
            privateData->direction = 6;
        }
        else if (hoverEvent->pos().y() >= rect().bottom() - privateData->handleSize &&
                 hoverEvent->pos().x() >= rect().right() - privateData->handleSize)
        {
            setCursor(Qt::SizeFDiagCursor);
            privateData->direction = 4;
        }
        else if (hoverEvent->pos().y() <= rect().top() + privateData->handleSize &&
                 hoverEvent->pos().x() >= rect().right() - privateData->handleSize)
        {
            setCursor(Qt::SizeBDiagCursor);
            privateData->direction = 2;
        }
        else if (hoverEvent->pos().x() <= rect().left() + privateData->handleSize)
        {
            setCursor(Qt::SizeHorCursor);
            privateData->direction = 7;
        }
        else if (hoverEvent->pos().x() >= rect().right() - privateData->handleSize)
        {
            setCursor(Qt::SizeHorCursor);
            privateData->direction = 3;
        }
        else if (hoverEvent->pos().y() <= rect().top() + privateData->handleSize)
        {
            setCursor(Qt::SizeVerCursor);
            privateData->direction = 1;
        }
        else if (hoverEvent->pos().y() >= rect().bottom() - privateData->handleSize)
        {
            setCursor(Qt::SizeVerCursor);
            privateData->direction = 5;
        }
        else
        {
            if (cursor().shape() != Qt::ArrowCursor)
                setCursor(Qt::ArrowCursor);
            privateData->direction = -1;
            privateData->leftMouseButtonPressed = false;
        }
    }

    return QObject::eventFilter(object, event);
}
