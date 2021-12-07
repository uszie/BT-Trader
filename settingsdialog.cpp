#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "bttrader.h"
#include <QPushButton>
#include <QListWidgetItem>

class SettingsDialog::PrivateData
{
    public:
        Ui::SettingsDialog *ui;
};

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent)
{
    privateData = new PrivateData;
    privateData->ui = new Ui::SettingsDialog;
    privateData->ui->setupUi(this);
    privateData->ui->settingsWidget->setLayout(new QVBoxLayout);
    privateData->ui->settingsWidget->layout()->setContentsMargins(0, 0, 0, 0);

    connect(privateData->ui->listWidget, SIGNAL(currentRowChanged(int)), this, SLOT(switchSettingsViewToRow(int)));
    connect(privateData->ui->buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(accept()));
    connect(privateData->ui->buttonBox->button(QDialogButtonBox::RestoreDefaults), SIGNAL(clicked()), this, SLOT(restoreDefaults()));
}

SettingsDialog::~SettingsDialog()
{
    delete privateData->ui;
    delete privateData;
}

void SettingsDialog::switchSettingsViewToRow(int row)
{
    if (row == 0)
    {
        QLayoutItem *child;
        while ((child = privateData->ui->settingsWidget->layout()->takeAt(0)))
        {
            if (child->widget())
                child->widget()->setVisible(false);
        }

        privateData->ui->settingsWidget->layout()->addWidget(simulationProperties);
        simulationProperties->setVisible(true);
    }
    else if (row == 1)
    {
        QLayoutItem *child;
        while ((child = privateData->ui->settingsWidget->layout()->takeAt(0)))
        {
            if (child->widget())
                child->widget()->setVisible(false);
        }
    }
    else if (row == 2)
    {
        QLayoutItem *child;
        while ((child = privateData->ui->settingsWidget->layout()->takeAt(0)))
        {
            if (child->widget())
                child->widget()->setVisible(false);
        }
    }
    else
    {
        QLayoutItem *child;
        while ((child = privateData->ui->settingsWidget->layout()->takeAt(0)))
        {
            if (child->widget())
                child->widget()->setVisible(false);
        }
    }
}

void SettingsDialog::restoreDefaults()
{
    int row = privateData->ui->listWidget->currentRow();
    if (row == 0)
        simulationProperties->loadDefaults();
}
