#ifndef AUTOOPTIMIZERWORKER_H
#define AUTOOPTIMIZERWORKER_H

#include <QObject>

class AutoOptimizer;
class AutoOptimizerOption;
class TradeResults;
class QVariant;

class AutoOptimizerWorker : public QObject
{
        Q_OBJECT

    public:
        AutoOptimizerWorker(AutoOptimizer *autoOptimizer);
        virtual ~AutoOptimizerWorker(void);

        void setTaskStartPoint(const QVariant &start);
        void setTaskEndPoint(const QVariant &end);
        float progressRate(void);
        void setTotalAvailableOptions(qulonglong count);
        bool isRunning(void) const;
        void setNewTradeResultsAvailable(bool available);

    public slots:
        void start(void);
        void stop(void);
        void pause(bool value);

    signals:
        void result(const TradeResults &tradeResults);
        void finished(void);

    private:
        QList<QVariant> settings(void) const;
        QString optionsSettingsToString(void) const;
        void collectSessionInfo(void);
        void updateSessionInfo(bool updateDeadStrategyCutOffPoint);
        bool newTradeResultsAvailable(void);

        friend class AutoOptimizer;
        class PrivateData;
        PrivateData *privateData;
};

#endif // AUTOOPTIMIZERWORKER_H
