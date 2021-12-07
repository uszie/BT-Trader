#ifndef AUTOOPTIMIZEROPTION_H
#define AUTOOPTIMIZEROPTION_H

#include "bttrader.h"

class Indicator;
class AutoOptimizerOption;

class Dependency
{
    public:
        enum compareType
        {
            IsEqual,
            IsLower,
            IsHigher
        };

        AutoOptimizerOption *dependencyOption;
        AutoOptimizerOption *option;
        QVariant value;
        Dependency::compareType cmpType;
        bool toggleOptionDisabled;
};

class AutoOptimizerOption : public QWidget
{
        Q_OBJECT

    public:
        enum variableTypes
        {
            Boolean,
            Integer,
            Double
        };

        enum optionTypes
        {
            IndicatorOption = 1,
            TradeOption = 2,
            StopLossOption = 4,
            TakeProfitOption = 8,
            RuleOption = 16,
            ExitRuleOption = 32,
            LongRuleOption = 64,
            ShortRuleOption = 128,
            AllOptions = 255
        };

        AutoOptimizerOption(bool *option, Indicator *indicator, const QStringList &valueTitles = QStringList());
        AutoOptimizerOption(int *option, Indicator *indicator, const QStringList &valueTitles = QStringList());
        AutoOptimizerOption(float *option, Indicator *indicator, bool percentualNotation = false);
        virtual ~AutoOptimizerOption();

        void *pointer(void) const;

        void setup(void);
        QString title(void) const;
        void setTitle(const QString &title);
        AutoOptimizerOption::variableTypes varType(void) const;
        AutoOptimizerOption::optionTypes type(void) const;
        void setType(AutoOptimizerOption::optionTypes type);
        QVariant value(int simdAddress = 0) const;
        void setValue(const QVariant &value, int simdAddress = 4);
        QVariant min(void) const;
        void setMin(const QVariant &min);
        QVariant max(void) const;
        void setMax(const QVariant &max);
        bool autoAdjustEnabled() const;
        void setAutoAdjustEnabled(bool enabled);
        int autoAdjustPeriods(void) const;
        void setAutoAdjustPeriods(int periods);
        QVariant stepSize(void) const;
        void setStepSize(const QVariant &stepSize);
        bool useAcceleration(void) const;
        void setUseAcceleration(bool enabled);
        bool useSimd(void) const;
        bool valueCheckBoxEnabled(const QVariant &value) const ;
        void setValueCheckBoxEnabled(const QVariant &value, bool enabled);
        QString suffix(void) const;
        void setSuffix(const QString &suffix);
        QString valueTitle(const QVariant &value) const;
        Indicator *indicator(void) const;
        bool enabled(void) const;
        bool hasGuiItemsEnabled(void) const;
        void setDependenciesChanged(void) const;
        void storeOriginalSetting(void);
        void restoreOriginalSetting(void);
        int possibleOptionsCount(void) const;
        void addDependency(const AutoOptimizerOption *option, Dependency::compareType cmpType = Dependency::IsEqual);
        void addDependency(const AutoOptimizerOption *option, QVariant value, Dependency::compareType cmpType = Dependency::IsEqual, bool toggleDisabled = true);
        void addDependent(const AutoOptimizerOption *option) const;
        bool dependenciesForfilled(void) const;
        bool dependencyForfilled(const Dependency &dependency) const;
        bool hasDependencies(void) const;
        bool dependsOn(const AutoOptimizerOption &option) const;
        bool containsDependency(const AutoOptimizerOption &option, Dependency::compareType cmpType) const;
        QVector<Dependency *> dependencies(void) const;
        bool operator== (const AutoOptimizerOption &other) const;

    private slots:
        void slotRelayUpdateGuiSignal(void) const;
        void updateAutoAdjustRanges(void);

    signals:
        void guiTouched(void) const;
        void valueChanged(const QVariant &newValue);

    private:
        class PrivateData;
        PrivateData *privateData;
};

#endif // AUTOOPTIMIZEROPTION_H
