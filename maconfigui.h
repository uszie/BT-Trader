#ifndef MACONFIGUI_H
#define MACONFIGUI_H

#include "periodsconfigui.h"

extern const char *movingAverageTitleList[ 11 ];

enum MAType
{
    Simple,
    Exponential,
    LinearWeighted,
    Adaptive,
    Hull,
    //    Jurik,
    T3,
    GDEMA,
    ZLEMA,
    MAMA,
    FAMA
};

class MovingAverageOptions : public PeriodOptions
{
    public:
        virtual ~MovingAverageOptions(void) {;}
        virtual void registerWithAutoOptimizer(Indicator *indicator, const QString &title = "", bool cycleAdativeEnabled = false, int aoMinPE = 1, int aoMaxPE = 100, int aoMinMA = 0, int aoMaxMA = 4, int aoMinFS = 2, int aoMaxFS = 2, int aoMinSS = 30, int aoMaxSS = 30/*, int aoMinMinCP, int aoMaxMinCP, int aoMinMaxCP, int aoMaxMaxCP*/);
        virtual QString createConfigKey(void) const;
        static MovingAverageOptions defaultOptions(int periods = 20)
        {
            MovingAverageOptions options;
            options.type = Simple;
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

        int type;
        int fastSmoothing;
        int slowSmoothing;
};

Q_DECLARE_METATYPE(MovingAverageOptions)
QDataStream &operator<<(QDataStream &stream, const MovingAverageOptions &options);
QDataStream &operator>>(QDataStream &stream, MovingAverageOptions &options);

class MovingAverageUI : public PeriodsUI
{
        Q_OBJECT

    public:
        MovingAverageUI(QWidget *parent = 0, const QString &title = "", Qt::WindowFlags f = 0);
        virtual ~MovingAverageUI();

        int maType(void);
        void setMaType(int type);
        int fastSmoothing(void);
        void setFastSmoothing(int fastSmoothing);
        int slowSmoothing(void);
        void setSlowSmoothing(int slowSmoothing);
        void setSmoothingBoxesEnabled(bool enabled);
        QString maTitle(bool shortTitle = true);
        virtual void options(MovingAverageOptions &options);
        virtual void setOptions(MovingAverageOptions &options);

    public slots:
        void slotPropertiesChanged(void);
        void slotOverloadedPropertiesChanged(void);

    signals:
        void propertiesChanged(void);

    private:
        class PrivateData;
        PrivateData *privateData;
};

#endif // MACONFIGUI_H
