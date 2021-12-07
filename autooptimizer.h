#ifndef AUTOOPTIMIZER_H
#define AUTOOPTIMIZER_H

#include "bttrader.h"
#include "autooptimizeroption.h"
#include "plot.h"
#include <qdialog.h>
#include <qtablewidget.h>

class Indicator;

class TradeResults
{
    public:
        TradeResults(void)
        {
            startCapital = 0.0f;
            finalCapital = 0.0f;
            spreadCosts = 0.0f;
            transactionCosts = 0.0f;
            profits = 0.0f;
            relativeProfits = 0.0f;
            successRate = 0.0f;
            totalTransactions = 0;
            totalTrades = 0;
            totalProfitTrades = 0;
            totalLossTrades = 0;
            ignore = false;
        }

        float startCapital;
        float finalCapital;
        float spreadCosts;
        float transactionCosts;
        float profits;
        float relativeProfits;
        float successRate;
        int totalTransactions;
        int totalTrades;
        int totalProfitTrades;
        int totalLossTrades;
        bool ignore;
        QList<QVariant> settings;
        QString configuration;
};

class IndicatorReference
{
    public:
        IndicatorReference(Indicator *indicator)
        {
            this->indicator = indicator;
            referenceCount = 1;
        }

        int referenceCount;
        Indicator *indicator;
};

class SessionInfo
{
    public:
        int maximumResultsVisible;
        bool showDoubleResults;
        bool showNegativeResults;
        float deadStrategyCutOffRate;
        int deadStrategyCutOffPoint;
        float maximumAllowedCapitalLoss;
        int minimumRequieredTrades;
        float minimumRequieredSuccessRate;
        int tradeResultsCount;
        TradeResults bestTradeResults;
        TradeResults worstTradeResults;
        QVector<QPointF> maximumProfitsMap;
};

class AutoOptimizer : public QDialog
{
        Q_OBJECT

    public:
        AutoOptimizer(Indicator *indicator, Qt::WindowFlags f = 0);
        ~AutoOptimizer(void);

        void addIndicator(Indicator *indicator);
        void removeIndicator(Indicator *indicator);
        Indicator *indicator(void) const;
        QList<Indicator *> indicators(void) const;
        QList<AutoOptimizerOption *> listOptions(AutoOptimizerOption::optionTypes type = AutoOptimizerOption::IndicatorOption) const;
        AutoOptimizerOption *addOption(Indicator *indicator, const QString &title, bool *option, int min = 0, int max = 1, AutoOptimizerOption::optionTypes type = AutoOptimizerOption::IndicatorOption, const QStringList &valueTitles = QStringList());
        AutoOptimizerOption *addOption(Indicator *indicator, const QString &title, int *option, int min, int max, int stepSize = 1, bool useAccelleration = true, AutoOptimizerOption::optionTypes type = AutoOptimizerOption::IndicatorOption, const QStringList &valueTitles = QStringList());
        AutoOptimizerOption *addOption(Indicator *indicator, const QString &title, float *option, float min, float max, float stepSize = 1.0, bool useAccelleration = true, bool percentualNotation = false, AutoOptimizerOption::optionTypes type = AutoOptimizerOption::IndicatorOption);
        AutoOptimizerOption *addOption(Indicator *indicator, const QString &title, float *option, float min, int periods, bool useAccelleration = true, bool percentualNotation = false, AutoOptimizerOption::optionTypes type = AutoOptimizerOption::IndicatorOption);
        void removeOption(AutoOptimizerOption *option);
        void clearResults(void);
        qulonglong countTotalAvailableOptions(void) const;
        qulonglong countTotalAvailableOptions(QList<AutoOptimizerOption *> &optionsList) const;
        const QList<QVariant> saveSettings(void) const;
        void loadSettings(const QList<QVariant> &settings) const;
        void setAllIndicatorCurvesToProcessMode(PlotCurve::ProcessMode type);
        TradeResults tradeResultsItemAt(int i) const;
        int tradeResultsCount(void) const;
        void lockTradeResultsTable(bool write = false) const;
        void unlockTradeResultsTable(void) const;
        SessionInfo &sessionInfo(void) const;
        void resortRuleLists(void);
        bool isRunning(void) const;
        void pause(void) const;
        void resume(void) const;
        void lockProgressMutex(void) const;
        void unlockProgressMutex(void) const;

    public slots:
        void testSlot(void);
        void start(void);
        void stop(void);
        void finished(void);
        void quit(void);
        void updateGui(void);
        void updateThreadPriorities(void);
        void updateProgressBar(void);
        void plotSelectedResultEntry(int row, int column);
        float totalAvailableOptions(void) const;
        void increaseProgressValue(qulonglong = 1);
        void addResult(const TradeResults &tradeResults);
        int maximumResultsVisible(void) const;
        void setMaximumResultsVisible(int count);
        bool showDoubleResults(void) const;
        void setShowDoubleResults(bool enabled);
        bool showNegativeResults(void) const;
        void setShowNegativeResults(bool enabled);
        float deadStrategyCutOffRate(void) const;
        void setDeadStrategyCutOffRate(float rate);
        float maximumAllowedCapitalLoss(void) const;
        void setMaximumAllowedCapitalLoss(float rate);
        int minimumRequieredTrades(void) const;
        void setMinimumRequieredTrades(int trades);
        float minimumRequieredSuccessRate(void) const;
        void setMinimumRequieredSuccessRate(float rate);
        virtual void setVisible(bool visible);
        void setColor(const QColor &color);

    private:
        void addOptionToInterface(AutoOptimizerOption *option);
        void removeOptionFromInterface(AutoOptimizerOption *option);
        void setOptionsToMinimumValue(const QList<AutoOptimizerOption *> &list) const;
        bool moveDependenciesForward(const AutoOptimizerOption *option);
        void resortOptionList(void);
        void startProcessing(void);
        void stopProcessing(void);
        void processOptions(void);
        void launchWorkerThreads(void);
        void clearResultsWidget();
        class PrivateData;
        PrivateData *privateData;

    private slots:
        void fillResultsWidget();
};

#endif // AUTOOPTIMIZER_H
