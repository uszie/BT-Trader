#ifndef STRATEGIE_H
#define STRATEGIE_H

#include "bttrader.h"
#include "dockwidget.h"
#include "plot.h"
#include "calculateCache.h"
#include "slidingstackedwidget.h"
#include "ui_strategiesetup.h"
#include "ui_quote.h"
#include <qpushbutton.h>
#include <qsplitter.h>
#include <qwidget.h>
#include <qdatetime.h>
#include <qdatetimeedit.h>
#include <qtablewidget.h>
#include <qstackedwidget.h>
#include <qfilesystemwatcher.h>
#include <qwt_plot_svgitem.h>
#include <qwt_text_label.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_price.h>
#include <qwt_price_move.h>
#include <qwt_plot.h>

extern bool aboutToQuit;

class Plot;
class PriceChart;
class StrategieOverview;
class PlotMCurve;
class PlotCurve;
class Index;
class Strategie;
class DataCollector;
class Indicator;
class Quote;

enum style
{
    SolidLine,
    DottedLine,
    Solid,
    Candle,
    Bar
};

enum transactionRows
{
    Date,
    Type,
    Relative,
    Capital
};

enum timeSkip
{
    Auto,
    Day,
    Week,
    Month,
    Year
};

struct transaction
{
    int type;
    int previousType;
    bool stopLoss;
    bool takeProfits;
    double x;
    double value;
    double relativeProfits;
    double currentCapital;
};

class MarkerPoints : public QObject
{
        Q_OBJECT

    public:
        enum IconType
        {
            IconLong,
            IconLongSelected,
            IconShort,
            IconShortSelected,
            IconExit,
            IconExitSelected
        };

        MarkerPoints(PriceChart *priceChart);
        ~MarkerPoints(void);
        void addMarker(struct transaction data);
        QByteArray svgIcon(MarkerPoints::IconType type) const;

    public slots:
        void highlightMarker(struct transaction data);
        void unHighlightMarker(struct transaction data);
        void rescaleMarkers(bool forceDataReload = false);
        void setColor(const QColor &color);


    signals:
        void hovered(QwtPlotSvgItem &item);
        void hoverLeft(QwtPlotSvgItem &item);

    protected:
        virtual bool eventFilter(QObject *object, QEvent *event);

    private:
        void init(void);
        void colorizeIcon(QByteArray &icon, bool selected = false);

        class PrivateData;
        PrivateData *privateData;

    private slots:
        void hover(void);
};

class PriceChart: public QWidget
{
        Q_OBJECT

    public:
        enum SourceType
        {
            Average,
            Ask,
            Bid
        };

        enum ProcessMode
        {
            Data,
            View,
            Hidden
        };

        PriceChart(const IsinInfo &info, int timeScale, Strategie *strategie, const QString &configGroup = "");
        virtual ~PriceChart(void);

        QList<Plot *> &plotList(void);
        const QList<Plot *> &plotList(void) const;
        void loadData(QDateTime startDateTime = QDateTime(), QDateTime endDateTime = QDateTime(), bool cached = false);
        void readSettings(void);
        void writeSettings(void);
        void removeSettings(void);
        QList<Indicator *> restoreIndicators(QStringList indicatorNumberList = QStringList(""), PriceChart::ProcessMode processMode = PriceChart::View);
        void finishCreateIndicator(Indicator *indicator);
        QString &configGroupKey(void);
        const QColor &takeColor(const QColor &color = Qt::transparent);
        void putColor(const QColor &color);
        PriceChart::ProcessMode processMode(void) const;
        void setProcessMode(PriceChart::ProcessMode mode);
        IsinInfo isinInfo(void) const;
        int timeScale(void) const;
        const QDateTime &firstAvailableDateTime(void) const;
        const QDateTime &lastAvailableDateTime(void) const;
        QDateTime startDateTime(void) const;
        QDateTime endDateTime(void) const;
        CalculateCache *indicatorCalculateCache(void) const;
        SlidingStackedWidget *indicatorStackWidget(void) const;
        StrategieOverview *strategieOverviewWidget(void) const;
        QTabWidget *transactionTabWidget(void) const;
        QSplitter *plotSplitterWidget(void) const;
        int dominantCyclePeriod(double x);
        bool trendAvailable(double x);
        Indicator *trendIndicator(void);
        Indicator *dominantCycleIndicator(void);
        Indicator *trendVigorIndicator(void);

