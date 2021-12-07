#ifndef __INDICATOR_H
#define __INDICATOR_H

#include "bttrader.h"
#include "strategie.h"
#include "plot.h"
#include "autooptimizer.h"
#include "maconfigui.h"
#include "rulelist.h"
#include "qwt_raster_data.h"
#include "qwt_plot_spectrogram.h"
#include <QtGui>

extern bool debugEnabled;

class Plot;
class MarkerPoints;
class Indicator;
class PeriodOptions;
class MovingAverageOptions;
class TradeResults;
class AutoOptimizer;
class AutoOptimizerOption;
class Rule;
class RuleList;
class RuleTreeWidgetItem;
class AutoOptimizerWorker;

typedef QVector<double> Array;
typedef QVector<QVector<double> > MultiArray;

//typedef QVector<float> Array;
//typedef QVector<QVector<float> > MultiArray;

enum IndicatorType
{
    MovingAverageIndicator,
    TwoMovingAveragesIndicator,
    ThreeMovingAveragesIndicator,
    IndexTracerIndicator,
    BollingerBandsIndicator,
    PriceChannelIndicator,
    TrendLineIndicator,
    SupportCeilingLineIndicator,
    RSIIndicator,
    StochasticIndicator,
    VolatilityIndicator,
    ERIndicator,
    ADXIndicator,
    TrendIndicator,
    DominantCycleIndicator,
    TrendVigorIndicator,
    SNRIndicator,
    ITrendIndicator
};

enum filter
{
    NoFilter,
    ClosesInRow,
    AbsoluteDifference,
    RelativeDifference
};

enum tradingRules
{
    OnCrossing,
    OnDirectionChange
};

enum compareType
{
    IsAbove,
    IsUnder,
    IsCrossingUp,
    IsCrossingDown,
    IsRising,
    IsFalling
};

enum logicalOperator
{
    AND = 1,
    OR = 2
};

extern ui4vector longMask;
extern ui4vector shortMask;
extern ui4vector exitMask;
extern ui4vector multiplierMask;
extern ui4vector iFullMask;
extern f4vector fNullMask;
extern f4vector fIgnoreValue;
extern f4vector fMinusOne;
extern ui4vector iZero4;
extern i8vector iZero8;
extern f4vector fZero;
extern ui4vector iOne4;
extern f4vector fOne;
extern f4vector fTwo;
extern ui4vector iTen4;
extern f4vector fHundred;
extern ui4vector intMax;

class Filter
{
    public:
        int type;
        i4vector periods;
        f4vector absoluteDifference;
        f4vector relativeDifference;
};

class IndicatorRule : public QObject
{
        Q_OBJECT

    public:
        IndicatorRule(void)
        {
            sourceIndicator = NULL;
            sourceCurve = NULL;
            referenceIndicator = NULL;
            referenceCurve = NULL;
            referenceValue = 0;
            referenceValues = NULL;
            enabled = true;
            offset = 0;
            indicatorCompareType = -1;
            logicalOperator = -1;
            previousTransactionType = -1;
            treeItem = NULL;
            filter = NULL;
            size = 0;
        }

        ~IndicatorRule(void)
        {
            if (treeItem)
                delete treeItem;

            if (filter)
                delete filter;

            if (referenceValues)
                _mm_free(referenceValues);
        }

        Indicator *sourceIndicator;
        Indicator *referenceIndicator;
        const PlotCurve *sourceCurve;
        const PlotCurve *referenceCurve;
        QTreeWidgetItem *treeItem;
        QString title;
        QList <IndicatorRule *> list;
        QList<AutoOptimizerOption *> autoOptimizerOptionList;
        Filter *filter;
        double referenceValue;
        float *referenceValues;
        bool enabled;
        bool container;
        int offset;
        int indicatorCompareType;
        int logicalOperator;
        int previousTransactionType;

    private:
        int size;

    public slots:
        void setReferenceValuesSize(int size)
        {
            if (size != this->size)
            {
                this->size = size;
                if (referenceValues)
                    _mm_free(referenceValues);
                referenceValues = (float *) _mm_malloc(size * sizeof(float), 16);
                updateReferenceValues(referenceValue);
            }
        }

        void updateReferenceValues(const QVariant &value)
        {
            (void) value;
            std::fill(referenceValues, referenceValues + size, (float) referenceValue);
        }
};

class RasterData: public QwtRasterData
{
    public:
        RasterData(const MultiArray &array, const QwtDoubleRect &rect, double colorRange, double arrayYBottom, double arrayYTop);
        ~RasterData(void);

        virtual QwtRasterData *copy() const;
        virtual QwtDoubleInterval range() const;
        virtual double value(double x, double y) const;
        virtual QSize rasterHint(const QwtDoubleRect &rect) const;

