#ifndef PERIODSCONFIGUI_H
#define PERIODSCONFIGUI_H

#include <QObject>
#include <QMetaType>
#include <QWidget>

class Indicator;

class PeriodOptions
{
    public:
        virtual ~PeriodOptions(void) {;}
        virtual void registerWithAutoOptimizer(Indicator *indicator, const QString &title = "", bool cycleAdativeEnabled = false, int aoMinPE = 1, int aoMaxPE = 100);
        virtual QString createConfigKey(void) const;
        static PeriodOptions defaultOptions(int periods = 20)
        {
            PeriodOptions options;
            options.periods = periods;
            options.fastSmoothing = 2;
            options.slowSmoothing = 30;
            options.minCyclePeriods = 3;
            options.maxCyclePeriods = 30;
            options.amplificationFactor = 1.0;
            options.cycleAdativeEnabled = false;
            options.trendAdaptiveEnabled = false;
            options.useHalfCycle = true;

            return options;
        }

        int periods;
        int fastSmoothing;
        int slowSmoothing;
        int minCyclePeriods;
        int maxCyclePeriods;
        int amplificationFactor;
        bool cycleAdativeEnabled;
        bool trendAdaptiveEnabled;
        bool useHalfCycle;
};

Q_DECLARE_METATYPE(PeriodOptions)
QDataStream &operator<<(QDataStream &stream, const PeriodOptions &options);
QDataStream &operator>>(QDataStream &stream, PeriodOptions &options);

class PeriodsUI: public QWidget
{
        Q_OBJECT

    public:
        PeriodsUI(QWidget *parent = 0, const QString &title = "", Qt::WindowFlags f = 0);
        virtual ~PeriodsUI();

        int periods(void);
        void setPeriods(int periods);
        bool cycleAdative(void);
        void setCycleAdaptive(bool enabled);
        int minCycleAdaptive(void);
        void setMinCycleAdaptive(int periods);
        int maxCycleAdaptive(void);
        void setMaxCycleAdaptive(int periods);
        bool adaptiveInTrend(void);
        void setAdaptiveInTrend(bool enabled);
        double amplificationFactor(void);
        void setAmplificationFactor(double amplificationFactor);
        virtual void options(PeriodOptions &options);
        virtual void setOptions(PeriodOptions &options);
        void setCycleAdaptiveButtonEnabled(bool enabled);
        void setPeriodsSpinboxEnabled(bool enabled);
        void setUseHalfCycle(bool enabled);

    public slots:
        void slotPropertiesChanged(void);
        void showCycleAdaptiveButton(bool show);

    signals:
        void propertiesChanged(void);
        void overloadedPropertiesChanged(void);

    private:
        class PrivateData;
        PrivateData *privateData;
};

#endif // PERIODSCONFIGUI_H