        Strategie *strategie;
        PlotMCurve *averagePriceComposition;
        PlotMCurve *askPriceComposition;
        PlotMCurve *bidPriceComposition;
        PlotCurve *averagePriceOpen;
        PlotCurve *askPriceOpen;
        PlotCurve *bidPriceOpen;
        PlotCurve *averagePriceHigh;
        PlotCurve *askPriceHigh;
        PlotCurve *bidPriceHigh;
        PlotCurve *averagePriceLow;
        PlotCurve *askPriceLow;
        PlotCurve *bidPriceLow;
        PlotCurve *averagePriceClose;
        PlotCurve *askPriceClose;
        PlotCurve *bidPriceClose;
        QList<PlotCurve *> priceCurveList;

        Plot *indexPlot;
        QScrollArea *indicatorScrollArea;
        QList <Indicator *> indicatorList;
        Ui::StrategieSetup *ssForm;
        double startCapital;
        double fixedTransactionCosts;
        double variableTransactionCosts;
        double minimumTransactionCosts;
        double maximumTransactionCosts;
        double averageSpread;
        bool mailNotify;
        bool smsNotify;

        class PrivateData;
        PrivateData *privateData;

    public slots:
        QWidget *priceCurvePropertiesWidget(void);
        int showSetupDialog(void);
        void setProperties(void);
        void setIsinInfo(const IsinInfo &info);
        void setTimeScale(int timeScale);
        void setStartDateFromPointer(void);
        void setEndDateFromPointer(void);
        void cursorMoved(const QPoint &pos);
        void handleOverviewDestroyed(void);

    signals:
        void dataSet(void);
        void dataAppended(void);
        void dataCleared(void);
        void calculateProfits(void);
        void timeScaleChanged(int oldTimeScale, int newTimeScale);
        void isinInfoChanged(IsinInfo oldIsinInfo, IsinInfo newIsinInfo);
        void firstAvailableDateTimeChanged(const QDateTime &dateTime);
        void lastAvailableDateTimeChanged(const QDateTime &dateTime);
        void startDateTimeChanged(const QDateTime &dateTime);
        void endDateTimeChanged(const QDateTime &dateTime);
        void removeRequested(PriceChart *priceChart);

    private slots:
        void handleGetFirstAvailableDateTimeRequest(QDateTime &dateTime);
        void handleGetLastAvailableDateTimeRequest(QDateTime &dateTime);
        void doLoadData(QList<PriceEntry> &priceEntrylist);
        void doAppendDataToPlot(QList<PriceEntry> &priceEntryList);
        void appendDataToPlot(const QString &notifier);
        void setPriceProperties(void);
        void updatePriceCurveVisibility(void);
        void updateSimulationProperties(void);

    private:
        QAction *setStartDateAction;
        QAction *setEndDateAction;
};

class Strategie : public QWidget
{

        Q_OBJECT

    public:

        Strategie(QWidget *parent);
        ~Strategie(void);

        void restorePriceCharts();
        PriceChart *currentPriceChart(void);

        bool ignoreDataEditChanged(void) { return ignoreDateEditChangedSignal; }
        void setIgnoreDateEditChanged(bool value) { ignoreDateEditChangedSignal = value; }
        QString timeScaleToString(int timeScale);
        void createBTTStrategieMenuEntry(const QString title);
        void fillStrategieMenu(void);
        int getFirstAvailableIndicatorNumber(PriceChart *strategiePage);
        void setDataSearchWidgetEnabledForPriceChart(bool enabled, PriceChart *priceChart);
        SlidingStackedWidget *chartStack(void) const;

        int globalStopLossType(void);
        void setGlobalStopLossType(int type);
        double globalStopLossValue(void);
        void setGlobalStopLossValue(double value);
        double globalLongStopLossValue(void);
        void setGlobalLongStopLossValue(double value);
        double globalShortStopLossValue(void);
        void setGlobalShortStopLossValue(double value);
        double globalProfitTrailingThreshold(void);
        void setGlobalProfitTrailingThresHold(double threshold);
        double globalLongProfitTrailingThreshold(void);
        void setGlobalLongProfitTrailingThresHold(double threshold);
        double globalShortProfitTrailingThreshold(void);
        void setGlobalShortProfitTrailingThresHold(double threshold);
        double globalMaximumStopLossFactor(void);
        void setGlobalMaximumStopLossFactor(double value);
        double globalMaximumThresholdStopLossFactor(void);
        void setGlobalMaximumThresholdStopLossFactor(double value);
        double globalLongMaximumStopLossFactor(void);
        void setGlobalLongMaximumStopLossFactor(double value);
        double globalLongMaximumThresholdStopLossFactor(void);
        void setGlobalLongMaximumThresholdStopLossFactor(double value);
        double globalShortMaximumStopLossFactor(void);
        void setGlobalShortMaximumStopLossFactor(double value);
        double globalShortMaximumThresholdStopLossFactor(void);
        void setGlobalShortMaximumThresholdStopLossFactor(double value);
        int globalStopLossHits(void);
        void setGlobalStopLossHits(int hits);
        int globalLongStopLossHits(void);
        void setGlobalLongStopLossHits(int hits);
        int globalShortStopLossHits(void);
        void setGlobalShortStopLossHits(int hits);