    private:
        MultiArray p_array;
        double p_colorRange;
        double p_arrayRange;
        double p_arrayYBottom;
        double p_arrayYTop;
        double p_plotRange;
        double p_plotYBottom;
        double p_plotYTop;
        double p_colorRangeRate;

};

class PlotSpectrogram: public QwtPlotSpectrogram
{
    public:
        PlotSpectrogram(const QString &title = QString::null);
        virtual ~PlotSpectrogram();
        virtual QImage renderImage(const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QwtDoubleRect &rect) const;
        QImage testFunc(const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QwtDoubleRect &rect) const;
};

class Indicator : public QWidget
{
        Q_OBJECT

    public:
        enum ProcessMode
        {
            Data,
            View,
            Hidden
        };

        Indicator(Plot *plot, PriceChart *priceChart, bool showProperties, const QString &configGroup = "", Indicator::ProcessMode mode = Indicator::View);
        ~Indicator(void);

        void readSettings(void);
        void writeSettings(void);
        virtual void removeSettings(void);
        void setConfigGroupKey(const QString key);
        QString &configGroupKey(void);
        bool autoRestore(void);
        void setAutoRestore(bool enabled);

        RuleList *ruleList(TransactionType type) const;
        void clearTransactions(void);
        void finishTransations(TransactionType currentTransactionType, const float *askOpen, const float *bidOpen);
        void addTransaction(float price, float spread, int bar, TransactionType type, TransactionType previousTransactionType, bool stopLoss, bool takeProfits, bool finished);
        void addTransactionToTransactionTable(float price, int bar, TransactionType type, float relativeProfits, bool stopLoss, bool takeProfits, bool finished);
        const TradeResults &tradeResults(void) const;

