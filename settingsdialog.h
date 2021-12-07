#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

namespace Ui
{
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
        Q_OBJECT

    public:
        explicit SettingsDialog(QWidget *parent = 0);
        ~SettingsDialog();

    private slots:
        void switchSettingsViewToRow(int row);
        void restoreDefaults(void);

    private:
        class PrivateData;
        PrivateData *privateData;
};

#endif // SETTINGSDIALOG_H
