#ifndef SIMULATIONPROPERTIES_H
#define SIMULATIONPROPERTIES_H

#include <QWidget>
#include <QMetaType>
#include <float.h>

namespace Ui
{
class SimulationProperties;
}

class SimulationPropertiesData;

class SimulationProperties : public QWidget
{
        Q_OBJECT

    public:
        enum TransactionCostsMode
        {
            Pips,
            Currency
        };

        enum SpreadMode
        {
            Fill,
            Fixed
        };

        explicit SimulationProperties(QWidget *parent = 0);
        ~SimulationProperties();

        SimulationPropertiesData properties(void) const;
        void setProperties(SimulationPropertiesData &data);
        float startCapital(void) const;
        void setStartCapital(float capital);
        SimulationProperties::TransactionCostsMode transactionCostsMode(void) const;
        void setTransactionCostsMode(SimulationProperties::TransactionCostsMode mode);
        SimulationProperties::SpreadMode spreadMode(void) const;
        void setSpreadMode(SimulationProperties::SpreadMode mode);
        float spread(void) const;
        void setSpread(float spread);
        float variableTransactionCosts(void) const;
        void setVariableTransactionsCost(float costs);
        float fixedTransactionCosts(void) const;
        void setFixedTransactionCosts(float costs);
        float minimumTransactionCosts(void) const;
        void setMinimumTransactionCosts(float costs);
        float maximumTransactionCosts(void) const;
        void setMaximumTransactionCosts(float costs);

    signals:
        void propertiesChanged(void);

    public slots:
        void loadDefaults(void);

    private slots:
        void updateGUI(void);

    private:
        class PrivateData;
        PrivateData *privateData;
};

class SimulationPropertiesData
{
    public:
        static SimulationPropertiesData defaultOptions(void)
        {
            SimulationPropertiesData data;
            data.startCapital = 10000.0f;
            data.transactionCostsMode = SimulationProperties::Pips;
            data.spreadMode = SimulationProperties::Fill;
            data.spread = 2.0f;
            data.variableTransactionCosts = 0.0f;
            data.fixedTransactionCosts = 0.0f;
            data.minimumTransactionCosts = 0.0f;
            data.maximumTransactionCosts = FLT_MAX;

            return data;
        }

        float startCapital;
        SimulationProperties::TransactionCostsMode transactionCostsMode;
        SimulationProperties::SpreadMode spreadMode;
        float spread;
        float variableTransactionCosts;
        float fixedTransactionCosts;
        float minimumTransactionCosts;
        float maximumTransactionCosts;
};

Q_DECLARE_METATYPE(SimulationPropertiesData)
QDataStream &operator<<(QDataStream &stream, const SimulationPropertiesData &data);
QDataStream &operator>>(QDataStream &stream, SimulationPropertiesData &data);

#endif // SIMULATIONPROPERTIES_H