        int autoPeriodsForX(double x, const PeriodOptions &options);
        void calculateMACurve(const PlotCurve &source, PlotCurve &dest, int periods, int type = Simple, int subType = -1, int slowSmoothing = 30, int fastSmoothing = 2/*, int minAutoPeriods = 6, int maxAutoPeriods = 63*/);
        void calculateMACurve(const QVector<QPointF> &source, QVector<QPointF> &dest, int periods, int type = Simple, int subType = -1, int slowSmoothing = 30, int fastSmoothing = 2);
        void calculateMACurve(const PlotCurve &source, PlotCurve &dest, const MovingAverageOptions &options);
        void calculateMACurve(const QVector<QPointF> &source, QVector<QPointF> &dest, const MovingAverageOptions &options);
        void calculateSMACurve(const PlotCurve &source, PlotCurve &dest, int size, int periods);
        void calculateSMACurve(const QVector<QPointF> &source, QVector<QPointF> &dest, int size, int periods);
        void calculateSMACurve(const PlotCurve &source, PlotCurve &dest, const MovingAverageOptions &options);
        void calculateSMACurve(const QVector<QPointF> &source, QVector<QPointF> &dest, const MovingAverageOptions &options);
        void calculateEMACurve(const PlotCurve &source, PlotCurve &dest, int size, int periods);
        void calculateEMACurve(const QVector<QPointF> &source, QVector<QPointF> &dest, int size, int periods);
        void calculateEMACurve(const PlotCurve &source, PlotCurve &dest, const MovingAverageOptions &options);
        void calculateEMACurve(const QVector<QPointF> &source, QVector<QPointF> &dest, const MovingAverageOptions &options);
        void calculateLWMACurve(const PlotCurve &source, PlotCurve &dest, int size, int periods);
        void calculateLWMACurve(const QVector<QPointF> &source, QVector<QPointF> &dest, int size, int periods);
        void calculateLWMACurve(const PlotCurve &source, PlotCurve &dest, const MovingAverageOptions &options);
        void calculateLWMACurve(const QVector<QPointF> &source, QVector<QPointF> &dest, const MovingAverageOptions &options);
        void calculateAMACurve(const PlotCurve &source, PlotCurve &dest, int size, int periods, int slowSmoothing = 30, int fastSmoothing = 2);
        void calculateAMACurve(const QVector<QPointF> &source, QVector<QPointF> &dest, int size, int periods, int slowSmoothing = 30, int fastSmoothing = 2);
        void calculateAMACurve(const PlotCurve &source, PlotCurve &dest, const MovingAverageOptions &options);
        void calculateAMACurve(const QVector<QPointF> &source, QVector<QPointF> &dest, const MovingAverageOptions &options);
        void calculateHMACurve(const PlotCurve &source, PlotCurve &dest, int size, int periods);
        void calculateHMACurve(const QVector<QPointF> &source, QVector<QPointF> &dest, int size, int periods);
        void calculateJMACurve(const PlotCurve &source, PlotCurve &dest, int periods, int subType);
        void calculateJMACurve(const QVector<QPointF> &source, QVector<QPointF> &dest, int periods, int subType);
        void calculateGDEMACurve(const PlotCurve &source, PlotCurve &dest, int periods);
        void calculateGDEMACurve(const QVector<QPointF> &source, QVector<QPointF> &dest, int periods);
        void calculateT3Curve(const PlotCurve &source, PlotCurve &dest, int periods);
        void calculateT3Curve(const QVector<QPointF> &source, QVector<QPointF> &dest, int periods);
        void calculateZLEMACurve(const PlotCurve &source, PlotCurve &dest, int periods);
        void calculateZLEMACurve(const QVector<QPointF> &source, QVector<QPointF> &dest, int periods);
        void calculateMAMACurve(const PlotCurve &source, PlotCurve &dest, int periods, int slowSmoothing = 20, int fastSmoothing = 2);
        void calculateMAMACurve(const QVector<QPointF> &source, QVector<QPointF> &dest, int periods, int slowSmoothing = 20, int fastSmoothing = 2);
        void calculateFAMACurve(const PlotCurve &source, PlotCurve &dest, int periods, int slowSmoothing = 20, int fastSmoothing = 2);
        void calculateFAMACurve(const QVector<QPointF> &source, QVector<QPointF> &dest, int periods, int slowSmoothing = 20, int fastSmoothing = 2);
        void calculateVolatilityCurve(const PlotCurve &source, PlotCurve &dest, const PeriodOptions &options);
        void calculateVolatilityCurve(const QVector<QPointF> &source, QVector<QPointF> &dest, const PeriodOptions &options);
        void calculateERCurve(const PlotCurve &source, PlotCurve &dest, const PeriodOptions &options);
        void calculateERCurve(const QVector<QPointF> &source, QVector<QPointF> &dest, const PeriodOptions &options);
        void calculateTRCurve(const PlotCurve &high, const PlotCurve &low, const PlotCurve &close, PlotCurve &dest, int size);
        void calculateTRCurve(const QVector<QPointF> &high, const QVector<QPointF> &low, const QVector<QPointF> &close, QVector<QPointF> &dest, int size);
        void calculateRSICurve(const PlotCurve &source, PlotCurve &dest, int periods, int type, int subType = -1, int slowSmoothing = 30, int fastSmoothing = 2);
        void calculateRSICurve(const QVector<QPointF> &source, QVector<QPointF> &dest, int periods, int type, int subType = -1, int slowSmoothing = 30, int fastSmoothing = 2);
        void calculateMedianCurve(const PlotCurve &source1, const PlotCurve &source2, PlotCurve &dest);
        void calculateMedianCurve(const QVector<QPointF> &source1, const QVector<QPointF> &source2, QVector<QPointF> &dest);
        void calculateMedianCurve(const QVector<QPointF> &source1, const QVector<QPointF> &source2, Array &dest);
        void calculateMedianCurve(const float *source1, const float *source2, double *dest, int size) const;
        double median(const Array &price, int per, int bar);
        double median(const double *price, int per, int bar) const;

        double findAddressForX(const double x, const PlotCurve &curve) const { if (curve.dataSize() == 0) return - 1; else return x - curve.x(0); }
        double findAddressForX(const double x, const QVector<QPointF> &curve) const { if (curve.isEmpty()) return -1; else return x - curve[0].x(); }
        void setTitle(const QString &title);
        QString title(void) { return windowTitle(); }
        const QColor &color(void);
        void setColor(const QColor &c);
        virtual QWidget *customIndicatorWidget(void);
        void readRules(void);
        void writeRules(void);
        void clearRuleLists(void);
        void clearRuleList(QList <IndicatorRule *> &ruleList);
        bool deleteRule(const QTreeWidgetItem *item, QList <IndicatorRule *> *ruleList = 0);
        void clearRuleTree(QList<QTreeWidgetItem *> *children = NULL);
        void fillRuleTree(RuleTreeWidgetItem *parentItem = NULL, const QVector<Rule *> *list = NULL);

