#include "simulationproperties.h"
#include "ui_simulationproperties.h"
#include <QDebug>

class SimulationProperties::PrivateData
{
    public:
        Ui::SimulationProperties *ui;
};

SimulationProperties::SimulationProperties(QWidget *parent) :
    QWidget(parent)
{
    privateData = new PrivateData;
    privateData->ui = new Ui::SimulationProperties;
    privateData->ui->setupUi(this);

    connect(privateData->ui->modeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateGUI()));
    connect(privateData->ui->startCapitalDoubleSpinBox, SIGNAL(valueChanged(double)), this, SIGNAL(propertiesChanged()));
    connect(privateData->ui->modeComboBox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(propertiesChanged()));
    connect(privateData->ui->fillAverageSpreadToCheckBox, SIGNAL(stateChanged(int)), this, SIGNAL(propertiesChanged()));
    connect(privateData->ui->fixedSpreadCheckBox, SIGNAL(stateChanged(int)), this, SIGNAL(propertiesChanged()));
    connect(privateData->ui->pipSpreadDoubleSpinBox, SIGNAL(valueChanged(double)), this, SIGNAL(propertiesChanged()));
    connect(privateData->ui->valutaSpreadDoubleSpinBox, SIGNAL(valueChanged(double)), this, SIGNAL(propertiesChanged()));
    connect(privateData->ui->variableTransactionCostsDoubleSpinBox, SIGNAL(valueChanged(double)), this, SIGNAL(propertiesChanged()));
    connect(privateData->ui->fixedTransactionCostsDoubleSpinBox, SIGNAL(valueChanged(double)), this, SIGNAL(propertiesChanged()));
    connect(privateData->ui->minimalTransactionCostsDoubleSpinBox, SIGNAL(valueChanged(double)), this, SIGNAL(propertiesChanged()));
    connect(privateData->ui->maximumTransactionCostsDoubleSpinBox, SIGNAL(valueChanged(double)), this, SIGNAL(propertiesChanged()));

    loadDefaults();
    updateGUI();
}

SimulationProperties::~SimulationProperties()
{
    delete privateData->ui;
    delete privateData;
}

SimulationPropertiesData SimulationProperties::properties(void) const
{
    SimulationPropertiesData data;
    data.startCapital = startCapital();
    data.transactionCostsMode = transactionCostsMode();
    data.spreadMode = spreadMode();
    data.spread = spread();
    data.variableTransactionCosts = variableTransactionCosts();
    data.fixedTransactionCosts = fixedTransactionCosts();
    data.minimumTransactionCosts = minimumTransactionCosts();
    data.maximumTransactionCosts = maximumTransactionCosts();

    return data;
}

void SimulationProperties::setProperties(SimulationPropertiesData &data)
{
    setStartCapital(data.startCapital);
    setTransactionCostsMode(data.transactionCostsMode);
    setSpreadMode(data.spreadMode);
    setSpread(data.spread);
    setVariableTransactionsCost(data.variableTransactionCosts);
    setFixedTransactionCosts(data.fixedTransactionCosts);
    setMinimumTransactionCosts(data.minimumTransactionCosts);
    setMaximumTransactionCosts(data.maximumTransactionCosts);
}

float SimulationProperties::startCapital() const
{
    return privateData->ui->startCapitalDoubleSpinBox->value();
}

void SimulationProperties::setStartCapital(float capital)
{
    privateData->ui->startCapitalDoubleSpinBox->setValue(static_cast<double>(capital));
}

SimulationProperties::TransactionCostsMode SimulationProperties::transactionCostsMode(void) const
{
    return (privateData->ui->modeComboBox->currentIndex() == 0) ? SimulationProperties::Pips : SimulationProperties::Currency;
}

void SimulationProperties::setTransactionCostsMode(SimulationProperties::TransactionCostsMode mode)
{
    int index = 0;
    if (mode == SimulationProperties::Currency)
        index = 1;

    privateData->ui->modeComboBox->setCurrentIndex(index);
}

SimulationProperties::SpreadMode SimulationProperties::spreadMode(void) const
{
    return privateData->ui->fillAverageSpreadToCheckBox->isChecked() ? SimulationProperties::Fill : SimulationProperties::Fixed;
}

void SimulationProperties::setSpreadMode(SimulationProperties::SpreadMode mode)
{
    bool checked = false;
    if (mode == SimulationProperties::Fill)
        checked = true;

    privateData->ui->fillAverageSpreadToCheckBox->setChecked(checked);
    privateData->ui->fixedSpreadCheckBox->setChecked(!checked);
}

float SimulationProperties::spread(void) const
{
    if (transactionCostsMode() == SimulationProperties::Pips)
        return static_cast<float>(privateData->ui->pipSpreadDoubleSpinBox->value());

    return static_cast<float>(privateData->ui->valutaSpreadDoubleSpinBox->value());
}

void SimulationProperties::setSpread(float spread)
{
    if (transactionCostsMode() == SimulationProperties::Pips)
        privateData->ui->pipSpreadDoubleSpinBox->setValue(static_cast<double>(spread));
    else
        privateData->ui->valutaSpreadDoubleSpinBox->setValue(static_cast<double>(spread));
}

