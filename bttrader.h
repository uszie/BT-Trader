#ifndef BTTRADER_H
#define BTTRADER_H

#include <QDebug>
#include <QSettings>
#include <iostream>
#include <float.h>
#include <math.h>
#include <omp.h>

#define QT_USE_FAST_CONCATENATION
#define QT_USE_FAST_OPERATOR_PLUS
#define MAX_HISTORY 20000

#define USE_AUTOOPTIMIZER_THREADS 1
#define USE_RULE_RESOLVEMENT_CACHE 1
#define USE_TRADEEND_SIGNAL_CACHE 1
#define USE_CALCULATION_CACHE 1
#define USE_RULE_RESORTING 1
#define USE_DEAD_STRATEGY_SKIPPING 1
#define USE_SUSPECTED_DEAD_STRATEGIE_SKIPPING 1
#define USE_LOOP_JUMPING 1
#define USE_SIMD 0

#define DEFAULT_PERIOD_COUNT 200

inline bool doublesAreSame(double a, double b, double epsilon = 0.0001)
{
    return fabs(a - b) < epsilon;//std::numeric_limits<double>::epsilon();
}

inline float powf_fast(const float a, const float b)
{
    return (a / (b - a * b + a));
}

inline long double powl_fast(const long double a, const long double b)
{
    return (a / (b - a * b + a));
}

typedef float price_t;

#define IGNORE_VALUE -FLT_MAX
#define bt_min( x, y ) ( y ^ ( ( x ^ y ) & - ( x < y ) ) );
#define bt_max( x, y ) ( x ^ ( ( x ^ y ) & - ( x < y ) ) );

enum TransactionType
{
    Short,
    Long,
    Exit,
    Undefined,
    TakeProfits
};

enum Source
{
    Open,
    High,
    Low,
    Close
};

enum TimeScale
{
    TickByTick = 1,
    MinuteByMinute = 2,
    HourByHour = 4,
    DayByDay = 8,
    WeekByWeek = 16,
    MonthByMonth = 32
};

extern QChar euroSign;

#include "intrinsics.h"
#include "mainwindow.h"
#include "dataconnection.h"
#include "settingsdialog.h"
#include "simulationproperties.h"

class MainWindow;
extern MainWindow *mainWindow;
class DataConnection;
extern DataConnection *dataConnection;
class SettingsDialog;
extern SettingsDialog *settingsDialog;
class SimulationProperties;
extern SimulationProperties *simulationProperties;
extern QSettings settings;
extern bool hyperThreadingEnabled;

class QUnifiedTimer : public QObject
{
    public:
        static QUnifiedTimer *instance();
        static QUnifiedTimer *instance(bool create);
        void setTimingInterval(int interval);
        int timingInterval;
};


#endif // BTTRADER_H