        void increaseExternalReferenceCount(void);
        void decreaseExternalReferenceCount(void);
        int externalReferenceCount(void);
        void setSkipCurveRecalculation(bool disabled);
        bool skipCurveRecalculation(void);
        void setXAxisScaleToCurve(const PlotCurve &curve);
        AutoOptimizerOption *addAutoOptimizerOption(const QString &title, bool *option, int min = 0, int max = 1, AutoOptimizerOption::optionTypes type = AutoOptimizerOption::IndicatorOption, const QStringList &valueTitles = QStringList());
        AutoOptimizerOption *addAutoOptimizerOption(const QString &title, int *option, int min, int max, int stepSize = 1, bool useAccelleration = true, AutoOptimizerOption::optionTypes type = AutoOptimizerOption::IndicatorOption, const QStringList &valueTitles = QStringList());
        AutoOptimizerOption *addAutoOptimizerOption(const QString &title, float *option, float min, float max, double stepSize = 1.0, bool useAccelleration = true, bool percentualNotation = false, AutoOptimizerOption::optionTypes type = AutoOptimizerOption::IndicatorOption);
        Indicator::ProcessMode processMode(void) const;
        void setProcessMode(Indicator::ProcessMode mode);
        bool autoOptimizerMode(void);
        void setAutoOptimizerMode(bool enabled);
        AutoOptimizer &autoOptimizer(void);
        const AutoOptimizer &autoOptimizer(void) const;
        void setSettingsGUIEnabled(bool enabled);
        bool blockSettingsGUISignals();
        void setBlockSettingsGUISignals(bool block);

        Plot *indicatorPlot;
        Plot *indexPlot;
        PriceChart *priceChart;
        MarkerPoints *markerPoints;
        QList <struct transaction> transactionList;
        QList <PlotCurve *> curveList;
        QList <IndicatorRule *> longRuleList;
        QList <IndicatorRule *> shortRuleList;
        QList <IndicatorRule *> exitRuleList;
        QString buttonName;
        const PlotCurve *indicatorSourceCurve;
        bool useOwnPlot;
        bool indicatorGivesSignals;
        bool showTransactionMarkers;
        double startCapital;
        double finalCapital;
        mutable long double totalEvaluations;
        double firstRuns;
        double secondRuns;

    signals:
        void titleChanged(Indicator *indicator, const QString &newTitle);
        void propertiesChanged(void);

    public slots:
        void executeTransactionRules();
        void zoomToSelectedItem(int currentRow, int currentColumn, int previousRow, int previousColumn);
        void removeIndicator(void);
        void setZoomPositionToMainPlot(const QwtDoubleRect &rect);
        void editRule(void);
        void addRule(void);
        void deleteRule(void);
        void renameRuleToNewTitle(Indicator *indicator, const QString &newTitle, QTreeWidgetItem *treeItem = NULL);
        void rulePopupApplyClicked(void);
        void fillCurveCombos(void);
        void toggleValueSpinBox(int index);
        void toggleReferenceComboBox(int index);
        void toggleRuleButtons(void);
        void setParentRulesEnabled(QTreeWidgetItem *child, bool enabled);
        void setChildRulesEnabled(QTreeWidgetItem *parent, bool enabled);
        void toggleCurrentRuleDisabled(void);
        int showColorDialog(void);
        void setToolTipForItem(QwtPlotSvgItem &item);
        void unsetToolTipForItem(QwtPlotSvgItem &item);
        void setColorButtonHidden(bool hidden);
        void setIndicatorSourceComboBoxHidden(bool hidden);
        void setIndicatorBackGroundColor(QColor color);
        void setIndicatorMajGridColor(QColor color);
        void setIndicatorMinGridColor(QColor color);
        void setIndicatorLegendColor(QColor color);
        void setTrackerColor(QColor color);

        virtual void plotCurve(void);
        virtual void setProperties();
        virtual void loadDefaultRules(void);
        virtual void updateGUI(void);
        void setupPropertiesWidget(void);

    protected:
        virtual void readSettingsPrivate(void) {}
        virtual void writeSettingsPrivate(void) {}

    private slots:
        void updatePalette();

    private:
        int nextSignal(int currentBar, TransactionType transactionType, int offset);
        void createStopLossCache(bool reuseOldCache = false);
        void clearStopLossCache(void);
        void createTakeProfitsCache(bool reuseOldCache = false);
        void clearTakeProfitsCache();
        float setLosingTradePrice(float startPrice, TransactionType transactionType, int currentBar, int transactionStart) const;
        bool isLosingTrade(float &losingPrice, float askHigh, float bidLow, TransactionType transactionType, int currentBar, int transactionStart) const;
        float losingTradePrice(float losingPrice, float askOpen, float bidOpen, TransactionType transactionType) const;
        float setWinningTradePrice(float startPrice, TransactionType transactionType) const;
        bool isWinningTrade(float winningPrice, float bidHigh, float askLow, TransactionType transactionType, int currentBar, int transactionStart) const;
        float winningTradePrice(float winningPrice, float bidOpen, float askOpen, TransactionType transactionType) const;
        bool isLosingStrategy(int currentBar, float price, TransactionType transactionType, const SessionInfo &sessionInfo) const;

        friend class AutoOptimizerWorker;
        class PrivateData;
        PrivateData *privateData;
};

#endif