float SimulationProperties::variableTransactionCosts(void) const
{
    if (privateData->ui->modeComboBox->currentIndex() == 0)
        return 0.0f;

    return static_cast<float>(privateData->ui->variableTransactionCostsDoubleSpinBox->value());
}

void SimulationProperties::setVariableTransactionsCost(float costs)
{
    privateData->ui->variableTransactionCostsDoubleSpinBox->setValue(static_cast<double>(costs));
}

float SimulationProperties::fixedTransactionCosts(void) const
{
    if (privateData->ui->modeComboBox->currentIndex() == 0)
        return 0.0f;

    return static_cast<float>(privateData->ui->fixedTransactionCostsDoubleSpinBox->value());
}

void SimulationProperties::setFixedTransactionCosts(float costs)
{
    privateData->ui->fixedTransactionCostsDoubleSpinBox->setValue(static_cast<double>(costs));
}

float SimulationProperties::minimumTransactionCosts(void) const
{
    if (privateData->ui->modeComboBox->currentIndex() == 0)
        return 0.0f;

    return static_cast<float>(privateData->ui->minimalTransactionCostsDoubleSpinBox->value());
}

void SimulationProperties::setMinimumTransactionCosts(float costs)
{
    privateData->ui->minimalTransactionCostsDoubleSpinBox->setValue(static_cast<double>(costs));
}

float SimulationProperties::maximumTransactionCosts(void) const
{
    if (transactionCostsMode() == SimulationProperties::Pips)
        return FLT_MAX;

    if (qFuzzyIsNull(privateData->ui->maximumTransactionCostsDoubleSpinBox->value()))
        return FLT_MAX;

    return static_cast<float>(privateData->ui->maximumTransactionCostsDoubleSpinBox->value());
}

void SimulationProperties::setMaximumTransactionCosts(float costs)
{
    if (costs == FLT_MAX)
        costs = 0.0f;

    privateData->ui->maximumTransactionCostsDoubleSpinBox->setValue(static_cast<double>(costs));
}

void SimulationProperties::updateGUI(void)
{
    bool pipMode = true;
    if (transactionCostsMode() == SimulationProperties::Currency)
        pipMode = false;

    privateData->ui->pipSpreadLabel->setVisible(pipMode);
    privateData->ui->pipSpreadDoubleSpinBox->setVisible(pipMode);
    privateData->ui->valutaSpreadLabel->setVisible(!pipMode);
    privateData->ui->valutaSpreadDoubleSpinBox->setVisible(!pipMode);
    privateData->ui->variableTransactionCostsLabel->setVisible(!pipMode);
    privateData->ui->variableTransactionCostsDoubleSpinBox->setVisible(!pipMode);
    privateData->ui->fixedTransactionCostsLabel->setVisible(!pipMode);
    privateData->ui->fixedTransactionCostsDoubleSpinBox->setVisible(!pipMode);
    privateData->ui->minimalTransactionCostsLabel->setVisible(!pipMode);
    privateData->ui->minimalTransactionCostsDoubleSpinBox->setVisible(!pipMode);
    privateData->ui->maximumTransactionCostsLabel->setVisible(!pipMode);
    privateData->ui->maximumTransactionCostsDoubleSpinBox->setVisible(!pipMode);
}

void SimulationProperties::loadDefaults()
{
    privateData->ui->startCapitalDoubleSpinBox->setValue(10000.0);
    privateData->ui->modeComboBox->setCurrentIndex(0);
    privateData->ui->fillAverageSpreadToCheckBox->setChecked(true);
    privateData->ui->pipSpreadDoubleSpinBox->setValue(2.0);
    privateData->ui->valutaSpreadDoubleSpinBox->setValue(0.0);
    privateData->ui->variableTransactionCostsDoubleSpinBox->setValue(0.0);
    privateData->ui->fixedTransactionCostsDoubleSpinBox->setValue(0.0);
    privateData->ui->minimalTransactionCostsDoubleSpinBox->setValue(0.0);
    privateData->ui->maximumTransactionCostsDoubleSpinBox->setValue(0.0);
}

QDataStream &operator<<(QDataStream &stream, const SimulationPropertiesData &data)
{
    stream << data.startCapital << data.transactionCostsMode << data.spreadMode << data.spread << data.variableTransactionCosts << data.fixedTransactionCosts << data.minimumTransactionCosts << data.maximumTransactionCosts;

    return stream;
}


QDataStream &operator>>(QDataStream &stream, SimulationPropertiesData &data)
{
    int transactionCostsMode;
    int spreadMode;
    stream >> data.startCapital >> transactionCostsMode >> spreadMode >> data.spread >> data.variableTransactionCosts >> data.fixedTransactionCosts >> data.minimumTransactionCosts >> data.maximumTransactionCosts;
    data.transactionCostsMode = static_cast<SimulationProperties::TransactionCostsMode>(transactionCostsMode);
    data.spreadMode = static_cast<SimulationProperties::SpreadMode>(spreadMode);

    return stream;
}