    public slots:
        void createRSIIndicator(void);
        void createStochasticIndicator(void);
        void createADXIndicator(void);
        void createDominantCycleIndicator(void);
        void createTrendVigorIndicator(void);
        void createSNRIndicator(void);
        void createVolatilityIndicator(void);
        void createERIndicator(void);
        void createMAIndicator(void);
        void create2MAIndicator(void);
        void create3MAIndicator(void);
        void createBollingerBandsIndicator(void);
        void createPriceChannelIndicator(void);
        void createInstantaneousTrendIndicator(void);
        void createTrendLineIndicator(void);
        void createSupportCeilingLineIndicator();
        void saveBTTStrategie(void);
        void removeBTTStrategie(void);
        void renameBTTStrategie(void);
        void importBTTStrategie(void);
        void exportBTTStrategie(void);
        void loadBTTStrategie(void);
        void zoomIn(void);
        void zoomOut(void);
        void zoomReset(void);
        void fullHistoryView(void);
        void rewindDay(void) { rewindDate(Day); }
        void rewindWeek(void) { rewindDate(Week); }
        void rewindMonth(void) { rewindDate(Month); }
        void rewindYear(void) { rewindDate(Year); }
        void rewindDate(int timeSkip = Auto);
        void forwardDay(void) { forwardDate(Day); }
        void forwardWeek(void) { forwardDate(Week); }
        void forwardMonth(void) { forwardDate(Month); }
        void forwardYear(void) { forwardDate(Year); }
        void forwardDate(int timeSKip = Auto);
        void setStartDate(QDate date);
        void setEndDate(QDate date);
        void updateStartDate(const QDateTime &dateTime);
        void updateEndDate(const QDateTime &dateTime);
        void updateFirstAvailableCalendarDate(const QDateTime &dateTime);
        void updateLastAvailableCalendarDate(const QDateTime &dateTime);
        void setMouseTracking(bool enabled);
        void switchToMinuteTimeScale(void);
        void switchToHourTimeScale(void);
        void switchToDayTimeScale(void);
        void switchToWeekTimeScale(void);
        void switchToMonthTimeScale(void);
        void updateTimeScaleMenu(void);
        PriceChart *addPriceChart(void);
        PriceChart *addPriceChart(const IsinInfo &info, int timeScale = HourByHour, const QString &configKey = "", bool switchToNewChart = true);
        void replacePriceChart(const IsinInfo &info);
        void switchToPriceChart(PriceChart *priceChart, bool animated = false);
        void doSwitchToPriceChart(void);
        void removePriceChart(PriceChart *priceChart);
        void updateDockBackground(const QPoint &from, const QPoint &to);
        void updateDockBackground(void);
        void updateDockSpacing(void);
        void updateDockStatus(void);
        void testSlot(void);

    public:
        QDateEdit *startDateEdit;
        QDateEdit *endDateEdit;
        QMainWindow *quotesDockWindow;

    private slots:
        void setApplicationAboutToQuit(void);
        void doUpdateDockBackground(void);

        void replotCurrentPriceChart();
    private:

        QFileSystemWatcher *fileWatcher;
        bool mouseTracking;
        bool ignoreDateEditChangedSignal;
        QList<PriceChart *>priceChartList;
        QList<bool> dataSearchEnabledList;
        class PrivateData;
        PrivateData *privateData;
};

extern Strategie *strategie;
class RoundWidget : public QWidget
{
        Q_OBJECT

    public:
        RoundWidget(QWidget *parent = 0, Qt::WindowFlags f = 0);
        virtual ~RoundWidget(void) {}

        virtual void paintEvent(QPaintEvent *paintEvent);
};

class PopupWidget : public RoundWidget
{
        Q_OBJECT

    public:
        PopupWidget(QWidget *parent = 0, Qt::WindowFlags f = 0);
        virtual ~PopupWidget(void);

        bool eventFilter(QObject *obj, QEvent *event);
        void show(void);
        void resize(const QSize &size);

    private:
        RoundWidget *layoutWidget;
        RoundWidget *colorRing;
        RoundWidget *widget;
};

#endif // STRATEGIE_H
