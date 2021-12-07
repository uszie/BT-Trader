#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"
#include <QMainWindow>

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
        Q_OBJECT

    public:
        explicit MainWindow(QWidget *parent = 0);
        ~MainWindow();

        Ui::MainWindow *ui;

    public slots:
        void toggleWindowNormalMaximized(void);

    protected:
        virtual bool eventFilter(QObject *object, QEvent *event);

    private:
        class PrivateData;
        PrivateData *privateData;
};

#endif // MAINWINDOW_H
