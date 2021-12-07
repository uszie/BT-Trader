#include "indicator.h"
#include "strategieoverview.h"
#include "main.h"
#include "autooptimizer.h"
#include "calculateCache.h"
#include "rule_evaluation.h"
#include "takeprofitoptions.h"
#include "stoplossoptions.h"
#include "blur.h"
#include "ui_rulepopup.h"
#include "ui_indicator.h"
#include <QtGui>
#include <qwt_legend.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_item.h>

#define USE_NEW_FAST_EVALUATORS 1

bool debugEnabled = false;

ui4vector longMask = { { Long, Long, Long, Long } };
ui4vector shortMask = { { Short, Short, Short, Short } };
ui4vector exitMask = { { Exit, Exit, Exit, Exit } };
ui4vector multiplierMask = { { 1, 1, 1, 1 } };
ui4vector iFullMask = { { 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff } };
f4vector fNullMask = { { 0.0f, 0.0f, 0.0f, 0.0f } };
f4vector fIgnoreValue = { { IGNORE_VALUE, IGNORE_VALUE, IGNORE_VALUE, IGNORE_VALUE } };
f4vector fMinusOne = { { -1.0f, -1.0f, -1.0f, -1.0f  } };
ui4vector iZero4 = { { 0, 0, 0, 0 } };
i8vector iZero8 = { { 0, 0, 0, 0, 0, 0, 0, 0 } };
f4vector fZero = { { 0.0f, 0.0f, 0.0f, 0.0f } };
ui4vector iOne4 = { { 1, 1, 1, 1 } };
f4vector fOne = { { 1.0f, 1.0f, 1.0f, 1.0f } };
f4vector fTwo = { { 2.0f, 2.0f, 2.0f, 2.0f } };
ui4vector iTen4 = { { 10, 10, 10, 10 } };
f4vector fHundred = { { 100.0f, 100.0f, 100.0f, 100.0f } };
ui4vector intMax = { { 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff } };

RasterData::RasterData(const MultiArray &array, const QwtDoubleRect &rect, double colorRange, double arrayYBottom, double arrayYTop):
    QwtRasterData(QwtDoubleRect(rect))
{
    p_array = array;
    p_colorRange = colorRange;
    p_arrayYBottom = arrayYBottom;
    p_arrayYTop = arrayYTop;
    p_plotYTop = rect.bottom();
    p_plotYBottom = rect.top();
    p_plotRange = rect.height();
    p_arrayRange = p_arrayYTop - p_arrayYBottom;
    p_colorRangeRate = 1.0 / p_colorRange;
}

RasterData::~RasterData(void)
{
}

QwtRasterData *RasterData::copy() const
{
    return new RasterData(p_array, boundingRect(), p_colorRange, p_arrayYBottom, p_arrayYTop);
}


QwtDoubleInterval RasterData::range() const
{
    return QwtDoubleInterval(0, 1.0);
}

inline double RasterData::value(double x, double y) const
{
    double translatedY = (((y - p_plotYBottom) * p_arrayRange) / p_plotRange) + p_arrayYBottom;
    const QVector<double> &yArray = p_array.at(x);
    const double &pixelValue = yArray.at(translatedY);
    if (pixelValue == 0.0)
        return 1.0;

    return p_colorRangeRate * pixelValue;
}

QSize RasterData::rasterHint(const QwtDoubleRect &rect) const
{
    if (p_array.size() > 2000 && rect.width()  > 2000)
        return QSize(2000, (p_arrayYTop - p_arrayYBottom) + 1);

    return QSize(p_array.size(), (p_arrayYTop - p_arrayYBottom) + 1);
}

PlotSpectrogram::PlotSpectrogram(const QString &title): QwtPlotSpectrogram(title)
{
}

PlotSpectrogram::~PlotSpectrogram()
{

}

QImage PlotSpectrogram::testFunc(const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QwtDoubleRect &rect) const
{
    return QwtPlotSpectrogram::renderImage(xMap, yMap, rect);
}

QImage PlotSpectrogram::renderImage(const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QwtDoubleRect &rect) const
{
    QTime timer;
    timer.start();
    QImage image = QwtPlotSpectrogram::renderImage(xMap, yMap, rect);

    if (image.format() == QImage::Format_ARGB32 || image.format() == QImage::Format_ARGB32_Premultiplied)
    {
        if (image.height() < 16 || image.width() < 16)
            image = image.scaled(image.width() * 16, image.height() * 16);

        vectorExponentialBlur(image, 8);
    }

    return image;
}

class TransactionCosts
{
    public:
        float transactionStartValue;
        float totalTransactionsCosts;
        float fixedTransactionCosts;
        float variableTransactionCosts;
        float minimumTransactionCosts;
        float maximumTransactionCosts;
        float averageSpread;
};

class RuleTreeWidgetItem : public QTreeWidgetItem
{
    public:
        Rule *rule;
};

class Indicator::PrivateData
{
    public:
        Ui::RulePopup rulePopupForm;
        bool showProperties;
        Ui::IndicatorPropertiesUI propertiesForm;
        QWidget *propertiesWidget;
        Ui::IndicatorTransactionsTableUI transactionTableForm;
        QWidget *transactionTableWidget;
        SimulationPropertiesData simulationPropertiesData;
        Indicator::ProcessMode processMode;
        AutoOptimizer *autoOptimizer;
        TradeResults tradeResults;
        TransactionCosts transactionCosts;
        float transactionStartValue;
        bool autoRestore;
        QString configGroup;
        QColor color;
        RuleFilter globalFilter;
        bool blockSettingsGUISignals;
        bool autoOptimizerMode;
        bool skipCurveRecalculation;
        bool useShortTransactions;
        bool useLongTransactions;
        bool useSmsNotification;
        bool useEmailNotification;
        bool colorButtonHidden;
        bool indicatorSourceComboBoxHidden;
        int externalUsageReference;

        TakeProfitOptions takeProfitOptions;
        StopLossOptions stopLossOptions;

        RuleList *longRuleList;
        RuleList *shortRuleList;
        RuleList *exitRuleList;

        QCache<quint64, SignalCacheOject> stopLossCache;
        SignalCacheOject *stopLossCacheEntry;
        QCache<quint64, SignalCacheOject> takeProfitsCache;
        SignalCacheOject *takeProfitsCacheEntry;
        QPair<TransactionType, QDateTime> lastAlarmTrigger;
};

Indicator::Indicator(Plot *plot, PriceChart *priceChart, bool showProperties, const QString &configGroup, Indicator::ProcessMode mode) : QWidget(plot)
{
    privateData = (PrivateData *) _mm_malloc(sizeof(PrivateData), 16);
    new(privateData) PrivateData();
    privateData->processMode = static_cast<Indicator::ProcessMode>(-1);
    privateData->propertiesWidget = NULL;
    privateData->showProperties = showProperties;
    privateData->color = Qt::transparent;
    indicatorPlot = NULL;
    markerPoints = NULL;
    this->priceChart = priceChart;
    indexPlot = priceChart->indexPlot;
    privateData->propertiesForm.titleWidget = NULL;
    privateData->configGroup = configGroup;
    privateData->externalUsageReference = 0;
    privateData->skipCurveRecalculation = false;
    privateData->autoOptimizerMode = false;
    privateData->blockSettingsGUISignals = false;
    privateData->autoOptimizer = new AutoOptimizer(this, Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    privateData->autoRestore = true;
    privateData->longRuleList = new RuleList(priceChart, this, Long);
    privateData->shortRuleList = new RuleList(priceChart, this, Short);
    privateData->exitRuleList = new RuleList(priceChart, this, Exit);
    privateData->stopLossCache.setMaxCost(200);
    privateData->takeProfitsCache.setMaxCost(200);
    privateData->lastAlarmTrigger.first = static_cast<TransactionType>(-1);
    privateData->lastAlarmTrigger.second = QDateTime();

    memset(&privateData->propertiesForm, 0, sizeof(privateData->propertiesForm));
    priceChart->indicatorList.append(this);


    indicatorGivesSignals = true;
    privateData->colorButtonHidden = false;
    privateData->indicatorSourceComboBoxHidden = false;
    indicatorSourceCurve = priceChart->averagePriceClose;

    if (!plot)
    {
        useOwnPlot = true;
        indicatorPlot = new Plot();
        indicatorPlot->setPeriods(indexPlot->periods);
        indicatorPlot->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        indicatorPlot->setMinimumHeight(60);
        indicatorPlot->setAutoZoomEnabled(false);
        indicatorPlot->zoomer()->setHScrollBarMode(Qt::ScrollBarAlwaysOff);
        setIndicatorBackGroundColor(Qt::black);
        setIndicatorMajGridColor(Qt::transparent);
        setIndicatorMinGridColor(Qt::transparent);

        indicatorPlot->setTimeScaleData(indexPlot->timeScaleData());
        indicatorPlot->setTimeScaleFormat(indexPlot->timeScaleFormat());

        connect(indexPlot, SIGNAL(periodsChanged(int)), indicatorPlot, SLOT(setPeriods(int)));
    }
    else
    {
        useOwnPlot = false;
        indicatorPlot = plot;
    }

    connect(mainWindow->ui->indicatorDockWidget, SIGNAL(topLevelChanged(bool)), this, SLOT(updatePalette()));
    connect(mainWindow->ui->transactionsDockWidget, SIGNAL(topLevelChanged(bool)), this, SLOT(updatePalette()));

    privateData->transactionTableWidget = new QWidget(this);
    privateData->transactionTableWidget->setHidden(true);
    privateData->transactionTableForm.setupUi(privateData->transactionTableWidget);
    privateData->transactionTableForm.tableWidget->setColumnWidth(0, 40);
    privateData->transactionTableForm.tableWidget->setColumnWidth(1, 60);
    privateData->transactionTableForm.tableWidget->setColumnWidth(2, 60);
    privateData->transactionTableForm.tableWidget->setColumnWidth(3, 35);
    privateData->transactionTableForm.tableWidget->setColumnWidth(4, 40);
    privateData->transactionTableForm.tableWidget->setColumnWidth(5, 60);
    privateData->transactionTableForm.tableWidget->setColumnWidth(6, 50);
    privateData->transactionTableForm.tableWidget->setColumnHidden(0, true);

    if (!debugEnabled)
    {
        QStringList stringList;
        stringList << tr("Normaal") << tr("Trailing");
        privateData->autoOptimizer->addOption(this, tr("Stop loss type"), (int *)&privateData->stopLossOptions.type, 0, 0, 1, false, AutoOptimizerOption::StopLossOption, stringList);

        stringList.clear();
        stringList << tr("Absoluut") << tr("Relatief");
        AutoOptimizerOption *stopLossCompareOption = privateData->autoOptimizer->addOption(this, tr("Stoploss vergelijk methode"), &privateData->stopLossOptions.isRelative, 1, 1, AutoOptimizerOption::StopLossOption, stringList);

        AutoOptimizerOption *relativeStopLossOption = privateData->autoOptimizer->addOption(this, tr("Stoploss (relatief)"), &privateData->stopLossOptions.relativeValue, 0.0, 40, true, true, AutoOptimizerOption::StopLossOption);
        relativeStopLossOption->setSuffix(" %");
        relativeStopLossOption->addDependency(stopLossCompareOption, 1);

        AutoOptimizerOption *absoluteStopLossOption = privateData->autoOptimizer->addOption(this, tr("Stoploss (absoluut)"), &privateData->stopLossOptions.absoluteValue, 0.0, 40, true, false, AutoOptimizerOption::StopLossOption);
        absoluteStopLossOption->setSuffix(" punten");
        absoluteStopLossOption->addDependency(stopLossCompareOption, 0);

        stringList.clear();
        stringList << tr("Absoluut") << tr("Relatief");
        AutoOptimizerOption *takeProfitsCompareOption = privateData->autoOptimizer->addOption(this, tr("Winst nemen vergelijk methode"), &privateData->takeProfitOptions.isRelative, 1, 1, AutoOptimizerOption::TakeProfitOption, stringList);

        AutoOptimizerOption *relativeTakeProfitsOption = privateData->autoOptimizer->addOption(this, tr("Winst nemen (relatief)"), &privateData->takeProfitOptions.relativeValue, 0.0, 40, true, true, AutoOptimizerOption::TakeProfitOption);
        relativeTakeProfitsOption->setSuffix(" %");
        relativeTakeProfitsOption->addDependency(takeProfitsCompareOption, 1);

        AutoOptimizerOption *absoluteTakeProfitsOption = privateData->autoOptimizer->addOption(this, tr("Winst nemen (absoluut)"), &privateData->takeProfitOptions.absoluteValue, 0.0, 40, true, false, AutoOptimizerOption::TakeProfitOption);
        absoluteTakeProfitsOption->setSuffix(" punten");
        absoluteTakeProfitsOption->addDependency(takeProfitsCompareOption, 0);
    }
    else
    {
        QStringList stringList;
        stringList << tr("Normaal") << tr("Trailing");
        privateData->autoOptimizer->addOption(this, tr("Stop loss type"), (int *)&privateData->stopLossOptions.type, 0, 0, 1, false, AutoOptimizerOption::StopLossOption, stringList);

        stringList.clear();
        stringList << tr("Absoluut") << tr("Relatief");
        AutoOptimizerOption *stopLossCompareOption = privateData->autoOptimizer->addOption(this, tr("Stoploss vergelijk methode"), &privateData->stopLossOptions.isRelative, 1, 1, AutoOptimizerOption::StopLossOption, stringList);

        AutoOptimizerOption *relativeStopLossOption = privateData->autoOptimizer->addOption(this, tr("Stoploss (relatief)"), &privateData->stopLossOptions.relativeValue, 0.0, 0.01, 0.0025, true, true, AutoOptimizerOption::StopLossOption);
        relativeStopLossOption->setSuffix(" %");
        relativeStopLossOption->addDependency(stopLossCompareOption, 1);

        AutoOptimizerOption *absoluteStopLossOption = privateData->autoOptimizer->addOption(this, tr("Stoploss (absoluut)"), &privateData->stopLossOptions.absoluteValue, 0.0, 1.0, 1.0, true, false, AutoOptimizerOption::StopLossOption);
        absoluteStopLossOption->setSuffix(" punten");
        absoluteStopLossOption->addDependency(stopLossCompareOption, 0);

        stringList.clear();
        stringList << tr("Absoluut") << tr("Relatief");
        AutoOptimizerOption *takeProfitsCompareOption = privateData->autoOptimizer->addOption(this, tr("Winst nemen vergelijk methode"), &privateData->takeProfitOptions.isRelative, 1, 1, AutoOptimizerOption::TakeProfitOption, stringList);

        AutoOptimizerOption *relativeTakeProfitsOption = privateData->autoOptimizer->addOption(this, tr("Winst nemen (relatief)"), &privateData->takeProfitOptions.relativeValue, 0.0, 0.01, 0.0025, true, true, AutoOptimizerOption::TakeProfitOption);
        relativeTakeProfitsOption->setSuffix(" %");
        relativeTakeProfitsOption->addDependency(takeProfitsCompareOption, 1);

        AutoOptimizerOption *absoluteTakeProfitsOption = privateData->autoOptimizer->addOption(this, tr("Winst nemen (absoluut)"), &privateData->takeProfitOptions.absoluteValue, 0.0, 1.0, 1.0, true, false, AutoOptimizerOption::TakeProfitOption);
        absoluteTakeProfitsOption->setSuffix(" punten");
        absoluteTakeProfitsOption->addDependency(takeProfitsCompareOption, 0);
    }

    setProcessMode(mode);
}

Indicator::~Indicator(void)
{
    if (!aboutToQuit || !autoRestore())
        removeSettings();

    priceChart->putColor(color());
    clearRuleLists();

    if (markerPoints)
        delete markerPoints;

    if (privateData->propertiesWidget)
        delete privateData->propertiesWidget;

    if (privateData->transactionTableWidget)
        delete privateData->transactionTableWidget;

    priceChart->indicatorList.removeOne(this);

    if (useOwnPlot)
    {
        priceChart->plotList().removeOne(indicatorPlot);
        delete indicatorPlot;

        for (int i = 0; i < priceChart->plotList().size(); i++)
        {
            Plot *plot = priceChart->plotList().at(i);
            if (i == (priceChart->plotList().size() - 1))
                plot->enableAxis(QwtPlot::xBottom, true);
            else
                plot->enableAxis(QwtPlot::xBottom, false);
        }
    }
    else
    {
        while (!curveList.isEmpty())
        {
            PlotCurve *ptr = curveList.takeLast();
            if (ptr)
                delete ptr;
        }

        indexPlot->replot();
    }

    //    delete privateData;
    privateData->~PrivateData();
    _mm_free(privateData);
}

void Indicator::removeIndicator(void)
{
    if (privateData->externalUsageReference > 0)
    {
        QMessageBox messageBox(this);
        messageBox.setText(tr("Deze indicator wordt gebruikt door een andere, verwijder daar eerst in de voorwaarden lijst alle verwijzingen naar deze indicator"));
        messageBox.exec();
        return;
    }

    deleteLater();
}

void Indicator::increaseExternalReferenceCount(void)
{
    privateData->externalUsageReference += 1;
}
void Indicator::decreaseExternalReferenceCount(void)
{
    privateData->externalUsageReference -= 1;
}

int Indicator::externalReferenceCount(void)
{
    return privateData->externalUsageReference;
}

void Indicator::readSettings(void)
{
    if (!priceChart)
        return;

    while (!settings.group().isEmpty())
        settings.endGroup();
    settings.beginGroup(QString("Strategie/Charts/" + priceChart->configGroupKey() + "/Indicators/" + configGroupKey()));

    int indicatorSource = settings.value("IndicatorSourceCurve", Close).toInt();
    if (indicatorSource == Open)
        indicatorSourceCurve = priceChart->averagePriceOpen;
    else if (indicatorSource == High)
        indicatorSourceCurve = priceChart->averagePriceHigh;
    else if (indicatorSource == Low)
        indicatorSourceCurve = priceChart->averagePriceLow;
    else
        indicatorSourceCurve = priceChart->averagePriceClose;

    for (int i = 0; i < curveList.size(); ++i)
    {
        if (indicatorSourceCurve == priceChart->averagePriceOpen)
            curveList.at(i)->setPlotOffset(0.2);
        else
            curveList.at(i)->setPlotOffset(0.8);
    }

    privateData->useSmsNotification = settings.value("UseSmsNotification", false).toBool();
    privateData->useEmailNotification = settings.value("UseEmailNotification", false).toBool();

    privateData->takeProfitOptions = settings.value("TakeProfitOptions", TakeProfitOptions::defaultProperties()).value<TakeProfitOptions>();
    privateData->stopLossOptions = settings.value("StopLossOptions", StopLossOptions::defaultProperties()).value<StopLossOptions>();

    if (processMode() != Indicator::Hidden)
    {
        QColor c = settings.value("Color", Qt::transparent).value<QColor>();
        if (c == Qt::transparent && privateData->color == Qt::transparent)
            setColor(Qt::transparent);
        else if (c != Qt::transparent)
            setColor(c);
    }

    privateData->useLongTransactions = settings.value("UseLongTransactions", true).toBool();
    privateData->useShortTransactions = settings.value("UseShortTransactions", true).toBool();
    privateData->globalFilter.type = settings.value("Filter", NoFilter).toInt();
    privateData->globalFilter.periods = settings.value("FilterPeriods", 2).toInt();
    privateData->globalFilter.relativeDifference = settings.value("RelativeDifference", 0.005).toFloat();
    privateData->globalFilter.absoluteDifference = settings.value("AbsoluteDifference", 1).toFloat();
    showTransactionMarkers = settings.value("ShowTransactionMarkers", true).toBool();
    startCapital = settings.value("StartCapital", priceChart->startCapital).toDouble();
    QList<QVariant> curveColorList = settings.value("CurveColorList").toList();
    QList<QVariant> curveWidthList = settings.value("CurveWidthList").toList();
    QList<QVariant> curveStyleList = settings.value("CurveStyleList").toList();
    QList<QVariant> curveFillList = settings.value("CurveFillList").toList();
    if (curveColorList.isEmpty() || curveWidthList.isEmpty() || curveStyleList.isEmpty() || curveFillList.isEmpty() || (curveList.size() != curveColorList.size()) || (curveColorList.size() != curveWidthList.size()) || (curveWidthList.size() != curveStyleList.size()) || (curveStyleList.size() != curveFillList.size()))
    {
        for (int i = 0; i < curveList.size(); i++)
        {
            PlotCurve *curve = curveList.at(i);
            QBrush brush = curve->brush();
            if (brush != Qt::NoBrush)
            {
                QColor c = color();
                c.setAlpha(127);
                brush.setColor(c);
                curve->setBrush(brush);
            }

            QPen pen = curve->pen();
            if (pen.color() == Qt::transparent)
            {
                pen.setWidth(2);
                pen.setStyle(static_cast<Qt::PenStyle>(i + 1));
                pen.setColor(color());
                curve->setPen(pen);
                QwtSymbol symbol = curve->symbol();
                symbol.setBrush(curve->pen().brush());
                symbol.setPen(curve->pen());
                curve->setSymbol(symbol);
            }
        }
    }
    else
    {
        for (int i = 0; i < curveList.size(); i++)
        {
            PlotCurve *curve = curveList.at(i);
            QPen pen = curve->pen();
            pen.setWidth(curveWidthList.at(i).toInt());
            pen.setStyle(static_cast<Qt::PenStyle>(curveStyleList.at(i).toInt()));
            pen.setColor(curveColorList.at(i).value<QColor>());
            curve->setPen(pen);
            curve->setBrush(curveFillList.at(i).value<QBrush>());
            QwtSymbol symbol = curve->symbol();
            symbol.setBrush(curve->pen().brush());
            symbol.setPen(curve->pen());
            curve->setSymbol(symbol);
        }
    }

    readSettingsPrivate();
}

void Indicator:: writeSettings(void)
{
    if (!autoRestore())
        return;

    if (!priceChart)
        return;

    while (!settings.group().isEmpty())
        settings.endGroup();
    settings.beginGroup(QString("Strategie/Charts/" + priceChart->configGroupKey() + "/Indicators/" + configGroupKey()));

    int indicatorSource;
    if (indicatorSourceCurve == priceChart->averagePriceOpen)
        indicatorSource = Open;
    else if (indicatorSourceCurve == priceChart->averagePriceHigh)
        indicatorSource = High;
    else if (indicatorSourceCurve == priceChart->averagePriceLow)
        indicatorSource = Low;
    else
        indicatorSource = Close;
    settings.setValue("IndicatorSourceCurve", indicatorSource);
    settings.setValue("UseSmsNotification", privateData->useSmsNotification);
    settings.setValue("UseEmailNotification", privateData->useEmailNotification);
    settings.setValue("TakeProfitOptions", privateData->takeProfitOptions);
    settings.setValue("StopLossOptions", privateData->stopLossOptions);
    settings.setValue("Color", color());
    settings.setValue("UseLongTransactions", privateData->useLongTransactions);
    settings.setValue("UseShortTransactions", privateData->useShortTransactions);
    settings.setValue("Filter", privateData->globalFilter.type);
    settings.setValue("FilterPeriods", privateData->globalFilter.periods);
    settings.setValue("AbsoluteDifference", privateData->globalFilter.absoluteDifference);
    settings.setValue("RelativeDifference", privateData->globalFilter.relativeDifference);
    settings.setValue("ShowTransactionMarkers", showTransactionMarkers);
    settings.setValue("StartCapital", startCapital);
    settings.setValue("AutoRestore", privateData->autoRestore);

    QList<QVariant> curveColorList;
    QList<QVariant> curveWidthList;
    QList<QVariant> curveStyleList;
    QList<QVariant> curveFillList;
    for (int i = 0; i < curveList.size(); i++)
    {
        QPen pen = curveList.at(i)->pen();
        curveColorList.append(pen.color());
        curveWidthList.append(pen.width());
        curveStyleList.append(static_cast<int>(pen.style()));
        curveFillList.append(curveList.at(i)->brush());
    }

    settings.setValue("CurveColorList", curveColorList);
    settings.setValue("CurveWidthList", curveWidthList);
    settings.setValue("CurveStyleList", curveStyleList);
    settings.setValue("CurveFillList", curveFillList);

    writeSettingsPrivate();
}

void Indicator::removeSettings(void)
{
    if (!priceChart)
        return;

    while (!settings.group().isEmpty())
        settings.endGroup();

    settings.beginGroup(QString("Strategie/Charts/" + priceChart->configGroupKey() + "/Indicators/"));

    settings.remove(configGroupKey());
}

void Indicator::setConfigGroupKey(const QString key)
{
    privateData->configGroup = key;
}

QString &Indicator::configGroupKey(void)
{
    if (!privateData->configGroup.isEmpty())
        return privateData->configGroup;

    QString oldGroup = settings.group();

    while (!settings.group().isEmpty())
        settings.endGroup();

    settings.beginGroup("Strategie");
    settings.beginGroup("Charts");
    settings.beginGroup(priceChart->configGroupKey());
    settings.beginGroup("Indicators");

    QString configGroup = "Indicator";
    QStringList indicatorConfigList = settings.childGroups();
    for (int i = 0; i < INT_MAX; ++i)
    {
        if (i == 0)
            configGroup.append(QString::number(i));
        else
            configGroup.replace(QString::number(i - 1), QString::number(i));

        if (!indicatorConfigList.contains(configGroup))
        {
            bool found = false;
            for (int j = 0; j < priceChart->indicatorList.size(); ++j)
            {
                if (priceChart->indicatorList[ j ] == this)
                    continue;

                if (priceChart->indicatorList[ j ]->configGroupKey() == configGroup)
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                privateData->configGroup = configGroup;
                break;
            }
        }
    }

    while (!settings.group().isEmpty())
        settings.endGroup();

    settings.beginGroup(oldGroup);

    return privateData->configGroup;
}

bool Indicator::autoRestore(void)
{
    return privateData->autoRestore;
}

void Indicator::setAutoRestore(bool enabled)
{
    privateData->autoRestore = enabled;
}

RuleList *Indicator::ruleList(TransactionType type) const
{
    switch (type)
    {
        case Long:
            return privateData->longRuleList;
        case Short:
            return privateData->shortRuleList;
        case Exit:
            return privateData->exitRuleList;
        default:
            return NULL;
    }

    return NULL;
}

void Indicator::setTitle(const QString &title)
{
    if (privateData->propertiesForm.titleWidget)
        privateData->propertiesForm.titleLabel->setText(title);

    for (int i = 0; i <  priceChart->transactionTabWidget()->count(); ++i)
    {
        if (priceChart->transactionTabWidget()->widget(i) == privateData->transactionTableWidget)
            priceChart->transactionTabWidget()->setTabText(i, title);
    }

    setWindowTitle(title);

    emit titleChanged(this, title);
}

const QColor &Indicator::color(void)
{
    return privateData->color;
}

void Indicator::setColor(const QColor &color)
{
    priceChart->putColor(privateData->color);
    privateData->color = priceChart->takeColor(color);

    if (markerPoints)
        markerPoints->setColor(color);

    privateData->autoOptimizer->setColor(privateData->color);

    if (privateData->propertiesWidget)
    {
        QColor transparentColor = privateData->color;
        if (mainWindow->ui->indicatorDockWidget->isFloating())
            transparentColor.setAlpha(80);

        QColor textColor = (color.value() < 200) ? Qt::white : Qt::black;
        QPalette palette;
        palette.setColor(QPalette::Window, transparentColor);
        palette.setColor(QPalette::WindowText, textColor);
        privateData->propertiesForm.titleWidget->setPalette(palette);
    }

    if (privateData->transactionTableWidget)
    {
        QColor transparentColor = privateData->color;
        if (mainWindow->ui->transactionsDockWidget->isFloating())
            transparentColor.setAlpha(80);

        QColor textColor = (color.value() < 200) ? Qt::white : Qt::black;
        QPalette palette;
        palette.setColor(QPalette::Window, transparentColor);
        palette.setColor(QPalette::WindowText, textColor);
        privateData->transactionTableForm.colorLabel->setPalette(palette);
    }

    if (indicatorPlot)
        indicatorPlot->replot();
}

void Indicator::updatePalette(void)
{
    setColor(color());
}

void Indicator::setZoomPositionToMainPlot(const QwtDoubleRect &rect)
{
    (void) rect;

    if (indexPlot->zoomer()->horizontalScrollBar())
        connect(indexPlot->zoomer()->horizontalScrollBar(), SIGNAL(valueChanged(Qt::Orientation, double, double)), indicatorPlot->zoomer(), SLOT(scrollBarMoved(Qt::Orientation, double, double)), Qt::UniqueConnection);
}

void Indicator::plotCurve(void)
{
    setSkipCurveRecalculation(false);
    if (processMode() == Indicator::View)
        indicatorPlot->scheduleReplot();
}

void Indicator::setSkipCurveRecalculation(bool disabled)
{
    privateData->skipCurveRecalculation = disabled;
}

bool Indicator::skipCurveRecalculation(void)
{

    return privateData->skipCurveRecalculation;
}

void Indicator::setXAxisScaleToCurve(const PlotCurve &curve)
{
    if (autoOptimizerMode())
        return;

    if (indicatorPlot == indexPlot)
        return;

    indicatorPlot->setXAxisScaleToCurve(curve);
}

QWidget *Indicator::customIndicatorWidget(void)
{
    qDebug() << "Indicator::customIndicatorWidget()";
    return NULL;
}

int Indicator::showColorDialog(void)
{
    QColorDialog *dlg = new QColorDialog(this);
    dlg->setWindowModality(Qt::NonModal);
    dlg->setCurrentColor(color());
    int retval = dlg->exec();
    if (retval == QDialog::Accepted)
    {
        setColor(dlg->currentColor());
        setProperties();
    }

    return retval;
}

void Indicator::setupPropertiesWidget(void)
{
    if (!privateData->propertiesWidget)
    {
        privateData->propertiesWidget = new QWidget(this);
        privateData->propertiesForm.setupUi(privateData->propertiesWidget);

        QWidget *ciWidget = customIndicatorWidget();
        if (ciWidget)
        {
            ciWidget->layout()->setMargin(0);
            QFormLayout *globalWidgetLayout = qobject_cast<QFormLayout *>(privateData->propertiesForm.fillWidget->layout());
            QFormLayout *ciWidgetLayout = qobject_cast<QFormLayout *>(ciWidget->layout());
            if (!ciWidgetLayout)
            {
                for (int i = 0; i < ciWidget->layout()->count(); i++)
                {
                    if (!ciWidget->layout()->itemAt(i)->widget())
                    {
                        globalWidgetLayout->insertRow(600, ciWidget);
                        break;
                    }

                    QFormLayout *formLayout = qobject_cast<QFormLayout *>(ciWidget->layout()->itemAt(i)->widget()->layout());
                    if (!formLayout)
                        continue;

                    for (int i = 0; i < formLayout->rowCount(); i++)
                    {
                        if (formLayout->itemAt(i, QFormLayout::SpanningRole))
                            globalWidgetLayout->insertRow(600, formLayout->itemAt(i, QFormLayout::SpanningRole)->widget());
                        else if (formLayout->itemAt(i, QFormLayout::LabelRole) && formLayout->itemAt(i, QFormLayout::FieldRole))
                            globalWidgetLayout->insertRow(600, formLayout->itemAt(i, QFormLayout::LabelRole)->widget() , formLayout->itemAt(i, QFormLayout::FieldRole)->widget());
                        else if (formLayout->itemAt(i, QFormLayout::FieldRole))
                            globalWidgetLayout->insertRow(600, "", formLayout->itemAt(i, QFormLayout::FieldRole)->widget());
                    }
                }
            }
            else
            {
                for (int i = 0; i < ciWidgetLayout->rowCount(); i++)
                {
                    if (ciWidgetLayout->itemAt(i, QFormLayout::SpanningRole))
                    {
                        if (ciWidgetLayout->itemAt(i, QFormLayout::SpanningRole)->widget() && ciWidgetLayout->itemAt(i, QFormLayout::SpanningRole)->widget()->layout())
                        {
                            QFormLayout *subLayout = qobject_cast<QFormLayout *>(ciWidgetLayout->itemAt(i, QFormLayout::SpanningRole)->widget()->layout());
                            if (subLayout)
                            {
                                int minimumWidth = 0;
                                for (int j = 0; j < globalWidgetLayout->rowCount(); j++)
                                {
                                    if (!globalWidgetLayout->itemAt(j, QFormLayout::LabelRole) || !globalWidgetLayout->itemAt(j, QFormLayout::LabelRole)->widget())
                                        continue;

                                    if (globalWidgetLayout->itemAt(j, QFormLayout::LabelRole)->widget()->sizeHint().width() > minimumWidth)
                                        minimumWidth = globalWidgetLayout->itemAt(j, QFormLayout::LabelRole)->widget()->sizeHint().width();
                                }

                                for (int j = 0; j < subLayout->rowCount(); j++)
                                {
                                    if (!subLayout->itemAt(j, QFormLayout::LabelRole) || !subLayout->itemAt(j, QFormLayout::LabelRole)->widget())
                                        continue;

                                    subLayout->itemAt(j, QFormLayout::LabelRole)->widget()->setMinimumWidth(minimumWidth);
                                    break;
                                }
                            }
                        }
                        globalWidgetLayout->insertRow(600, ciWidgetLayout->itemAt(i, QFormLayout::SpanningRole)->widget());
                    }
                    else if (ciWidgetLayout->itemAt(i, QFormLayout::LabelRole) && ciWidgetLayout->itemAt(i, QFormLayout::FieldRole))
                        globalWidgetLayout->insertRow(600, ciWidgetLayout->itemAt(i, QFormLayout::LabelRole)->widget() , ciWidgetLayout->itemAt(i, QFormLayout::FieldRole)->widget());
                    else if (ciWidgetLayout->itemAt(i, QFormLayout::FieldRole))
                        globalWidgetLayout->insertRow(600, "", ciWidgetLayout->itemAt(i, QFormLayout::FieldRole)->widget());
                }
            }
        }

        QColor transparentColor = color();
        if (mainWindow->ui->indicatorDockWidget->isFloating())
            transparentColor.setAlpha(80);
        QColor textColor = (color().value() < 200) ? QColor(Qt::white) : QColor(Qt::black);
        QPalette palette;
        palette.setColor(QPalette::Window, transparentColor);
        palette.setColor(QPalette::WindowText, textColor);
        privateData->propertiesForm.titleWidget->setPalette(palette);
        privateData->propertiesForm.dialogClose->setIcon(QIcon(":/images/window-close"));
        privateData->propertiesForm.titleLabel->setText(windowTitle());

        privateData->propertiesForm.globalWidget->layout()->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding));

        connect(privateData->propertiesForm.clPushButton, SIGNAL(clicked()), this, SLOT(showColorDialog()));
        connect(privateData->propertiesForm.aoPushButton, SIGNAL(clicked()), privateData->autoOptimizer, SLOT(show()));
        connect(privateData->propertiesForm.srComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setProperties()));
        connect(privateData->propertiesForm.ftsComboBox, SIGNAL(currentIndexChanged(int)) , this, SLOT(setProperties()));
        connect(privateData->propertiesForm.ftsSpinBox, SIGNAL(valueChanged(int)) , this, SLOT(setProperties()));
        connect(privateData->propertiesForm.rdDoubleSpinBox, SIGNAL(valueChanged(double)) , this, SLOT(setProperties()));
        connect(privateData->propertiesForm.adDoubleSpinBox, SIGNAL(valueChanged(double)) , this, SLOT(setProperties()));
        connect(privateData->propertiesForm.uttComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setProperties()));
        connect(privateData->propertiesForm.addPushButton, SIGNAL(clicked(bool)), this, SLOT(addRule()));
        connect(privateData->propertiesForm.editPushButton, SIGNAL(clicked(bool)), this, SLOT(editRule()));
        connect(privateData->propertiesForm.delPushButton, SIGNAL(clicked(bool)), this, SLOT(deleteRule()));
        connect(privateData->propertiesForm.disableButton, SIGNAL(clicked(bool)), this, SLOT(toggleCurrentRuleDisabled()));
        connect(privateData->propertiesForm.defaultPushButton, SIGNAL(clicked(bool)), this, SLOT(loadDefaultRules()));
        connect(privateData->propertiesForm.dialogClose, SIGNAL(clicked()), this, SLOT(removeIndicator()));
        connect(privateData->propertiesForm.tpdComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setProperties()));
        connect(privateData->propertiesForm.tpvDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setProperties()));
        connect(privateData->propertiesForm.sltComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setProperties()));
        connect(privateData->propertiesForm.sldComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setProperties()));
        connect(privateData->propertiesForm.slvDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setProperties()));
        connect(privateData->propertiesForm.emCheckBox, SIGNAL(stateChanged(int)), this, SLOT(setProperties()));
        connect(privateData->propertiesForm.smsCheckBox, SIGNAL(stateChanged(int)), this, SLOT(setProperties()));
        connect(privateData->propertiesForm.tmCheckBox, SIGNAL(stateChanged(int)), this, SLOT(setProperties()));
        connect(privateData->propertiesForm.ruleTreeWidget, SIGNAL(itemSelectionChanged(void)), this, SLOT(toggleRuleButtons(void)));

        //    indicatorGivesSignals = false;
        if (!indicatorGivesSignals)
        {
            QWidget *page1 = privateData->propertiesForm.tabWidget->widget(1);
            privateData->propertiesWidget->layout()->addWidget(page1);
        }

        updateGUI(); //stelt de GUI in op de juiste grootte om geflikker te voorkomen

        if (processMode() != Indicator::Hidden)
        {
            priceChart->indicatorStackWidget()->insertWidget(0, privateData->propertiesWidget);
            priceChart->indicatorStackWidget()->setCurrentIndex(0);
        }
        else
            privateData->propertiesWidget->setVisible(false);
    }
}

void Indicator::setProperties(void)
{
    if (blockSettingsGUISignals())
        return;

    bool indexPlottingDisabled = indexPlot->plottingDisabled();
    indexPlot->setPlottingDisabled(true);
    bool indicatorPlottingDisabled = indicatorPlot->plottingDisabled();
    indicatorPlot->setPlottingDisabled(true);

    QString isString = privateData->propertiesForm.srComboBox->currentText();
    if (isString == tr("Open"))
        indicatorSourceCurve = priceChart->averagePriceOpen;
    else if (isString == tr("High"))
        indicatorSourceCurve = priceChart->averagePriceHigh;
    else if (isString == tr("Low"))
        indicatorSourceCurve = priceChart->averagePriceLow;
    else if (isString == tr("Close"))
        indicatorSourceCurve = priceChart->averagePriceClose;

    for (int i = 0; i < curveList.size(); ++i)
    {
        if (indicatorSourceCurve == priceChart->averagePriceOpen)
            curveList.at(i)->setPlotOffset(0.2);
        else
            curveList.at(i)->setPlotOffset(0.8);
    }

    privateData->globalFilter.type = privateData->propertiesForm.ftsComboBox->currentIndex();;
    privateData->globalFilter.periods = privateData->propertiesForm.ftsSpinBox->value();
    privateData->globalFilter.relativeDifference = privateData->propertiesForm.rdDoubleSpinBox->value() / 100;
    privateData->globalFilter.absoluteDifference = privateData->propertiesForm.adDoubleSpinBox->value();

    if (privateData->globalFilter.type == RuleFilter::NoFilter)
    {
        privateData->propertiesForm.ftsSpinBox->hide();
        privateData->propertiesForm.adDoubleSpinBox->hide();
        privateData->propertiesForm.rdDoubleSpinBox->hide();
    }
    else if (privateData->globalFilter.type == RuleFilter::ClosesInRow)
    {
        privateData->propertiesForm.ftsSpinBox->show();
        privateData->propertiesForm.adDoubleSpinBox->hide();
        privateData->propertiesForm.rdDoubleSpinBox->hide();
    }
    else if (privateData->globalFilter.type == RuleFilter::AbsoluteDifference)
    {
        privateData->propertiesForm.ftsSpinBox->hide();
        privateData->propertiesForm.adDoubleSpinBox->show();
        privateData->propertiesForm.rdDoubleSpinBox->hide();
    }
    else if (privateData->globalFilter.type == RuleFilter::RelativeDifference)
    {
        privateData->propertiesForm.ftsSpinBox->hide();
        privateData->propertiesForm.adDoubleSpinBox->hide();
        privateData->propertiesForm.rdDoubleSpinBox->show();
    }

    if (privateData->propertiesForm.tpdComboBox->currentIndex() == 0)
        privateData->takeProfitOptions.isRelative = true;
    else
        privateData->takeProfitOptions.isRelative = false;

    if (privateData->takeProfitOptions.isRelative)
    {
        privateData->takeProfitOptions.relativeValue = (float)(privateData->propertiesForm.tpvDoubleSpinBox->value() / 100.0);
        privateData->propertiesForm.tpvDoubleSpinBox->setSuffix(" %");
        privateData->propertiesForm.tpvDoubleSpinBox->setMaximum(1000);
    }
    else
    {
        privateData->takeProfitOptions.absoluteValue = (float)privateData->propertiesForm.tpvDoubleSpinBox->value();
        privateData->propertiesForm.tpvDoubleSpinBox->setSuffix(" punten");
        privateData->propertiesForm.tpvDoubleSpinBox->setMaximum(1000);
    }

    privateData->stopLossOptions.type = (StopLossOptions::StopLossType)privateData->propertiesForm.sltComboBox->currentIndex();

    if (privateData->propertiesForm.sldComboBox->currentIndex() == 0)
        privateData->stopLossOptions.isRelative = true;
    else
        privateData->stopLossOptions.isRelative = false;

    if (privateData->stopLossOptions.isRelative)
    {
        privateData->stopLossOptions.relativeValue = (float)(privateData->propertiesForm.slvDoubleSpinBox->value() / 100.0);
        privateData->propertiesForm.slvDoubleSpinBox->setSuffix(" %");
        privateData->propertiesForm.slvDoubleSpinBox->setMaximum(100);
    }
    else
    {
        privateData->stopLossOptions.absoluteValue = (float)privateData->propertiesForm.slvDoubleSpinBox->value();
        privateData->propertiesForm.slvDoubleSpinBox->setSuffix(" punten");
        privateData->propertiesForm.slvDoubleSpinBox->setMaximum(1000);
    }

    if (privateData->propertiesForm.tmCheckBox->isChecked())
        showTransactionMarkers = 1;
    else
        showTransactionMarkers = 0;

    if (privateData->propertiesForm.uttComboBox->currentIndex() == 0)
    {
        privateData->useLongTransactions = true;
        privateData->useShortTransactions = true;
    }
    else if (privateData->propertiesForm.uttComboBox->currentIndex() == 1)
    {
        privateData->useLongTransactions = true;
        privateData->useShortTransactions = false;
    }
    else if (privateData->propertiesForm.uttComboBox->currentIndex() == 2)
    {
        privateData->useLongTransactions = false;
        privateData->useShortTransactions = true;
    }
    else
    {
        privateData->useLongTransactions = false;
        privateData->useShortTransactions = false;
    }

    privateData->useEmailNotification = privateData->propertiesForm.emCheckBox->isChecked();
    privateData->useSmsNotification = privateData->propertiesForm.smsCheckBox->isChecked();

    writeSettingsPrivate();
    writeRules();

    plotCurve();
    executeTransactionRules();

    emit propertiesChanged();

    indicatorPlot->setPlottingDisabled(indicatorPlottingDisabled);
    indicatorPlot->replot();
    indexPlot->setPlottingDisabled(indexPlottingDisabled);
    indexPlot->replot();
}

void Indicator::updateGUI(void)
{
    bool block = blockSettingsGUISignals();
    setBlockSettingsGUISignals(true);

    if (privateData->takeProfitOptions.isRelative)
        privateData->propertiesForm.tpdComboBox->setCurrentIndex(0);
    else
        privateData->propertiesForm.tpdComboBox->setCurrentIndex(1);

    if (privateData->takeProfitOptions.isRelative)
    {
        privateData->propertiesForm.tpvDoubleSpinBox->setValue(privateData->takeProfitOptions.relativeValue * 100.0);
        privateData->propertiesForm.tpvDoubleSpinBox->setSuffix(" %");
        privateData->propertiesForm.tpvDoubleSpinBox->setMaximum(1000);
    }
    else
    {
        privateData->propertiesForm.tpvDoubleSpinBox->setValue(privateData->takeProfitOptions.absoluteValue);
        privateData->propertiesForm.tpvDoubleSpinBox->setSuffix(" punten");
        privateData->propertiesForm.tpvDoubleSpinBox->setMaximum(1000);
    }

    privateData->propertiesForm.sltComboBox->setCurrentIndex((int)privateData->stopLossOptions.type);
    if (privateData->stopLossOptions.isRelative == true)
        privateData->propertiesForm.sldComboBox->setCurrentIndex(0);
    else
        privateData->propertiesForm.sldComboBox->setCurrentIndex(1);

    if (privateData->stopLossOptions.isRelative)
    {
        privateData->propertiesForm.slvDoubleSpinBox->setValue(privateData->stopLossOptions.relativeValue * 100.0);
        privateData->propertiesForm.slvDoubleSpinBox->setSuffix(" %");
        privateData->propertiesForm.slvDoubleSpinBox->setMaximum(100);
    }
    else
    {
        privateData->propertiesForm.slvDoubleSpinBox->setValue(privateData->stopLossOptions.absoluteValue);
        privateData->propertiesForm.slvDoubleSpinBox->setSuffix(" punten");
        privateData->propertiesForm.slvDoubleSpinBox->setMaximum(1000);
    }

    privateData->propertiesForm.emCheckBox->setChecked(privateData->useEmailNotification);
    privateData->propertiesForm.smsCheckBox->setChecked(privateData->useSmsNotification);
    privateData->propertiesForm.addPushButton->setDisabled(true);
    privateData->propertiesForm.editPushButton->setDisabled(true);
    privateData->propertiesForm.delPushButton->setDisabled(true);
    privateData->propertiesForm.disableButton->setDisabled(true);

    if (indicatorSourceCurve == priceChart->averagePriceOpen)
        privateData->propertiesForm.srComboBox->setCurrentIndex(Open);
    else if (indicatorSourceCurve == priceChart->averagePriceHigh)
        privateData->propertiesForm.srComboBox->setCurrentIndex(High);
    else if (indicatorSourceCurve == priceChart->averagePriceLow)
        privateData->propertiesForm.srComboBox->setCurrentIndex(Low);
    else
        privateData->propertiesForm.srComboBox->setCurrentIndex(Close);

    privateData->propertiesForm.ftsComboBox->setCurrentIndex(privateData->globalFilter.type);
    privateData->propertiesForm.ftsSpinBox->hide();
    privateData->propertiesForm.ftsSpinBox->setValue(privateData->globalFilter.periods);
    privateData->propertiesForm.rdDoubleSpinBox->hide();
    privateData->propertiesForm.rdDoubleSpinBox->setValue(privateData->globalFilter.relativeDifference * 100.0);
    privateData->propertiesForm.adDoubleSpinBox->hide();
    privateData->propertiesForm.adDoubleSpinBox->setValue(privateData->globalFilter.absoluteDifference);

    if (privateData->globalFilter.type == RuleFilter::ClosesInRow)
        privateData->propertiesForm.ftsSpinBox->show();
    else if (privateData->globalFilter.type == RuleFilter::AbsoluteDifference)
        privateData->propertiesForm.adDoubleSpinBox->show();
    else if (privateData->globalFilter.type == RuleFilter::RelativeDifference)
        privateData->propertiesForm.rdDoubleSpinBox->show();

    if (privateData->useLongTransactions && privateData->useShortTransactions)
        privateData->propertiesForm.uttComboBox->setCurrentIndex(0);
    else if (privateData->useLongTransactions)
        privateData->propertiesForm.uttComboBox->setCurrentIndex(1);
    else if (privateData->useShortTransactions)
        privateData->propertiesForm.uttComboBox->setCurrentIndex(2);
    else
        privateData->propertiesForm.uttComboBox->setCurrentIndex(3);

    if (showTransactionMarkers)
        privateData->propertiesForm.tmCheckBox->setChecked(true);
    else
        privateData->propertiesForm.tmCheckBox->setChecked(false);

    if (privateData->colorButtonHidden)
        privateData->propertiesForm.clPushButton->setHidden(true);

    if (privateData->indicatorSourceComboBoxHidden)
    {
        privateData->propertiesForm.srComboBox->setHidden(true);
        privateData->propertiesForm.srLabel->setHidden(true);
    }

    bool rulesAvailable;
    if (privateData->longRuleList->items().isEmpty() && privateData->shortRuleList->items().isEmpty() && privateData->exitRuleList->items().isEmpty())
        rulesAvailable = false;
    else
        rulesAvailable = true;

    privateData->propertiesForm.wLabel->setVisible(!rulesAvailable);
    privateData->propertiesForm.tpwLabel->setVisible(!rulesAvailable);
    privateData->propertiesForm.slwLabel->setVisible(!rulesAvailable);
    privateData->propertiesForm.tradeRulesEnabledWidget->setVisible(rulesAvailable);
    privateData->propertiesForm.takeProfitsRulesEnabledWidget->setVisible(rulesAvailable);
    privateData->propertiesForm.stopLossRulesEnabledWidget->setVisible(rulesAvailable);

    clearRuleTree();
    fillRuleTree();

    for (int i = 0; i < privateData->propertiesForm.tabWidget->count(); i++)
        privateData->propertiesForm.tabWidget->widget(i)->resize(0, 0);
    privateData->propertiesWidget->resize(0, 0);

    setBlockSettingsGUISignals(block);
}

void Indicator::fillCurveCombos(void)
{
    privateData->rulePopupForm.scComboBox->clear();
    privateData->rulePopupForm.rcComboBox->clear();
    privateData->rulePopupForm.rcComboBox->addItem(tr("Waarde"));

    //    privateData->rulePopupForm.scComboBox->addItem(priceChart->priceCurve->title().text().section(" ", 0, 1));
    //    privateData->rulePopupForm.rcComboBox->addItem(priceChart->priceCurve->title().text().section(" ", 0, 1));
    privateData->rulePopupForm.scComboBox->addItem(priceChart->averagePriceOpen->title().text());
    privateData->rulePopupForm.rcComboBox->addItem(priceChart->averagePriceOpen->title().text());
    privateData->rulePopupForm.scComboBox->addItem(priceChart->averagePriceClose->title().text());
    privateData->rulePopupForm.rcComboBox->addItem(priceChart->averagePriceClose->title().text());
    privateData->rulePopupForm.scComboBox->addItem(priceChart->averagePriceHigh->title().text());
    privateData->rulePopupForm.rcComboBox->addItem(priceChart->averagePriceHigh->title().text());
    privateData->rulePopupForm.scComboBox->addItem(priceChart->averagePriceLow->title().text());
    privateData->rulePopupForm.rcComboBox->addItem(priceChart->averagePriceLow->title().text());

    //    if (tradeSourceCurve != priceChart->priceCurveOpen)
    //    {
    //        QModelIndex index = privateData->rulePopupForm.scComboBox->model()->index(2, 0);
    //        privateData->rulePopupForm.scComboBox->model()->setData(index, QVariant(0), Qt::UserRole - 1);

    //        index = privateData->rulePopupForm.rcComboBox->model()->index(3, 0);
    //        privateData->rulePopupForm.rcComboBox->model()->setData(index, QVariant(0), Qt::UserRole - 1);

    //        index = privateData->rulePopupForm.scComboBox->model()->index(3, 0);
    //        privateData->rulePopupForm.scComboBox->model()->setData(index, QVariant(0), Qt::UserRole - 1);

    //        index = privateData->rulePopupForm.rcComboBox->model()->index(4, 0);
    //        privateData->rulePopupForm.rcComboBox->model()->setData(index, QVariant(0), Qt::UserRole - 1);

    //        index = privateData->rulePopupForm.scComboBox->model()->index(4, 0);
    //        privateData->rulePopupForm.scComboBox->model()->setData(index, QVariant(0), Qt::UserRole - 1);

    //        index = privateData->rulePopupForm.rcComboBox->model()->index(5, 0);
    //        privateData->rulePopupForm.rcComboBox->model()->setData(index, QVariant(0), Qt::UserRole - 1);
    //    }

    for (int i = 0; i < priceChart->indicatorList.size(); ++i)
    {
        QList <PlotCurve *> *curveList = &priceChart->indicatorList.at(i)->curveList;
        for (int j = 0; j < curveList->size(); ++j)
        {
            privateData->rulePopupForm.scComboBox->addItem(priceChart->indicatorList.at(i)->windowTitle() + "->" + curveList->at(j)->title().text());
            privateData->rulePopupForm.rcComboBox->addItem(priceChart->indicatorList.at(i)->windowTitle() + "->" + curveList->at(j)->title().text());
        }
    }
}

//void Indicator::fillTradeSourceCombo(void)
//{

//    privateData->propertiesForm.tsrComboBox->blockSignals(true);
//    privateData->propertiesForm.srComboBox->blockSignals(true);

//    if (ruleListcontainsCloseIndex() || privateData->propertiesForm.srComboBox->currentIndex() > Open)
//    {
//        QModelIndex index = privateData->propertiesForm.tsrComboBox->model()->index(0, 0);
//        privateData->propertiesForm.tsrComboBox->model()->setData(index, QVariant(0), Qt::UserRole - 1);
//    }
//    else
//    {
//        QModelIndex index = privateData->propertiesForm.tsrComboBox->model()->index(0, 0);
//        privateData->propertiesForm.tsrComboBox->model()->setData(index, QVariant(1 | 32), Qt::UserRole - 1);
//    }

//    privateData->propertiesForm.tsrComboBox->blockSignals(false);
//    privateData->propertiesForm.srComboBox->blockSignals(false);
//}

//void Indicator::fillIndicatorSourceCombo(void)
//{
//    privateData->propertiesForm.tsrComboBox->blockSignals(true);
//    privateData->propertiesForm.srComboBox->blockSignals(true);

//    if (privateData->propertiesForm.tsrComboBox->currentIndex() == Open)
//    {
//        QModelIndex index = privateData->propertiesForm.srComboBox->model()->index(1, 0);
//        privateData->propertiesForm.srComboBox->model()->setData(index, QVariant(0), Qt::UserRole - 1);

//        index = privateData->propertiesForm.srComboBox->model()->index(2, 0);
//        privateData->propertiesForm.srComboBox->model()->setData(index, QVariant(0), Qt::UserRole - 1);

//        index = privateData->propertiesForm.srComboBox->model()->index(3, 0);
//        privateData->propertiesForm.srComboBox->model()->setData(index, QVariant(0), Qt::UserRole - 1);
//    }
//    else
//    {
//        QModelIndex index = privateData->propertiesForm.srComboBox->model()->index(1, 0);
//        privateData->propertiesForm.srComboBox->model()->setData(index, QVariant(1 | 32), Qt::UserRole - 1);

//        index = privateData->propertiesForm.srComboBox->model()->index(2, 0);
//        privateData->propertiesForm.srComboBox->model()->setData(index, QVariant(1 | 32), Qt::UserRole - 1);

//        index = privateData->propertiesForm.srComboBox->model()->index(3, 0);
//        privateData->propertiesForm.srComboBox->model()->setData(index, QVariant(1 | 32), Qt::UserRole - 1);
//    }

//    privateData->propertiesForm.tsrComboBox->blockSignals(false);
//    privateData->propertiesForm.srComboBox->blockSignals(false);
//}

void Indicator::clearRuleTree(QList<QTreeWidgetItem *> *children)
{
    if (!privateData->propertiesForm.tradeWidget)
        return;

    if (!children)
    {
        QList<QTreeWidgetItem *> childList = privateData->propertiesForm.ruleTreeWidget->topLevelItem(0)->takeChildren();
        clearRuleTree(&childList);
        childList = privateData->propertiesForm.ruleTreeWidget->topLevelItem(1)->takeChildren();
        clearRuleTree(&childList);
        childList = privateData->propertiesForm.ruleTreeWidget->topLevelItem(2)->takeChildren();
        clearRuleTree(&childList);
        return;
    }

    if (children->isEmpty())
        return;

    for (int i = 0; i < children->size(); i++)
    {
        QTreeWidgetItem *child = children->at(i);
        QList<QTreeWidgetItem *> childList = child->takeChildren();
        clearRuleTree(&childList);
        delete child;

    }
}

void Indicator::fillRuleTree(RuleTreeWidgetItem *parentItem, const QVector<Rule *> *list)
{
    if (!privateData->propertiesForm.tradeWidget)
        return;

    if (!parentItem && !list)
    {
        fillRuleTree(static_cast<RuleTreeWidgetItem *>(privateData->propertiesForm.ruleTreeWidget->topLevelItem(0)), &privateData->longRuleList->items());
        fillRuleTree(static_cast<RuleTreeWidgetItem *>(privateData->propertiesForm.ruleTreeWidget->topLevelItem(1)), &privateData->shortRuleList->items());
        fillRuleTree(static_cast<RuleTreeWidgetItem *>(privateData->propertiesForm.ruleTreeWidget->topLevelItem(2)), &privateData->exitRuleList->items());

        return;
    }

    if (parentItem && list)
    {
        for (int i = 0; i < list->size(); i++)
        {
            Rule *rule = const_cast<Rule *>(list->at(i));
            RuleTreeWidgetItem *newItem = new RuleTreeWidgetItem;
            newItem->setText(0 , rule->humanReadableTitle);
            newItem->rule = rule;
            if (!rule->enabled)
                newItem->setForeground(0, QPalette().brush(QPalette::Disabled, QPalette::Text));
            parentItem->addChild(newItem);
            privateData->propertiesForm.ruleTreeWidget->expandItem(parentItem);
            privateData->propertiesForm.ruleTreeWidget->expandItem(newItem);

            if (!rule->children.isEmpty())
                fillRuleTree(newItem, &rule->children);
        }

        privateData->propertiesForm.ruleTreeWidget->resizeColumnToContents(0);

        return;
    }
}

//void Indicator::clearRuleTree(const QList<IndicatorRule *> &ruleList)
//{
//    if (ruleList.isEmpty())
//    {
//        if (&ruleList == &longRuleList || &ruleList == &shortRuleList || &ruleList == &exitRuleList)
//            return;

//        clearRuleTree(longRuleList);
//        clearRuleTree(shortRuleList);
//        clearRuleTree(exitRuleList);
//        return;
//    }

//    for (int i = 0; i < ruleList.size(); ++i)
//    {
//        IndicatorRule *rule = ruleList.at(i);
//        if (!rule->list.isEmpty())
//            clearRuleTree(rule->list);
//        delete rule->treeItem;
//        rule->treeItem = NULL;
//    }
//}

//void Indicator::fillRuleTree(QTreeWidgetItem *item, QList <IndicatorRule *> *list)
//{
//    if (!privateData->propertiesForm.tradeWidget)
//        return;

//    if (!item)
//    {
//        clearRuleTree();
//        for (int i = 0; i < longRuleList.size(); ++ i)
//        {
//            if (longRuleList.at(i)->treeItem)
//            {
//                fillRuleTree(longRuleList.at(i)->treeItem, &longRuleList.at(i)->list);
//                continue;
//            }

//            QTreeWidgetItem *newItem = new QTreeWidgetItem;
//            if (!longRuleList.at(i)->enabled)
//                newItem->setForeground(0, QPalette().brush(QPalette::Disabled, QPalette::Text));

//            newItem->setText(0, longRuleList.at(i)->title);
//            longRuleList.at(i)->treeItem = newItem;
//            privateData->propertiesForm.ruleTreeWidget->topLevelItem(0)->addChild(newItem);
//            privateData->propertiesForm.ruleTreeWidget->expandItem(privateData->propertiesForm.ruleTreeWidget->topLevelItem(0));
//            privateData->propertiesForm.ruleTreeWidget->expandItem(newItem);
//            fillRuleTree(newItem, &longRuleList.at(i)->list);
//        }

//        for (int i = 0; i < shortRuleList.size(); ++ i)
//        {
//            if (shortRuleList.at(i)->treeItem)
//            {
//                fillRuleTree(shortRuleList.at(i)->treeItem, &shortRuleList.at(i)->list);
//                continue;
//            }

//            QTreeWidgetItem *newItem = new QTreeWidgetItem;
//            if (!shortRuleList.at(i)->enabled)
//                newItem->setForeground(0, QPalette().brush(QPalette::Disabled, QPalette::Text));

//            newItem->setText(0, shortRuleList.at(i)->title);
//            shortRuleList.at(i)->treeItem = newItem;
//            privateData->propertiesForm.ruleTreeWidget->topLevelItem(1)->addChild(newItem);
//            privateData->propertiesForm.ruleTreeWidget->expandItem(privateData->propertiesForm.ruleTreeWidget->topLevelItem(1));
//            privateData->propertiesForm.ruleTreeWidget->expandItem(newItem);
//            fillRuleTree(newItem, &shortRuleList.at(i)->list);
//        }

//        for (int i = 0; i < exitRuleList.size(); ++ i)
//        {
//            if (exitRuleList.at(i)->treeItem)
//            {
//                fillRuleTree(exitRuleList.at(i)->treeItem, &exitRuleList.at(i)->list);
//                continue;
//            }

//            QTreeWidgetItem *newItem = new QTreeWidgetItem;
//            if (!exitRuleList.at(i)->enabled)
//                newItem->setForeground(0, QPalette().brush(QPalette::Disabled, QPalette::Text));

//            newItem->setText(0, exitRuleList.at(i)->title);
//            exitRuleList.at(i)->treeItem = newItem;
//            privateData->propertiesForm.ruleTreeWidget->topLevelItem(2)->addChild(newItem);
//            privateData->propertiesForm.ruleTreeWidget->expandItem(privateData->propertiesForm.ruleTreeWidget->topLevelItem(2));
//            privateData->propertiesForm.ruleTreeWidget->expandItem(newItem);
//            fillRuleTree(newItem, &exitRuleList.at(i)->list);
//        }

//        return;
//    }

//    for (int i = 0; i < list->size(); ++i)
//    {
//        if (list->at(i)->treeItem)
//        {
//            fillRuleTree(list->at(i)->treeItem, &list->at(i)->list);
//            continue;
//        }

//        QTreeWidgetItem *newItem = new QTreeWidgetItem;
//        if (!list->at(i)->enabled)
//            newItem->setForeground(0, QPalette().brush(QPalette::Disabled, QPalette::Text));

//        newItem->setText(0, list->at(i)->title);
//        list->at(i)->treeItem = newItem;
//        item->addChild(newItem);
//        privateData->propertiesForm.ruleTreeWidget->expandItem(newItem);
//        fillRuleTree(newItem, &list->at(i)->list);
//    }

//    privateData->propertiesForm.ruleTreeWidget->resizeColumnToContents(0);
//}

void Indicator::toggleValueSpinBox(int index)
{
    if (index == 0)
        privateData->rulePopupForm.rcDoubleSpinBox->show();
    else
        privateData->rulePopupForm.rcDoubleSpinBox->hide();
}

void Indicator::toggleReferenceComboBox(int index)
{
    if (index == 4 || index == 5)
    {
        privateData->rulePopupForm.rcLabel->hide();
        privateData->rulePopupForm.rcComboBox->hide();
        privateData->rulePopupForm.rcDoubleSpinBox->hide();
    }
    else
    {
        privateData->rulePopupForm.rcLabel->show();
        privateData->rulePopupForm.rcComboBox->show();
        toggleValueSpinBox(privateData->rulePopupForm.rcComboBox->currentIndex());
    }
}

void Indicator::toggleRuleButtons(void)
{
    QTreeWidgetItem *item = privateData->propertiesForm.ruleTreeWidget->currentItem();
    if (!item)
    {
        privateData->propertiesForm.addPushButton->setDisabled(true);
        privateData->propertiesForm.delPushButton->setDisabled(true);
        privateData->propertiesForm.editPushButton->setDisabled(true);
        privateData->propertiesForm.disableButton->setDisabled(true);
    }
    else
    {
        if (item == privateData->propertiesForm.ruleTreeWidget->topLevelItem(0) ||
            item == privateData->propertiesForm.ruleTreeWidget->topLevelItem(1) ||
            item == privateData->propertiesForm.ruleTreeWidget->topLevelItem(2))
        {

            privateData->propertiesForm.addPushButton->setDisabled(false);
            privateData->propertiesForm.delPushButton->setDisabled(true);
            privateData->propertiesForm.editPushButton->setDisabled(true);
            privateData->propertiesForm.disableButton->setDisabled(true);
        }
        else
        {
            privateData->propertiesForm.addPushButton->setDisabled(false);
            privateData->propertiesForm.delPushButton->setDisabled(false);
            privateData->propertiesForm.editPushButton->setDisabled(false);
            privateData->propertiesForm.disableButton->setDisabled(false);
        }
    }
}

void Indicator::setChildRulesEnabled(QTreeWidgetItem *parent, bool enabled)
{
    for (int i = 0; i < parent->childCount(); ++i)
    {
        RuleTreeWidgetItem *child = dynamic_cast<RuleTreeWidgetItem *>(parent->child(i));
        if (!child || !child->rule)
            continue;

        setChildRulesEnabled(child, enabled);

        child->rule->enabled = enabled;
        child->setForeground(0, QPalette().brush(enabled ? QPalette::Active : QPalette::Disabled, QPalette::Text));
    }
}

void Indicator::setParentRulesEnabled(QTreeWidgetItem *child, bool enabled)
{
    if (!child)
        return;

    RuleTreeWidgetItem *parent = dynamic_cast<RuleTreeWidgetItem *>(child->parent());
    if (!parent || !parent->rule)
        return;

    if (parent == privateData->propertiesForm.ruleTreeWidget->topLevelItem(0) ||
        parent == privateData->propertiesForm.ruleTreeWidget->topLevelItem(1) ||
        parent == privateData->propertiesForm.ruleTreeWidget->topLevelItem(2))
    {
        return;
    }

    setParentRulesEnabled(parent, enabled);

    parent->rule->enabled = enabled;
    parent->setForeground(0, QPalette().brush(enabled ? QPalette::Active : QPalette::Disabled, QPalette::Text));
}

void Indicator::toggleCurrentRuleDisabled(void)
{
    RuleTreeWidgetItem *current = dynamic_cast<RuleTreeWidgetItem *>(privateData->propertiesForm.ruleTreeWidget->currentItem());
    if (!current || !current->rule)
        return;

    if (current == privateData->propertiesForm.ruleTreeWidget->topLevelItem(0) ||
        current == privateData->propertiesForm.ruleTreeWidget->topLevelItem(1) ||
        current == privateData->propertiesForm.ruleTreeWidget->topLevelItem(2))
    {
        return;
    }
    current->rule->enabled = !current->rule->enabled;
    current->setForeground(0, QPalette().brush(current->rule->enabled ? QPalette::Active : QPalette::Disabled, QPalette::Text));
    setChildRulesEnabled(current, current->rule->enabled);
    if (current->rule->enabled)
        setParentRulesEnabled(current, true);

    setProperties();
}

void Indicator::loadDefaultRules(void)
{
    clearRuleTree();
    fillRuleTree();
}

void Indicator::writeRules(void)
{
    if (!autoRestore())
        return;

    privateData->longRuleList->write(QString("Strategie/Charts/") + priceChart->configGroupKey() + QString("/Indicators/") + configGroupKey() + "/LongRuleList");
    privateData->shortRuleList->write(QString("Strategie/Charts/") + priceChart->configGroupKey() + QString("/Indicators/") + configGroupKey() + "/ShortRuleList");
    privateData->exitRuleList->write(QString("Strategie/Charts/") + priceChart->configGroupKey() + QString("/Indicators/") + configGroupKey() + "/ExitRuleList");

    return;
}

void Indicator::readRules(void)
{
    while (!settings.group().isEmpty())
        settings.endGroup();

    settings.beginGroup(QString("Strategie/Charts/") + priceChart->configGroupKey() + QString("/Indicators/") + configGroupKey());
    if (settings.allKeys().count() == 0)
        loadDefaultRules();
    else
    {
        privateData->longRuleList->read(QString("Strategie/Charts/") + priceChart->configGroupKey() + QString("/Indicators/") + configGroupKey() + "/LongRuleList");
        privateData->shortRuleList->read(QString("Strategie/Charts/") + priceChart->configGroupKey() + QString("/Indicators/") + configGroupKey() + "/ShortRuleList");
        privateData->exitRuleList->read(QString("Strategie/Charts/") + priceChart->configGroupKey() + QString("/Indicators/") + configGroupKey() + "/ExitRuleList");
    }

    return;
}

void Indicator::addRule(void)
{
    QTreeWidgetItem *currentItem = privateData->propertiesForm.ruleTreeWidget->currentItem();
    if (!currentItem)
    {
        QMessageBox messageBox(this);
        messageBox.setText(tr("Selecteer eerst waar de nieuwe voorwaarde ingevoegd moet worden."));
        messageBox.exec();
        return;
    }

    QDialog rulePopupDialog(this);
    privateData->rulePopupForm.setupUi(&rulePopupDialog);
    privateData->rulePopupForm.coCheckBox->setHidden(true);
    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &rulePopupDialog);
    connect(&buttonBox, SIGNAL(accepted()), &rulePopupDialog, SLOT(accept()));
    connect(&buttonBox, SIGNAL(rejected()), &rulePopupDialog, SLOT(reject()));
    rulePopupDialog.layout()->addWidget(&buttonBox);
    fillCurveCombos();
    connect(privateData->rulePopupForm.rcComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(toggleValueSpinBox(int)));
    connect(privateData->rulePopupForm.rulComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(toggleReferenceComboBox(int)));

    int retval = rulePopupDialog.exec();
    if (retval == QDialog::Rejected)
        return;

    QString ruleString;
    QStringList split = privateData->rulePopupForm.scComboBox->currentText().split("->", QString::SkipEmptyParts);
    if (split.count() < 1 || split.count() > 2)
    {
        qWarning() << "Indicator::addRule(): " << tr("Ongeldige bron indicator en/of bron curve");
        QMessageBox messageBox(this);
        messageBox.setText(tr("Ongeldige combinatie van bron indicator en/of bron curve"));
        messageBox.exec();
        return;
    }

    if (split.count() == 1)
        ruleString.append("SourceIndicator=NULL;SourceCurve=" + split.at(0) + ";");
    else
        ruleString.append("SourceIndicator=" + split.at(0) + ";SourceCurve=" + split.at(1) + ";");

    split = privateData->rulePopupForm.rcComboBox->currentText().split("->", QString::SkipEmptyParts);
    if (split.count() < 1 || split.count() > 2)
    {
        qWarning() << "Indicator::addRule(): " << tr("Ongeldige referentie indicator en/of referentie curve");
        QMessageBox messageBox(this);
        messageBox.setText(tr("Ongeldige combinatie van referentie indicator en/of referentie curve"));
        messageBox.exec();
        return;
    }

    if (split.count() == 1)
        ruleString.append("ReferenceIndicator=NULL;ReferenceCurve=" + (split.at(0).compare(tr("Waarde"), Qt::CaseInsensitive) == 0 ? "NULL" : split.at(0)) + ";");
    else
        ruleString.append("ReferenceIndicator=" + split.at(0) + ";ReferenceCurve=" + (split.at(1).compare(tr("Waarde"), Qt::CaseInsensitive) == 0 ? "NULL" : split.at(1)) + ";");

    ruleString.append("ReferenceValue=" + QString::number(privateData->rulePopupForm.rcDoubleSpinBox->value()) + ";" +
                      "Offset=" + QString::number(privateData->rulePopupForm.ofSpinBox->value()) + ";" +
                      "CompareType=" + Rule::compareTypeTitle((Rule::CompareType) privateData->rulePopupForm.rulComboBox->currentIndex()) + ";" +
                      "LogicalOperator=" + Rule::logicalOperatorTitle((Rule::LogicalOperator) privateData->rulePopupForm.logComboBox->currentIndex()) + ";" +
                      "PreviousTransactionType=" + Rule::previousTransactionTypeTitle((TransactionType) privateData->rulePopupForm.ptComboBox->currentIndex()) + ";Enabled=1");

    QTreeWidgetItem *topLevelItem = currentItem;
    QTreeWidgetItem *previousItem = currentItem;
    while ((previousItem = previousItem->parent()))
        topLevelItem = previousItem;

    Rule *newRule = NULL;
    Rule *currentRule = NULL;
    RuleList *ruleList = NULL;
    if (dynamic_cast<RuleTreeWidgetItem *>(currentItem))
        currentRule = dynamic_cast<RuleTreeWidgetItem *>(currentItem)->rule;

    if (privateData->propertiesForm.ruleTreeWidget->topLevelItem(0) == topLevelItem)
    {
        newRule = privateData->longRuleList->addRule(ruleString, currentRule);
        ruleList = privateData->longRuleList;
    }
    else if (privateData->propertiesForm.ruleTreeWidget->topLevelItem(1) == topLevelItem)
    {
        newRule = privateData->shortRuleList->addRule(ruleString, currentRule);
        ruleList = privateData->shortRuleList;
    }
    else if (privateData->propertiesForm.ruleTreeWidget->topLevelItem(2) == topLevelItem)
    {
        newRule = privateData->exitRuleList->addRule(ruleString, currentRule);
        ruleList = privateData->exitRuleList;
    }

    if (!newRule)
    {
        qWarning() << "Indicator::addRule(): Failed to find the container of the current selected item";
        QMessageBox messageBox(this);
        messageBox.setText(tr("Kon de nieuwe voorwaarde niet toevoegen aan de lijst: hoofdlijst niet gevonden"));
        messageBox.exec();
        return;
    }

    RuleTreeWidgetItem *child = new RuleTreeWidgetItem;
    child->rule = newRule;
    child->setText(0, newRule->humanReadableTitle);
    currentItem->addChild(child);

    privateData->propertiesForm.ruleTreeWidget->expandItem(currentItem);
    privateData->propertiesForm.tradeRulesEnabledWidget->setVisible(true);
    privateData->propertiesForm.takeProfitsRulesEnabledWidget->setVisible(true);
    privateData->propertiesForm.stopLossRulesEnabledWidget->setVisible(true);
    privateData->propertiesForm.wLabel->setVisible(false);
    privateData->propertiesForm.tpwLabel->setVisible(false);
    privateData->propertiesForm.slwLabel->setVisible(false);
    privateData->propertiesForm.ruleTreeWidget->resizeColumnToContents(0);

    setProperties();
}

void Indicator::editRule(void)
{
    RuleTreeWidgetItem *currentItem = dynamic_cast<RuleTreeWidgetItem *>(privateData->propertiesForm.ruleTreeWidget->currentItem());
    if (!currentItem ||
        currentItem == privateData->propertiesForm.ruleTreeWidget->topLevelItem(0) ||
        currentItem == privateData->propertiesForm.ruleTreeWidget->topLevelItem(1) ||
        currentItem == privateData->propertiesForm.ruleTreeWidget->topLevelItem(2))
    {
        QMessageBox messageBox(this);
        messageBox.setText(tr("Selecteer eerst een voorwaarde die bewerkt moet worden."));
        messageBox.exec();
        return;
    }


    Rule *rule = currentItem->rule;
    if (!rule || !rule->isValid())
    {
        qWarning() << "Indicator::editRule(): current item contains no valid rule";
        QMessageBox messageBox(this);
        messageBox.setText(tr("Deze voorwaarde kan niet bewerkt worden door een onbekende fout, verwijder deze voorwaarde."));
        messageBox.exec();
        return;
    }

    QDialog *rulePopupDialog = new QDialog(this);
    privateData->rulePopupForm.setupUi(rulePopupDialog);
    privateData->rulePopupForm.coCheckBox->setHidden(true);
    connect(privateData->rulePopupForm.rcComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(toggleValueSpinBox(int)));
    connect(privateData->rulePopupForm.rulComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(toggleReferenceComboBox(int)));
    fillCurveCombos();

    QString sourceString;
    if (rule->sourceIndicator && rule->sourceCurve)
        sourceString = QString(rule->sourceIndicator->windowTitle() + "->" + rule->sourceCurve->title().text());
    else if (rule->sourceCurve)
        sourceString = rule->sourceCurve->title().text();

    for (int i = 0; i < privateData->rulePopupForm.scComboBox->count(); ++i)
    {
        if (privateData->rulePopupForm.scComboBox->itemText(i) == sourceString)
        {
            privateData->rulePopupForm.scComboBox->setCurrentIndex(i);
            break;
        }
    }

    QString referenceString;
    if (rule->referenceIndicator && rule->referenceCurve)
        referenceString = QString(rule->referenceIndicator->windowTitle() + "->" + rule->referenceCurve->title().text());
    else if (rule->referenceCurve)
        referenceString = rule->referenceCurve->title().text();
    else
        referenceString = tr("Waarde");

    for (int i = 0; i < privateData->rulePopupForm.rcComboBox->count(); ++i)
    {
        if (privateData->rulePopupForm.rcComboBox->itemText(i) == referenceString)
        {
            privateData->rulePopupForm.rcComboBox->setCurrentIndex(i);
            break;
        }
    }

    privateData->rulePopupForm.rcDoubleSpinBox->setValue(rule->referenceValue);
    privateData->rulePopupForm.ofSpinBox->setValue(rule->offset);
    privateData->rulePopupForm.rulComboBox->setCurrentIndex(rule->compareType);
    privateData->rulePopupForm.logComboBox->setCurrentIndex(rule->logicalOperator);
    privateData->rulePopupForm.ptComboBox->setCurrentIndex(rule->previousTransactionType);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, rulePopupDialog);
    buttonBox->setCenterButtons(false);
    connect(buttonBox, SIGNAL(rejected()), rulePopupDialog, SLOT(reject()));

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addWidget(buttonBox);

    QGridLayout *popupLayout = qobject_cast<QGridLayout *>(rulePopupDialog->layout());
    popupLayout->addLayout(vLayout, 100, 1, 1, 2);

    connect(privateData->rulePopupForm.logComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(rulePopupApplyClicked()));
    connect(privateData->rulePopupForm.scComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(rulePopupApplyClicked()));
    connect(privateData->rulePopupForm.rulComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(rulePopupApplyClicked()));
    connect(privateData->rulePopupForm.rcComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(rulePopupApplyClicked()));
    connect(privateData->rulePopupForm.rcDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(rulePopupApplyClicked()));
    connect(privateData->rulePopupForm.ptComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(rulePopupApplyClicked()));
    connect(privateData->rulePopupForm.ofSpinBox, SIGNAL(valueChanged(int)), this, SLOT(rulePopupApplyClicked()));

    rulePopupDialog->exec();
}

void Indicator::rulePopupApplyClicked(void)
{
    RuleTreeWidgetItem *currentItem = dynamic_cast<RuleTreeWidgetItem *>(privateData->propertiesForm.ruleTreeWidget->currentItem());
    if (!currentItem ||
        currentItem == privateData->propertiesForm.ruleTreeWidget->topLevelItem(0) ||
        currentItem == privateData->propertiesForm.ruleTreeWidget->topLevelItem(1) ||
        currentItem == privateData->propertiesForm.ruleTreeWidget->topLevelItem(2))
    {
        QMessageBox messageBox(this);
        messageBox.setText(tr("Geen voorwaarde geselecteerd."));
        messageBox.exec();
        return;
    }

    Rule *currentRule = currentItem->rule;
    if (!currentRule)
    {
        qWarning() << "Indicator::rulePopupApplyClicked(): current item contains no rule";
        QMessageBox messageBox(this);
        messageBox.setText(tr("Huidige selectie bevat geen voorwaarde"));
        messageBox.exec();
        return;
    }

    Rule *parentRule = NULL;
    RuleTreeWidgetItem *parentItem = dynamic_cast<RuleTreeWidgetItem *>(currentItem->parent());
    if (parentItem)
        parentRule = parentItem->rule;

    QString ruleString;
    QStringList split = privateData->rulePopupForm.scComboBox->currentText().split("->", QString::SkipEmptyParts);
    if (split.count() < 1 || split.count() > 2)
    {
        qWarning() << "Indicator::rulePopupApplyClicked(): " << tr("Ongeldige bron indicator en/of bron curve");
        QMessageBox messageBox(this);
        messageBox.setText(tr("Ongeldige combinatie van bron indicator en/of bron curve"));
        messageBox.exec();
        return;
    }

    if (split.count() == 1)
        ruleString.append("SourceIndicator=NULL;SourceCurve=" + split.at(0) + ";");
    else
        ruleString.append("SourceIndicator=" + split.at(0) + ";SourceCurve=" + split.at(1) + ";");

    split = privateData->rulePopupForm.rcComboBox->currentText().split("->", QString::SkipEmptyParts);
    if (split.count() < 1 || split.count() > 2)
    {
        qWarning() << "Indicator::addRule(): " << tr("Ongeldige referentie indicator en/of referentie curve");
        QMessageBox messageBox(this);
        messageBox.setText(tr("Ongeldige combinatie van referentie indicator en/of referentie curve"));
        messageBox.exec();
        return;
    }

    if (split.count() == 1)
        ruleString.append("ReferenceIndicator=NULL;ReferenceCurve=" + (split.at(0).compare(tr("Waarde"), Qt::CaseInsensitive) == 0 ? "NULL" : split.at(0)) + ";");
    else
        ruleString.append("ReferenceIndicator=" + split.at(0) + ";ReferenceCurve=" + (split.at(1).compare(tr("Waarde"), Qt::CaseInsensitive) == 0 ? "NULL" : split.at(1)) + ";");

    ruleString.append("RefereneValue=" + QString::number(privateData->rulePopupForm.rcDoubleSpinBox->value()) + ";" +
                      "Offset=" + QString::number(privateData->rulePopupForm.ofSpinBox->value()) + ";" +
                      "CompareType=" + Rule::compareTypeTitle((Rule::CompareType) privateData->rulePopupForm.rulComboBox->currentIndex()) + ";" +
                      "LogicalOperator=" + Rule::logicalOperatorTitle((Rule::LogicalOperator) privateData->rulePopupForm.logComboBox->currentIndex()) + ";" +
                      "PreviousTransactionType=" + Rule::previousTransactionTypeTitle((TransactionType) privateData->rulePopupForm.ptComboBox->currentIndex()) + ";Enabled=1");

    QTreeWidgetItem *topLevelItem = currentItem;
    QTreeWidgetItem *previousItem = currentItem;
    while ((previousItem = previousItem->parent()))
        topLevelItem = previousItem;

    Rule *newRule = NULL;
    RuleList *ruleList = NULL;
    if (privateData->propertiesForm.ruleTreeWidget->topLevelItem(0) == topLevelItem)
    {
        newRule = privateData->longRuleList->addRule(ruleString, parentRule);
        ruleList = privateData->longRuleList;
    }
    else if (privateData->propertiesForm.ruleTreeWidget->topLevelItem(1) == topLevelItem)
    {
        newRule = privateData->shortRuleList->addRule(ruleString, parentRule);
        ruleList = privateData->shortRuleList;
    }
    else if (privateData->propertiesForm.ruleTreeWidget->topLevelItem(2) == topLevelItem)
    {
        newRule = privateData->exitRuleList->addRule(ruleString, parentRule);
        ruleList = privateData->exitRuleList;
    }

    if (!newRule)
    {
        qWarning() << "Indicator::rulePopupApplyClicked(): Failed to find the container of the current selected item";
        QMessageBox messageBox(this);
        messageBox.setText(tr("Kon de gewijzigde rule niet toevoegen aan de lijst: hoofdlijst niet gevonden"));
        messageBox.exec();
        return;
    }

    newRule->enabled = currentRule->enabled;
    newRule->children = currentRule->children;
    currentRule->children.clear();
    ruleList->deleteRule(currentRule);
    currentItem->rule = newRule;
    currentItem->setText(0, newRule->humanReadableTitle);

    privateData->propertiesForm.ruleTreeWidget->resizeColumnToContents(0);

    setProperties();
}

void Indicator::clearRuleLists(void)
{
    privateData->longRuleList->clear();
    privateData->shortRuleList->clear();
    privateData->exitRuleList->clear();

    return;
}


void Indicator::deleteRule(void)
{
    RuleTreeWidgetItem *currentItem = dynamic_cast<RuleTreeWidgetItem *>(privateData->propertiesForm.ruleTreeWidget->currentItem());
    if (!currentItem ||
        currentItem == privateData->propertiesForm.ruleTreeWidget->topLevelItem(0) ||
        currentItem == privateData->propertiesForm.ruleTreeWidget->topLevelItem(1) ||
        currentItem == privateData->propertiesForm.ruleTreeWidget->topLevelItem(2))
    {
        QMessageBox messageBox(this);
        messageBox.setText(tr("Selecteer eerst welke voorwaarde verwijdert moet worden."));
        messageBox.exec();
        return;
    }

    QTreeWidgetItem *topLevelItem = currentItem;
    QTreeWidgetItem *previousItem = currentItem;
    while ((previousItem = previousItem->parent()))
        topLevelItem = previousItem;

    RuleList *ruleList = NULL;
    if (privateData->propertiesForm.ruleTreeWidget->topLevelItem(0) == topLevelItem)
        ruleList = privateData->longRuleList;
    else if (privateData->propertiesForm.ruleTreeWidget->topLevelItem(1) == topLevelItem)
        ruleList = privateData->shortRuleList;
    else if (privateData->propertiesForm.ruleTreeWidget->topLevelItem(2) == topLevelItem)
        ruleList = privateData->exitRuleList;

    if (!ruleList)
    {
        qWarning() << "Indicator::deleteRule(): current item does not belong to a ruleList";
        QMessageBox messageBox(this);
        messageBox.setText(tr("Deze voorwaarde kan niet verwijdert worden."));
        messageBox.exec();
        return;
    }

    QTreeWidgetItem *parent = currentItem->parent();
    if (!parent)
    {
        qWarning() << "Indicator::deleteRule(): current item has no parent";
        QMessageBox messageBox(this);
        messageBox.setText(tr("Deze voorwaarde kan niet verwijdert worden omdat het geen onderdeel uitmaakt van de boom structuur."));
        messageBox.exec();
        return;
    }

    parent->removeChild(currentItem);
    ruleList->deleteRule(currentItem->rule);

    if (privateData->longRuleList->items().isEmpty() && privateData->shortRuleList->items().isEmpty() && privateData->exitRuleList->items().isEmpty())
    {
        privateData->propertiesForm.tradeRulesEnabledWidget->setVisible(false);
        privateData->propertiesForm.takeProfitsRulesEnabledWidget->setVisible(false);
        privateData->propertiesForm.stopLossRulesEnabledWidget->setVisible(false);
        privateData->propertiesForm.wLabel->setVisible(true);
        privateData->propertiesForm.tpwLabel->setVisible(true);
        privateData->propertiesForm.slwLabel->setVisible(true);
    }

    privateData->propertiesForm.ruleTreeWidget->resizeColumnToContents(0);

    setProperties();
}

void Indicator::renameRuleToNewTitle(Indicator *indicator, const QString &newTitle, QTreeWidgetItem *treeItem)
{
    if (!treeItem)
    {
        renameRuleToNewTitle(indicator, newTitle, privateData->propertiesForm.ruleTreeWidget->topLevelItem(0));
        renameRuleToNewTitle(indicator, newTitle, privateData->propertiesForm.ruleTreeWidget->topLevelItem(1));
        renameRuleToNewTitle(indicator, newTitle, privateData->propertiesForm.ruleTreeWidget->topLevelItem(2));
        return;
    }

    for (int i = 0; i < treeItem->childCount(); i++)
    {
        RuleTreeWidgetItem *ruleTreeItem = dynamic_cast<RuleTreeWidgetItem *>(treeItem->child(i));
        if (!ruleTreeItem)
            continue;

        Rule *rule = ruleTreeItem->rule;
        if (rule && (rule->sourceIndicator == indicator || rule->referenceIndicator == indicator))
        {
            rule->updateTitle();
            ruleTreeItem->setText(0, rule->humanReadableTitle);
        }

        renameRuleToNewTitle(indicator, newTitle, treeItem->child(i));
    }
}

void Indicator::clearTransactions(void)
{
    startCapital = priceChart->startCapital;
    finalCapital = startCapital;

    TradeResults *results = &privateData->tradeResults;
    results->startCapital = priceChart->startCapital;
    results->finalCapital = priceChart->startCapital;
    results->transactionCosts = 0.0;
    results->profits = 0.0;
    results->relativeProfits = 0.0;
    results->successRate = 0.0;
    results->totalTransactions = 0;
    results->totalTrades = 0;
    results->totalProfitTrades = 0;
    results->totalLossTrades = 0;
    results->ignore = false;

    if (processMode() != Indicator::View)
        return;

    QTableWidget *table = privateData->transactionTableForm.tableWidget;
    if (!table)
        return;

    table->setSortingEnabled(false);
    disconnect(table, SIGNAL(currentCellChanged(int, int, int, int)) , this, SLOT(zoomToSelectedItem(int, int, int, int)));

    while (table->rowCount() != 0)
        table->removeRow(0);

    privateData->transactionTableForm.scvLabel->setText("");
    privateData->transactionTableForm.fcvLabel->setText("");
    privateData->transactionTableForm.pvLabel->setText("");
    privateData->transactionTableForm.rpvLabel->setText("");
    privateData->transactionTableForm.ttvLabel->setText("");
    privateData->transactionTableForm.ftvLabel->setText("");
    privateData->transactionTableForm.ltvLabel->setText("");
    privateData->transactionTableForm.tcvLabel->setText("");

    if (markerPoints)
        delete markerPoints;

    markerPoints = new MarkerPoints(priceChart);
    connect(markerPoints, SIGNAL(hovered(QwtPlotSvgItem &)), this, SLOT(setToolTipForItem(QwtPlotSvgItem &)));
    connect(markerPoints, SIGNAL(hoverLeft(QwtPlotSvgItem &)), this, SLOT(unsetToolTipForItem(QwtPlotSvgItem &)));
    markerPoints->setColor(color());
}

void Indicator::finishTransations(TransactionType currentTransactionType, const float *askOpen, const float *bidOpen)
{
    if (currentTransactionType != Exit)
    {
        float price = (currentTransactionType == Long) ? bidOpen[indexPlot->periods - 1] : askOpen[indexPlot->periods - 1];
        addTransaction(price, fabsf(askOpen[indexPlot->periods - 1] - bidOpen[indexPlot->periods - 1]), indexPlot->periods - 1, Exit, currentTransactionType, false, false, false);
    }

    if (processMode() != Indicator::View)
        return;

    QTableWidget *table = privateData->transactionTableForm.tableWidget;
    if (!table)
        return;

    //    table->setSortingEnabled(true);
    table->resizeColumnsToContents();
    connect(table, SIGNAL(currentCellChanged(int, int, int, int)) , this, SLOT(zoomToSelectedItem(int, int, int, int)), Qt::UniqueConnection);

    markerPoints->rescaleMarkers(false);
}

#define min(x, y) y + ((x - y) & ((x - y) >> (sizeof(int) * CHAR_BIT - 1)))
#define max(x, y) x - ((x - y) & ((x - y) >> (sizeof(int) * CHAR_BIT - 1)))

int Indicator::nextSignal(int currentBar, TransactionType transactionType, int offset)
{
    int nextSignal = indexPlot->periods - 1;

    if (transactionType == Long)
    {
        int nextShortSignal = privateData->shortRuleList->currentCacheObject()->signalCache()[transactionType][currentBar] + offset;
        nextSignal = min(nextSignal, nextShortSignal);
        //        if (nextShortSignal < nextSignal)
        //            nextSignal = nextShortSignal;

        int nextExitSignal = privateData->exitRuleList->currentCacheObject()->signalCache()[transactionType][currentBar] + offset;
        nextSignal = min(nextSignal, nextExitSignal);
        //        if (nextExitSignal < nextSignal)
        //            nextSignal = nextExitSignal;

        if ((privateData->stopLossOptions.isRelative && !qFuzzyIsNull(privateData->stopLossOptions.relativeValue)) || (!privateData->stopLossOptions.isRelative && !qFuzzyIsNull(privateData->stopLossOptions.absoluteValue)))
        {
            int nextStopLossSignal = privateData->stopLossCacheEntry->signalCache()[transactionType][currentBar];
            nextSignal = min(nextSignal, nextStopLossSignal);
            //            if (nextStopLossSignal < nextSignal)
            //                nextSignal = nextStopLossSignal;
        }

        if ((privateData->takeProfitOptions.isRelative && !qFuzzyIsNull(privateData->takeProfitOptions.relativeValue)) || (!privateData->takeProfitOptions.isRelative && !qFuzzyIsNull(privateData->takeProfitOptions.absoluteValue)))
        {
            int nextTakeProfitsSignal = privateData->takeProfitsCacheEntry->signalCache()[transactionType][currentBar];
            nextSignal = min(nextSignal, nextTakeProfitsSignal);
            //            if (nextTakeProfitsSignal < nextSignal)
            //                nextSignal = nextTakeProfitsSignal;
        }
    }
    else if (transactionType == Short)
    {
        int nextLongSignal = privateData->longRuleList->currentCacheObject()->signalCache()[transactionType][currentBar] + offset;
        nextSignal = min(nextSignal, nextLongSignal);
        //        if (nextLongSignal < nextSignal)
        //            nextSignal = nextLongSignal;

        int nextExitSignal = privateData->exitRuleList->currentCacheObject()->signalCache()[transactionType][currentBar] + offset;
        nextSignal = min(nextSignal, nextExitSignal);
        //        if (nextExitSignal < nextSignal)
        //            nextSignal = nextExitSignal;

        if ((privateData->stopLossOptions.isRelative && !qFuzzyIsNull(privateData->stopLossOptions.relativeValue)) || (!privateData->stopLossOptions.isRelative && !qFuzzyIsNull(privateData->stopLossOptions.absoluteValue)))
        {
            int nextStopLossSignal = privateData->stopLossCacheEntry->signalCache()[transactionType][currentBar];
            nextSignal = min(nextSignal, nextStopLossSignal);
            //            if (nextStopLossSignal < nextSignal)
            //                nextSignal = nextStopLossSignal;
        }

        if ((privateData->takeProfitOptions.isRelative && !qFuzzyIsNull(privateData->takeProfitOptions.relativeValue)) || (!privateData->takeProfitOptions.isRelative && !qFuzzyIsNull(privateData->takeProfitOptions.absoluteValue)))
        {
            int nextTakeProfitsSignal = privateData->takeProfitsCacheEntry->signalCache()[transactionType][currentBar];
            nextSignal = min(nextSignal, nextTakeProfitsSignal);
            //            if (nextTakeProfitsSignal < nextSignal)
            //                nextSignal = nextTakeProfitsSignal;
        }
    }
    else if (transactionType == Exit)
    {
        int nextLongSignal = privateData->longRuleList->currentCacheObject()->signalCache()[transactionType][currentBar] + offset;
        nextSignal = min(nextSignal, nextLongSignal);
        //        if (nextLongSignal < nextSignal)
        //            nextSignal = nextLongSignal;

        int nextShortSignal = privateData->shortRuleList->currentCacheObject()->signalCache()[transactionType][currentBar] + offset;
        nextSignal = min(nextSignal, nextShortSignal);
        //        if (nextShortSignal < nextSignal)
        //            nextSignal = nextShortSignal;
    }

    nextSignal = max(nextSignal, currentBar);
    //    if (nextSignal > currentBar)
    //        return nextSignal;
    return nextSignal;
    //    return currentBar;
}

void Indicator::executeTransactionRules(void)
{
    if (indexPlot->periods < 1)
    {
        clearTransactions();
        return;
    }

    if (privateData->shortRuleList->items().isEmpty() && privateData->longRuleList->items().isEmpty())
    {
        clearTransactions();
        return;
    }

    if (!autoOptimizerMode())
    {
        firstRuns = 0;
        secondRuns = 0;
        privateData->longRuleList->clearRuleResolvementCache();
        privateData->longRuleList->createRuleResolvementCache();
        privateData->shortRuleList->clearRuleResolvementCache();
        privateData->shortRuleList->createRuleResolvementCache();
        privateData->exitRuleList->clearRuleResolvementCache();
        privateData->exitRuleList->createRuleResolvementCache();
        clearStopLossCache();
        createStopLossCache();
        clearTakeProfitsCache();
        createTakeProfitsCache();
    }

    bool replottingDisabled = indexPlot->plottingDisabled();
    if (processMode() == Indicator::View)
    {
        indexPlot->setPlottingDisabled(true);
        privateData->transactionTableWidget->setUpdatesEnabled(false);
    }

    privateData->simulationPropertiesData = simulationProperties->properties();

    const SessionInfo &sessionInfo = autoOptimizer().sessionInfo();
    TradeResults results1;
    TradeResults results2;
    bool secondRun = true;
retry:

    clearTransactions();

    const float *askOpen = priceChart->askPriceOpen->fastData();
    const float *bidOpen = priceChart->bidPriceOpen->fastData();
    const float *askHigh = priceChart->askPriceHigh->fastData();
    const float *bidHigh = priceChart->bidPriceHigh->fastData();
    const float *askLow = priceChart->askPriceLow->fastData();
    const float *bidLow = priceChart->bidPriceLow->fastData();
    const float *avgOpen = priceChart->averagePriceOpen->fastData();

    int offset = 1;; // tenzij indicator source ingesteld staat op open.
    int transactionStart = offset;
    float winningPrice = 0.0;
    float losingPrice = 0.0;
    int  evaluatedJump = -1;
    int losingStrategyIterator = 0;
    TransactionType currentTransactionType = Exit;

    for (int currentBar = offset; currentBar < indexPlot->periods; currentBar++)
    {
        if (secondRun)
        {
            if (evaluatedJump != transactionStart)
            {
                evaluatedJump = transactionStart;

                int nextBar = nextSignal(transactionStart, currentTransactionType, offset);
                if (nextBar > currentBar)
                {
                    //                    qDebug() << "at bar " << currentBar << " jump to " << nextBar;
                    currentBar = nextBar;
                }
            }

            //            qDebug() << "currentBar=" << currentBar;
        }

        if (secondRun)
            secondRuns++;
        else
            firstRuns++;

        if (currentTransactionType == Long || currentTransactionType == Short)
        {
            const RuleListResult result = privateData->exitRuleList->resolveRuleList(currentBar - offset, currentTransactionType, transactionStart);
            if (result.result)
            {
                privateData->longRuleList->updateSignalCacheForTransaction(currentBar - offset, currentTransactionType, transactionStart);
                privateData->shortRuleList->updateSignalCacheForTransaction(currentBar - offset, currentTransactionType, transactionStart);

                if (currentBar > privateData->stopLossCacheEntry->signalCache()[currentTransactionType][transactionStart])
                    privateData->stopLossCacheEntry->signalCache()[currentTransactionType][transactionStart] = currentBar;

                if (currentBar > privateData->takeProfitsCacheEntry->signalCache()[currentTransactionType][transactionStart])
                    privateData->takeProfitsCacheEntry->signalCache()[currentTransactionType][transactionStart] = currentBar;

                float price = (currentTransactionType == Long) ? bidOpen[currentBar] : askOpen[currentBar];
                addTransaction(price, fabsf(askOpen[currentBar] - bidOpen[currentBar]), currentBar, Exit, currentTransactionType, false, false, true);
                currentTransactionType = Exit;
                transactionStart = currentBar;
            }
        }

        if (currentTransactionType == Exit || currentTransactionType == Short)
        {
            const RuleListResult result = privateData->longRuleList->resolveRuleList(currentBar - offset, currentTransactionType, transactionStart);
            if (result.result)
            {
                privateData->shortRuleList->updateSignalCacheForTransaction(currentBar - offset, currentTransactionType, transactionStart);

                if (currentTransactionType == Short)
                {
                    addTransaction(askOpen[currentBar], fabsf(askOpen[currentBar] - bidOpen[currentBar]), currentBar, Exit, currentTransactionType, false, false, true);
                    currentTransactionType = Exit;
                }

                addTransaction(askOpen[currentBar], fabsf(askOpen[currentBar] - bidOpen[currentBar]), currentBar, Long, currentTransactionType, false, false, true);
                currentTransactionType = Long;
                transactionStart = currentBar;
                winningPrice = setWinningTradePrice(askOpen[currentBar], Long);
                losingPrice = setLosingTradePrice(askOpen[currentBar], Long, currentBar, transactionStart);
            }
        }

        if (currentTransactionType == Exit || currentTransactionType == Long)
        {
            const RuleListResult result = privateData->shortRuleList->resolveRuleList(currentBar - offset, currentTransactionType, transactionStart);
            if (result.result)
            {
                privateData->longRuleList->updateSignalCacheForTransaction(currentBar - offset, currentTransactionType, transactionStart);

                if (currentTransactionType == Long)
                {
                    addTransaction(bidOpen[currentBar], fabsf(askOpen[currentBar] - bidOpen[currentBar]), currentBar, Exit, currentTransactionType, false, false, true);
                    currentTransactionType = Exit;
                }

                addTransaction(bidOpen[currentBar], fabsf(askOpen[currentBar] - bidOpen[currentBar]), currentBar, Short, currentTransactionType, false, false, true);
                currentTransactionType = Short;
                transactionStart = currentBar;
                winningPrice = setWinningTradePrice(bidOpen[currentBar], Short);
                losingPrice = setLosingTradePrice(bidOpen[currentBar], Short, currentBar, transactionStart);
            }
        }

        if (currentTransactionType == Long || currentTransactionType == Short)
        {
            if (isLosingTrade(losingPrice, askHigh[currentBar], bidLow[currentBar], currentTransactionType, currentBar, transactionStart))
            {
                privateData->exitRuleList->updateSignalCacheForTransaction(currentBar - offset, currentTransactionType, transactionStart);

                if (currentBar > privateData->takeProfitsCacheEntry->signalCache()[currentTransactionType][transactionStart])
                    privateData->takeProfitsCacheEntry->signalCache()[currentTransactionType][transactionStart] = currentBar;

                addTransaction(losingTradePrice(losingPrice, askOpen[currentBar], bidOpen[currentBar], currentTransactionType), fabsf(askOpen[currentBar] - bidOpen[currentBar]), currentBar, Exit, currentTransactionType, true, false, true);
                currentTransactionType = Exit;
                transactionStart = currentBar;
                continue;
            }

            if (isWinningTrade(winningPrice, bidHigh[currentBar], askLow[currentBar], currentTransactionType, currentBar, transactionStart))
            {
                privateData->exitRuleList->updateSignalCacheForTransaction(currentBar - offset, currentTransactionType, transactionStart);

                if (currentBar > privateData->stopLossCacheEntry->signalCache()[currentTransactionType][transactionStart])
                    privateData->stopLossCacheEntry->signalCache()[currentTransactionType][transactionStart] = currentBar;

                addTransaction(winningTradePrice(winningPrice, bidOpen[currentBar], askOpen[currentBar], currentTransactionType), fabsf(askOpen[currentBar] - bidOpen[currentBar]), currentBar, Exit, currentTransactionType, false, true, true);
                currentTransactionType = Exit;
                transactionStart = currentBar;
            }
        }

#if USE_DEAD_STRATEGY_SKIPPING
        if (losingStrategyIterator >= 14)
        {
            losingStrategyIterator = 0;
            bool losingStrategy = autoOptimizerMode() ? isLosingStrategy(currentBar, avgOpen[currentBar], currentTransactionType, sessionInfo) : false;
            if (losingStrategy)
            {
                privateData->tradeResults.ignore = true;
                break;
            }
        }
        else
            losingStrategyIterator++;
#endif
    }

    finishTransations(currentTransactionType, askOpen, bidOpen);

    if (!secondRun)
    {
        secondRun = true;
        results1 = tradeResults();

        goto retry;
    }
    else
    {
        results2 = tradeResults();
        if (results1.relativeProfits > 0.0 && results1.relativeProfits != results2.relativeProfits)
        {
            qDebug() << "ERROR: trade results are not the same " << results1.relativeProfits << " vs " << results2.relativeProfits;
            exit(0);
        }
    }

    if (processMode() == Indicator::View)
    {
        privateData->transactionTableWidget->setUpdatesEnabled(true);
        indexPlot->setPlottingDisabled(replottingDisabled);
        indexPlot->scheduleReplot();
    }
}

void Indicator::calculateMACurve(const PlotCurve &source, PlotCurve &dest, int periods, int type, int subType, int slowSmoothing, int fastSmoothing)
{
    QVector<QPointF> buffer;

    calculateMACurve(source.data(), buffer, periods, type, subType, slowSmoothing, fastSmoothing);
    dest.setData(buffer);
}

int Indicator::autoPeriodsForX(double x, const PeriodOptions &options)
{
    if (!options.cycleAdativeEnabled)
        return options.periods;
    else
    {
        if (options.trendAdaptiveEnabled || !priceChart->trendAvailable(x))
        {
            int adaptivePeriods = priceChart->dominantCyclePeriod(x);
            if (adaptivePeriods < 0)
                return options.periods;

            adaptivePeriods = adaptivePeriods * options.amplificationFactor;
            if (options.useHalfCycle)
                adaptivePeriods = adaptivePeriods / 2;

            if (adaptivePeriods < options.minCyclePeriods)
                adaptivePeriods = options.minCyclePeriods;

            if (adaptivePeriods > options.maxCyclePeriods)
                adaptivePeriods = options.maxCyclePeriods;

            if (adaptivePeriods < 2)
                adaptivePeriods = 2;

            return adaptivePeriods;
        }
    }

    return options.periods;
}

void Indicator::calculateMACurve(const QVector< QPointF > &source, QVector< QPointF > &dest, int periods, int type, int subType, int slowSmoothing, int fastSmoothing)
{
    (void)subType;

    dest.clear();

    if (type == Simple)
        calculateSMACurve(source, dest, source.size(), periods);
    else if (type == Exponential)
        calculateEMACurve(source, dest, source.size(), periods);
    else if (type == LinearWeighted)
        calculateLWMACurve(source, dest, source.size(), periods);
    else if (type == Adaptive)
        calculateAMACurve(source, dest, source.size(), periods, slowSmoothing, fastSmoothing);
    else if (type == Hull)
        calculateHMACurve(source, dest, source.size(), periods);
    //     else if ( type == Jurik )
    //         calculateJMACurve( source, dest, periods, subType );
    else if (type == GDEMA)
        calculateGDEMACurve(source, dest, periods);
    else if (type == T3)
        calculateT3Curve(source, dest, periods);
    else if (type == ZLEMA)
        calculateZLEMACurve(source, dest, periods);
    else if (type == MAMA)
        calculateMAMACurve(source, dest, periods, slowSmoothing, fastSmoothing);
    else if (type == FAMA)
        calculateFAMACurve(source, dest, periods, slowSmoothing, fastSmoothing);
    else
        calculateSMACurve(source, dest, source.size(), periods);
}

void Indicator::calculateMACurve(const PlotCurve &source, PlotCurve &dest, const MovingAverageOptions &options)
{
    QVector<QPointF> buffer;

    calculateMACurve(source.data(), buffer, options);
    dest.setData(buffer);
}

void Indicator::calculateMACurve(const QVector< QPointF > &source, QVector< QPointF > &dest, const MovingAverageOptions &options)
{
    dest.clear();

    if (source.isEmpty())
        return;

    QVector <QPointF> cache = priceChart->indicatorCalculateCache()->findItem(source, options.createConfigKey());
    if (!cache.isEmpty())
    {
        dest = cache;
        return;
    }

    if (options.type == Simple)
        calculateSMACurve(source, dest, options);
    else if (options.type == Exponential)
        calculateEMACurve(source, dest, options);
    else if (options.type == LinearWeighted)
        calculateLWMACurve(source, dest, options);
    else if (options.type == Adaptive)
        calculateAMACurve(source, dest, options);
    else
        calculateSMACurve(source, dest, options);

    priceChart->indicatorCalculateCache()->addItem(source, dest, options.createConfigKey());
}


void Indicator::calculateSMACurve(const PlotCurve &source, PlotCurve &dest, int size, int periods)
{
    QVector<QPointF> buffer;

    calculateSMACurve(source.data(), buffer, size, periods);
    dest.setData(buffer);
}

void Indicator::calculateSMACurve(const QVector<QPointF> &source, QVector<QPointF> &dest, int size, int periods)
{
    double tmp = 0;
    QPointF point;

    if (source.isEmpty())
        return;

    for (int i = 0; i < size; ++i)
    {
        tmp += source[ i ].y();
        if (i < (periods - 1))
            continue;

        point.setX(source[ i ].x());
        point.setY(tmp / periods);
        dest.append(point);
        tmp -= source[ i - (periods - 1) ].y();
    }
}

void Indicator::calculateSMACurve(const PlotCurve &source, PlotCurve &dest, const MovingAverageOptions &options)
{
    QVector<QPointF> buffer;

    calculateSMACurve(source.data(), buffer, options);
    dest.setData(buffer);
}

void Indicator::calculateSMACurve(const QVector< QPointF > &source, QVector< QPointF > &dest, const MovingAverageOptions &options)
{
    QVector<QPointF>::const_iterator it = source.constBegin();
    const QVector<QPointF>::const_iterator end = source.constEnd();
    for (int i = 0; it != end; it++, i++)
    {
        if (i < (options.periods - 1))
            continue;

        const QPointF &currentPoint = *it;
        double x = currentPoint.x();
        int periods = autoPeriodsForX(x, options);
        if (i < (periods - 1))
            periods = i + 1;

        double tmp = 0;
        for (int j = 0; j < periods; ++j)
            tmp += it[ -j ].y();

        QPointF point(x, tmp / periods);
        dest.append(point);
    }
}

void Indicator::calculateEMACurve(const PlotCurve &source, PlotCurve &dest, int size, int periods)
{
    QVector<QPointF> buffer;

    calculateEMACurve(source.data(), buffer, size, periods);
    dest.setData(buffer);
}

void Indicator::calculateEMACurve(const QVector<QPointF> &source, QVector<QPointF> &dest, int size, int periods)
{
    double ema = 2 / (double)(periods + 1);
    QPointF point;

    if (source.isEmpty())
        return;

    point.setX(source[ 0 ].x());
    point.setY(source[ 0 ].y());
    dest.append(point);

    for (int i = 1; i < size; ++i)
    {
        point.setX(source[ i ].x());
        point.setY((source[ i ].y() * ema) + (dest[ i - 1 ].y() * (1 - ema)));
        dest.append(point);
    }
}

void Indicator::calculateEMACurve(const PlotCurve &source, PlotCurve &dest, const MovingAverageOptions &options)
{
    QVector<QPointF> buffer;

    calculateEMACurve(source.data(), buffer, options);
    dest.setData(buffer);
}

void Indicator::calculateEMACurve(const QVector< QPointF > &source, QVector< QPointF > &dest, const MovingAverageOptions &options)
{
    double ema;
    int periods;
    QPointF point;

    if (source.isEmpty())
        return;

    point.setX(source[ 0 ].x());
    point.setY(source[ 0 ].y());
    dest.append(point);

    size_t sourceSize = source.size();
    for (size_t i = 1; i < sourceSize; ++i)
    {
        periods = autoPeriodsForX(source[ i ].x(), options);
        ema = 2 / (double)(periods + 1);
        point.setX(source[ i ].x());
        point.setY((source[ i ].y() * ema) + (dest[ i - 1 ].y() * (1 - ema)));
        dest.append(point);
    }
}

void Indicator::calculateLWMACurve(const PlotCurve &source, PlotCurve &dest, int size, int periods)
{
    QVector<QPointF> buffer;

    calculateLWMACurve(source.data(), buffer, size, periods);
    dest.setData(buffer);
}

void Indicator::calculateLWMACurve(const QVector<QPointF> &source, QVector<QPointF> &dest, int size, int periods)
{
    double periodsSum = (periods * (periods + 1)) / 2;
    QPointF point;

    for (int i = 0; i < size; ++i)
    {
        if (i < (periods - 1))
            continue;

        double tmp = 0;
        for (int j = periods - 1; j >= 0; --j)
        {
            tmp += (source[ i - j ].y() / periodsSum) * (periods - j);
        }
        point.setX(source[ i ].x());
        point.setY(tmp);
        dest.append(point);
    }
}

void Indicator::calculateLWMACurve(const PlotCurve &source, PlotCurve &dest, const MovingAverageOptions &options)
{
    QVector<QPointF> buffer;

    calculateLWMACurve(source.data(), buffer, options);
    dest.setData(buffer);
}

void Indicator::calculateLWMACurve(const QVector<QPointF> &source, QVector<QPointF> &dest, const MovingAverageOptions &options)
{
    double tmp;
    double periodsSum;
    int periods;
    QPointF point;

    int sourceSize = source.size();
    for (int i = 0; i < sourceSize; ++i)
    {
        if (i < (options.periods - 1))
            continue;

        periods = autoPeriodsForX(source[ i ].x(), options);
        if (i < (periods - 1))
            periods = i + 1;

        periodsSum = (periods * (periods + 1)) / 2;
        tmp = 0;
        for (int j = periods - 1; j >= 0; --j)
        {
            tmp += (source[ i - j ].y() / periodsSum) * (periods - j);
        }
        point.setX(source[ i ].x());
        point.setY(tmp);
        dest.append(point);
    }
}

void Indicator::calculateAMACurve(const PlotCurve &source, PlotCurve &dest, int size, int periods, int slowSmoothing, int fastSmoothing)
{
    QVector<QPointF> buffer;

    calculateAMACurve(source.data(), buffer, size, periods, slowSmoothing, fastSmoothing);
    dest.setData(buffer);
}

void Indicator::calculateAMACurve(const QVector<QPointF> &source, QVector<QPointF> &dest, int size, int periods, int slowSmoothing, int fastSmoothing)
{
    double direction;
    double volatility;
    double sum;
    double c;
    double ssc;
    double slowSC;
    double fastSC;
    double er;
    double rsi;
    double factor;
    //    double adaptivePeriods;
    //    int address;
    QPointF point;

    //     if ( fastSmoothing == -1 )
    //         fastSmoothing = periods / 10;
    //
    //     if ( fastSmoothing < 1 )
    //         fastSmoothing = 1;
    //
    //     if ( slowSmoothing == -1 )
    //         slowSmoothing = periods * 1.5;
    //
    //     if ( slowSmoothing < fastSmoothing )
    //         slowSmoothing = fastSmoothing;

    //     if ( periods < 2 )
    //         periods = 2;

    slowSC = 2.0 / (slowSmoothing + 1.0);
    fastSC = 2.0 / (fastSmoothing + 1.0);

    if (periods < 2)
        periods = 2;

    QVector<QPointF> rsiData;
    calculateRSICurve(source, rsiData, periods, Exponential);
    for (int i = 0; i < size; ++i)
    {
        if (i < (periods - 1))
        {
            point.setX(source[ i ].x());
            point.setY(source[ i ].y());
            dest.append(point);
            continue;
        }

        //         adaptivePeriods = strategiePage->dominantCyclePeriod( source[ i ].x() );
        //         if ( adaptivePeriods > 5 /*&& ( i - ( ( int ) adaptivePeriods - 1 ) >= 0 )*/ )
        //             periods = adaptivePeriods/2;//2 / ( double ) ( ( adaptivePeriods/2.0 ) + 1 );
        //         if ( periods < 10 )
        //             periods = 10;
        direction = source[ i ].y() - source[ i - (periods - 1) ].y();
        volatility = 0;
        for (int j = 0; j < (periods - 1); ++j)
        {
            sum = source[ i - j ].y() - source[ i - (1 + j) ].y();
            if (sum < 0)
                sum = sum * -1;

            volatility = volatility + sum;
        }

        er = direction / volatility;
        if (er < 0)
            er = er * - 1;

        if (er != er)
            er = 0;

        rsi = rsiData.at(i - 1).y() - 50;
        if (rsi < 0)
            rsi = rsi * -1;

        rsi = rsi / 50;

        //        if ( rsi < 0.4 ) {
        //            factor = er;
        //            factor = ( er - (rsi + rsi ) ) / 2;
        //        } else
        factor = (er + rsi + rsi) / 2;
        //        factor = 1 - er;
        if (((er / 18) * periods) < 0.20)
            //        if ( er < 0.20 )
            factor = 0;
        //        factor = 1 - er;
        //        factor = er;
        ssc = (factor * (fastSC - slowSC)) + slowSC;
        c = ssc * ssc;

        point.setX(source[ i ].x());
        point.setY(c * (source[ i ].y() - dest[ i - 1 ].y()) + dest[ i - 1 ].y());

        dest.append(point);
    }

    for (int i = 0; i < source.size() && i < (periods - 1); ++i)
        dest.remove(0);

    //     static int count = 0;
    //     if ( count > 0 )
    //         return;
    //
    //     count++;
    //      QVector<QPointF> ema;
    //      calculateMACurve( dest, ema, periods/2, Adaptive, -1, 300, 2 );
    //      dest.clear();
    //      dest = ema;
}

void Indicator::calculateAMACurve(const PlotCurve &source, PlotCurve &dest, const MovingAverageOptions &options)
{
    QVector<QPointF> buffer;

    calculateAMACurve(source.data(), buffer, options);
    dest.setData(buffer);
}

void Indicator::calculateAMACurve(const QVector< QPointF > &source, QVector< QPointF > &dest, const MovingAverageOptions &options)
{
    double direction;
    double volatility;
    double sum;
    double c;
    double ssc;
    double slowSC;
    double fastSC;
    double er;
    double rsi;
    double factor;
    //    double adaptivePeriods;
    //    int address;
    int periods;
    QPointF point;

    slowSC = 2.0 / (options.slowSmoothing + 1.0);
    fastSC = 2.0 / (options.fastSmoothing + 1.0);

    QVector<QPointF> rsiData;
    calculateRSICurve(source, rsiData, options.periods, Exponential);

    int sourceSize = source.size();
    for (int i = 0; i < sourceSize; ++i)
    {
        periods = autoPeriodsForX(source[ i ].x(), options);
        if (periods < 2)
            periods = 2;

        if (i < (periods - 1))
        {
            point.setX(source[ i ].x());
            point.setY(source[ i ].y());
            dest.append(point);
            continue;
        }


        direction = source[ i ].y() - source[ i - (periods - 1) ].y();
        volatility = 0;
        for (int j = 0; j < (periods - 1); ++j)
        {
            sum = source[ i - j ].y() - source[ i - (1 + j) ].y();
            if (sum < 0)
                sum = sum * -1;

            volatility = volatility + sum;
        }

        er = direction / volatility;
        if (er < 0)
            er = er * - 1;

        if (er != er)
            er = 0;

        rsi = rsiData.at(i - 1).y() - 50;
        if (rsi < 0)
            rsi = rsi * -1;

        rsi = rsi / 50;

        factor = (er + rsi + rsi) / 2;
        if (((er / 18) * periods) < 0.20)
            factor = 0;

        ssc = (factor * (fastSC - slowSC)) + slowSC;
        c = ssc * ssc;

        point.setX(source[ i ].x());
        point.setY(c * (source[ i ].y() - dest[ i - 1 ].y()) + dest[ i - 1 ].y());

        dest.append(point);
    }

    for (int i = 0; i < source.size() && i < (options.periods - 1); ++i)
        dest.remove(0);
}

void Indicator::calculateHMACurve(const PlotCurve &source, PlotCurve &dest, int size, int periods)
{
    QVector<QPointF> buffer;

    calculateHMACurve(source.data(), buffer, size, periods);
    dest.setData(buffer);
}

void Indicator::calculateHMACurve(const QVector< QPointF > &source, QVector< QPointF > &dest, int size, int periods)
{
    (void)size;

    int halfPeriods = periods / 2;
    int offset = periods - halfPeriods;
    QVector<QPointF> halfMA;
    QVector<QPointF> fullMA;

    if (periods == 1)
    {
        halfPeriods = 1;
        offset = 0;
    }

    calculateMACurve(source, halfMA, halfPeriods, LinearWeighted);
    calculateMACurve(source, fullMA, periods, LinearWeighted);

    QVector<QPointF> waData;
    QPointF point;
    for (int i = 0; i < fullMA.size(); ++i)
    {
        point.setX(fullMA.at(i).x());
        point.setY((2 * halfMA.at(i + offset).y()) - fullMA.at(i).y());
        waData.append(point);
    }

    calculateMACurve(waData, dest, sqrt(periods), LinearWeighted);
}

void Indicator::calculateJMACurve(const PlotCurve &source, PlotCurve &dest, int periods, int subType)
{
    QVector<QPointF> buffer;

    calculateJMACurve(source.data(), buffer, periods, subType);
    dest.setData(buffer);
}

void Indicator::calculateJMACurve(const QVector< QPointF > &source, QVector< QPointF > &dest, int periods, int subType)
{
    if (subType == -1)
        subType = Simple;

    QVector<QPointF> ma;
    calculateMACurve(source, ma, periods, subType);

    QPointF point;
    for (int i = 0; i < ma.size(); ++i)
    {
        double s1 = 0;
        for (int j = 0; j < periods; j++)
        {
            s1 = s1 + ((periods - (2 * j) - 1) / 2) * source.at((i + (periods - 1)) - j).y();
        }

        point.setX(ma.at(i).x());
        point.setY(ma.at(i).y() + (((periods / 2) + 1) * s1) / ((periods + 1) * periods));
        dest.append(point);
    }
}

double vFactor = 0.7;

void Indicator::calculateGDEMACurve(const PlotCurve &source, PlotCurve &dest, int periods)
{
    QVector<QPointF> buffer;

    calculateGDEMACurve(source.data(), buffer, periods);
    dest.setData(buffer);
}
// X1= XAverage(Price, Period) * (1 + vFactor);
// X2= XAverage(XAverage(Price, Period), Period) * vFactor;
// GD = X1 - X2;
void Indicator::calculateGDEMACurve(const QVector< QPointF > &source, QVector< QPointF > &dest, int periods)
{
    QVector<QPointF> EMA1;
    QVector<QPointF> EMA2;
    QVector<QPointF> EMA3;
    QPointF point;
    double X1;
    double X2;

    calculateMACurve(source, EMA1, periods, Exponential);
    calculateMACurve(source, EMA2, periods, Exponential);
    calculateMACurve(EMA2, EMA3, periods, Exponential);

    for (int i = 0; i < EMA1.size(); ++i)
    {
        X1 = EMA1.at(i).y() * (1 + vFactor);
        X2 = EMA3.at(i).y() * vFactor;

        point.setX(EMA1.at(i).x());
        point.setY(X1 - X2);
        dest.append(point);
    }
}

//T3 = GD (GD ( GD(Price, Period, vFactor), Period, vFactor), Period, vFactor);

void Indicator::calculateT3Curve(const PlotCurve &source, PlotCurve &dest, int periods)
{
    QVector<QPointF> buffer;

    calculateT3Curve(source.data(), buffer, periods);
    dest.setData(buffer);

}

void Indicator::calculateT3Curve(const QVector< QPointF > &source, QVector< QPointF > &dest, int periods)
{

    QVector<QPointF> DEMA1;
    QVector<QPointF> DEMA2;

    calculateMACurve(source, DEMA1, periods, GDEMA);
    calculateMACurve(DEMA1, DEMA2, periods, GDEMA);
    calculateMACurve(DEMA2, dest, periods, GDEMA);
}


void Indicator::calculateZLEMACurve(const PlotCurve &source, PlotCurve &dest, int periods)
{
    QVector<QPointF> buffer;

    calculateZLEMACurve(source.data(), buffer, periods);
    dest.setData(buffer);

}

void Indicator::calculateZLEMACurve(const QVector< QPointF > &source, QVector< QPointF > &dest, int periods)
{
    double factor = 2 / (double)(periods + 1.0);
    int lag = (periods - 1) / 2;
    QPointF point;

    //EMA = K * ( 2 * price - price[x]) + (1 - K) * EMA[1];
    for (int i = 0; i < source.size(); ++i)
    {
        if (i < lag || i < 1)
        {
            point.setX(source[ 0 ].x());
            point.setY(source[ 0 ].y());
            dest.append(point);
            continue;
        }

        point.setX(source[ i ].x());
        point.setY(factor * (2 * source.at(i).y() - source.at(i - lag).y()) + (1 - factor) * dest.at(i - 1).y());
        dest.append(point);
    }
}

void Indicator::calculateMAMACurve(const PlotCurve &source, PlotCurve &dest, int periods, int slowSmoothing, int fastSmoothing)
{
    QVector<QPointF> buffer;

    calculateMAMACurve(source.data(), buffer, periods, slowSmoothing, fastSmoothing);
    dest.setData(buffer);
}

void Indicator::calculateMAMACurve(const QVector< QPointF > &source, QVector< QPointF > &dest, int periods, int slowSmoothing, int fastSmoothing)
{
    (void)periods;

    QVector<QPointF> smooth;
    QVector<QPointF> detrender;
    QVector<QPointF> II2;
    QVector<QPointF> Q2;
    QVector<QPointF> re;
    QVector<QPointF> im;
    QVector<QPointF> I1;
    QVector<QPointF> Q1;
    QVector<QPointF> jl;
    QVector<QPointF> jq;
    QVector<QPointF> phase;
    QVector<QPointF> deltaPhase;
    QVector<QPointF> period;
    QPointF point;
    double PI = 3.1415926;
    double factor;
    double alpha;
    double fastLimit;
    double slowLimit;

    //     QVector<QPointF> tmp;
    //     calculateMedianCurve( strategiePage->indexCurveLow->data(), strategiePage->indexCurveHigh->data(), tmp );
    //     source = tmp;

    fastLimit = 1.0 / (double) fastSmoothing;
    slowLimit = 1.0 / (double) slowSmoothing;

    for (int i = 0; i < source.size() && i < 3; ++i)
    {
        point.setX(source.at(i).x());
        point.setY(0.0);
        smooth.append(point);
    }

    for (int i = 3; i < source.size(); ++i)
    {
        point.setX(source[ i ].x());
        point.setY((4 * source[ i ].y() + 3 * source[ i - 1 ].y() + 2 * source[ i - 2 ].y() + source[ i - 3 ].y()) / 10);
        smooth.append(point);
    }

    dest = detrender = II2 = Q2 = re = im = I1 = Q1 = jl = jq = phase = deltaPhase = period = smooth;

    for (int i = 0; i < source.size(); ++i)
    {
        if (i < 6)
        {
            deltaPhase[ i ].setY(0);
            II2[ i ].setY(0);
            Q2[ i ].setY(0);
            re[ i ].setY(0);
            im[ i ].setY(0);
            period[ i ].setY(6);
        }
        else
        {
            factor = 0.075 * period[ i - 1 ].y() + 0.54;
            detrender[ i ].setY(((0.0962 * smooth[ i ].y() + 0.5769 * smooth[ i - 2 ].y() - 0.5769 * smooth[ i - 4].y() - 0.0962 * smooth[ i - 6 ].y()) * factor));

            Q1[ i ].setY(((0.0962 * detrender[ i ].y() + 0.5769 * detrender[ i - 2 ].y() - 0.5769 * detrender[ i - 4 ].y() - 0.0962 * detrender[ i - 6 ].y()) * factor));
            I1[ i ] = detrender[ i - 3 ];

            jl[ i ].setY(((0.0962 * I1[ i ].y() + 0.5769 * I1[ i - 2 ].y() - 0.5769 * I1[ i - 4 ].y() - 0.0962 * I1[ i - 6 ].y()) * factor));

            jq[ i ].setY(((0.0962 * Q1[ i ].y() + 0.5769 * Q1[ i - 2 ].y() - 0.5769 * Q1[ i - 4 ].y() - 0.0962 * Q1[ i - 6 ].y()) * factor));

            II2[ i ].setY(I1[ i ].y() - jq[ i ].y());

            Q2[ i ].setY(Q1[ i ].y() + jl[ i ].y());

            II2[ i ].setY(0.2 * II2[ i ].y() + 0.8 * II2[ i - 1 ].y());

            Q2[ i ].setY(0.2 * Q2[ i ].y() + 0.8 * Q2[ i - 1 ].y());

            re[ i ].setY(II2[ i ].y() * II2[ i - 1 ].y() + Q2[ i ].y() * Q2[ i - 1 ].y());

            im[ i ].setY(II2[ i ].y() * Q2[ i - 1 ].y() - Q2[ i ].y() * II2[ i - 1 ].y());

            re[ i ].setY(0.2 * re[ i ].y() + 0.8 * re[ i - 1 ].y());

            im[ i ].setY(0.2 * im[ i ].y() + 0.8 * im[ i - 1 ].y());

            period[ i ] = period[ i - 1 ];

            if (!qFuzzyCompare(im[ i ].y(), 0) && !qFuzzyCompare(re[ i ].y(), 0))
                period[ i ].setY((2 * PI) / atan(im[ i ].y() / re[ i ].y()));

            if (period[ i ].y() > 1.5 * period[ i - 1 ].y())
                period[ i ].setY(1.5 * period[ i - 1 ].y());

            if (period[ i ].y() < 0.67 * period[ i - 1 ].y())
                period[ i ].setY(0.67 * period[ i - 1 ].y());

            if (period[ i ].y() < 6)
                period[ i ].setY(6);

            if (period[ i ].y() > 50)
                period[ i ].setY(50);

            period[ i ].setY(0.2 * period[ i ].y() + 0.8 * period[ i - 1 ].y());

            if (!qFuzzyCompare(I1[ i ].y(), 0))
                phase[ i ].setY((180 / PI) * atan(Q1[ i ].y() / I1[ i ].y()));

            deltaPhase[ i ].setY(phase[ i - 1 ].y() - phase[ i ].y());

            if (deltaPhase[ i ].y() < 1)
                deltaPhase[ i ].setY(1);

            alpha = fastLimit / deltaPhase[ i ].y();

            if (alpha < slowLimit)
                alpha = slowLimit;

            dest[ i ].setY(alpha * source[ i ].y() + (1 - alpha) * dest[ i - 1].y());
        }
    }

    for (int i = 0; i < 6; ++i)
        dest.remove(0);
}

void Indicator::calculateFAMACurve(const PlotCurve &source, PlotCurve &dest, int periods, int slowSmoothing, int fastSmoothing)
{
    QVector<QPointF> buffer;

    calculateFAMACurve(source.data(), buffer, periods, slowSmoothing, fastSmoothing);
    dest.setData(buffer);
}

void Indicator::calculateFAMACurve(const QVector< QPointF > &source, QVector< QPointF > &dest, int periods, int slowSmoothing, int fastSmoothing)
{
    (void)periods;

    QVector<QPointF> smooth;
    QVector<QPointF> detrender;
    QVector<QPointF> II2;
    QVector<QPointF> Q2;
    QVector<QPointF> re;
    QVector<QPointF> im;
    QVector<QPointF> I1;
    QVector<QPointF> Q1;
    QVector<QPointF> jl;
    QVector<QPointF> jq;
    QVector<QPointF> phase;
    QVector<QPointF> deltaPhase;
    QVector<QPointF> period;
    QPointF point;
    double PI = 3.1415926;
    double factor;
    double alpha;
    double fastLimit;
    double slowLimit;

    //     QVector<QPointF> tmp;
    //     calculateMedianCurve( strategiePage->indexCurveLow->data(), strategiePage->indexCurveHigh->data(), tmp );
    //     source = tmp;

    fastLimit = 1.0 / (double) fastSmoothing;
    slowLimit = 1.0 / (double) slowSmoothing;

    for (int i = 0; i < source.size() && i < 3; ++i)
    {
        point.setX(source.at(i).x());
        point.setY(0.0);
        smooth.append(point);
    }

    for (int i = 3; i < source.size(); ++i)
    {
        point.setX(source[ i ].x());
        point.setY((4 * source[ i ].y() + 3 * source[ i - 1 ].y() + 2 * source[ i - 2 ].y() + source[ i - 3 ].y()) / 10);
        smooth.append(point);
    }

    dest = detrender = II2 = Q2 = re = im = I1 = Q1 = jl = jq = phase = deltaPhase = period = smooth;

    for (int i = 0; i < source.size(); ++i)
    {
        if (i < 6)
        {
            deltaPhase[ i ].setY(0);
            II2[ i ].setY(0);
            Q2[ i ].setY(0);
            re[ i ].setY(0);
            im[ i ].setY(0);
            period[ i ].setY(6);
        }
        else
        {
            factor = 0.075 * period[ i - 1 ].y() + 0.54;
            detrender[ i ].setY(((0.0962 * smooth[ i ].y() + 0.5769 * smooth[ i - 2 ].y() - 0.5769 * smooth[ i - 4].y() - 0.0962 * smooth[ i - 6 ].y()) * factor));

            Q1[ i ].setY(((0.0962 * detrender[ i ].y() + 0.5769 * detrender[ i - 2 ].y() - 0.5769 * detrender[ i - 4 ].y() - 0.0962 * detrender[ i - 6 ].y()) * factor));
            I1[ i ] = detrender[ i - 3 ];

            jl[ i ].setY(((0.0962 * I1[ i ].y() + 0.5769 * I1[ i - 2 ].y() - 0.5769 * I1[ i - 4 ].y() - 0.0962 * I1[ i - 6 ].y()) * factor));

            jq[ i ].setY(((0.0962 * Q1[ i ].y() + 0.5769 * Q1[ i - 2 ].y() - 0.5769 * Q1[ i - 4 ].y() - 0.0962 * Q1[ i - 6 ].y()) * factor));

            II2[ i ].setY(I1[ i ].y() - jq[ i ].y());

            Q2[ i ].setY(Q1[ i ].y() + jl[ i ].y());

            II2[ i ].setY(0.2 * II2[ i ].y() + 0.8 * II2[ i - 1 ].y());

            Q2[ i ].setY(0.2 * Q2[ i ].y() + 0.8 * Q2[ i - 1 ].y());

            re[ i ].setY(II2[ i ].y() * II2[ i - 1 ].y() + Q2[ i ].y() * Q2[ i - 1 ].y());

            im[ i ].setY(II2[ i ].y() * Q2[ i - 1 ].y() - Q2[ i ].y() * II2[ i - 1 ].y());

            re[ i ].setY(0.2 * re[ i ].y() + 0.8 * re[ i - 1 ].y());

            im[ i ].setY(0.2 * im[ i ].y() + 0.8 * im[ i - 1 ].y());

            period[ i ] = period[ i - 1 ];

            if (!qFuzzyCompare(im[ i ].y(), 0) && !qFuzzyCompare(re[ i ].y(), 0))
                period[ i ].setY((2 * PI) / atan(im[ i ].y() / re[ i ].y()));

            if (period[ i ].y() > 1.5 * period[ i - 1 ].y())
                period[ i ].setY(1.5 * period[ i - 1 ].y());

            if (period[ i ].y() < 0.67 * period[ i - 1 ].y())
                period[ i ].setY(0.67 * period[ i - 1 ].y());

            if (period[ i ].y() < 6)
                period[ i ].setY(6);

            if (period[ i ].y() > 50)
                period[ i ].setY(50);

            period[ i ].setY(0.2 * period[ i ].y() + 0.8 * period[ i - 1 ].y());

            if (!qFuzzyCompare(I1[ i ].y(), 0))
                phase[ i ].setY((180 / PI) * atan(Q1[ i ].y() / I1[ i ].y()));

            deltaPhase[ i ].setY(phase[ i - 1 ].y() - phase[ i ].y());

            if (deltaPhase[ i ].y() < 1)
                deltaPhase[ i ].setY(1);

            alpha = fastLimit / deltaPhase[ i ].y();

            if (alpha < slowLimit)
                alpha = slowLimit;

            dest[ i ].setY(0.5 * alpha * source[ i ].y() + (1 - 0.5 * alpha) * dest[ i - 1].y());
        }
    }

    for (int i = 0; i < 6; ++i)
        dest.remove(0);
}

void Indicator::calculateVolatilityCurve(const PlotCurve &source, PlotCurve &dest, const PeriodOptions &options)
{
    QVector<QPointF> buffer;

    calculateVolatilityCurve(source.data(), buffer, options);
    dest.setData(buffer);
}

void Indicator::calculateVolatilityCurve(const QVector<QPointF> &source, QVector<QPointF> &dest, const PeriodOptions &options)
{
    double volatility;
    double sum;
    int periods;
    QPointF point;

    int sourceSize = source.size();
    for (int i = 0; i < sourceSize; ++i)
    {
        if (i < options.periods)
            continue;

        periods = autoPeriodsForX(source[ i ].x(), options);
        if (i < periods)
            periods = i;

        volatility = 0;
        for (int j = 0; j < periods; ++j)
        {
            sum = source[ i - j ].y() - source[ i - 1 - j ].y();
            if (sum < 0)
                sum = sum * -1;
            volatility = volatility + sum;
        }

        point.setX(source[ i ].x());
        point.setY(volatility);
        dest.append(point);
    }
}

void Indicator::calculateERCurve(const PlotCurve &source, PlotCurve &dest, const PeriodOptions &options)
{
    QVector<QPointF> buffer;

    calculateERCurve(source.data(), buffer, options);
    dest.setData(buffer);
}

void Indicator::calculateERCurve(const QVector<QPointF> &source, QVector<QPointF> &dest, const PeriodOptions &options)
{
    double direction;
    double er;
    int periods;

    calculateVolatilityCurve(source, dest, options);

    int sourceSize = source.size();
    for (int i = 0; i < sourceSize; ++i)
    {
        if (i < options.periods)
            continue;

        periods = autoPeriodsForX(source[i].x(), options);
        if (i < periods)
            periods = i;

        direction = source[i].y() - source[i - periods].y();
        er = direction / dest[findAddressForX(i, dest)].y();
        if (er != er)
            er = 0;

        dest[findAddressForX(i, dest)].setY(er);
    }
}

void Indicator::calculateTRCurve(const PlotCurve &high, const PlotCurve &low, const PlotCurve &close, PlotCurve &dest, int size)
{
    QVector<QPointF> buffer;

    calculateTRCurve(high.data(), low.data(), close.data(), buffer, size);
    dest.setData(buffer);
}

#include <unistd.h>
void Indicator::calculateTRCurve(const QVector< QPointF > &high, const QVector< QPointF > &low, const QVector< QPointF > &close, QVector< QPointF > &dest, int size)
{
    double tr;
    double tmp;
    QPointF point;

    for (int i = 0; i < size; ++i)
    {
        if (i == 0)
        {
            tr = high.at(i).y() - low.at(i).y();
            point.setX(close.at(i).x());
            point.setY(tr);
            dest.append(point);

            continue;
        }

        tmp = high.at(i).y() - low.at(i).y();
        tr = tmp;

        tmp = high.at(i).y() - close.at(i - 1).y();
        if (tmp < 0)
            tmp = tmp * -1;

        if (tmp > tr)
            tr = tmp;

        tmp = low.at(i).y() - close.at(i - 1).y();
        if (tmp < 0)
            tmp = tmp * -1;

        if (tmp > tr)
            tr = tmp;

        point.setX(close.at(i).x());
        point.setY(tr);
        dest.append(point);
    }
}

void Indicator::calculateRSICurve(const PlotCurve &source, PlotCurve &dest, int periods, int type, int subType, int slowSmoothing, int fastSmoothing)
{
    (void)subType;

    QVector<QPointF> buffer;

    calculateRSICurve(source.data(), buffer, periods, type, slowSmoothing, fastSmoothing);
    dest.setData(buffer);
}

void Indicator::calculateRSICurve(const QVector< QPointF > &source, QVector< QPointF > &dest, int periods, int type, int subType, int slowSmoothing, int fastSmoothing)
{
    QVector<QPointF> riseData;
    QVector<QPointF> fallData;
    QVector<QPointF> riseCurve;
    QVector<QPointF> fallCurve;
    QVector<QPointF> rsiData;
    QPointF point;

    for (int i = 1; i < source.size(); ++i)
    {

        point.setX(source.at(i).x());
        if (source.at(i).y() > source.at(i - 1).y())
        {
            point.setY(source.at(i).y() - source.at(i - 1).y());
            riseData.append(point);
            point.setY(0);
            fallData.append(point);
        }
        else if (source.at(i).y() < source.at(i - 1).y())
        {
            point.setY(0);
            riseData.append(point);
            point.setY(source.at(i - 1).y() - source.at(i).y());
            fallData.append(point);
        }
        else
        {
            point.setY(0);
            riseData.append(point);
            fallData.append(point);
        }
    }

    calculateMACurve(riseData, riseCurve, periods, type, subType, slowSmoothing, fastSmoothing);
    calculateMACurve(fallData, fallCurve, periods, type, subType, slowSmoothing, fastSmoothing);

    for (int i = 0; i < riseCurve.size(); ++i)
    {
        point.setX(riseCurve[ i ].x());
        point.setY(100 - (100 / (1 + (riseCurve[ i ].y() / fallCurve[ i ].y()))));
        dest.append(point);
    }
}

void Indicator::calculateMedianCurve(const PlotCurve &source1, const PlotCurve &source2, PlotCurve &dest)
{
    QVector<QPointF> buffer;

    calculateMedianCurve(source1.data(), source2.data(), buffer);
    dest.setData(buffer);
}

void Indicator::calculateMedianCurve(const QVector< QPointF > &source1, const QVector< QPointF > &source2, QVector< QPointF > &dest)
{
    dest.clear();
    for (int i = 0; i < source1.size() && i < source2.size(); ++i)
    {
        QPointF point(source1.at(i).x(), (source1.at(i).y() + source2.at(i).y()) / 2.0);
        dest.append(point);
    }
}

void Indicator::calculateMedianCurve(const QVector< QPointF > &source1, const QVector< QPointF > &source2, Array &dest)
{
    QPointF point;

    for (int i = 0; i < source1.size() && i < source2.size(); ++i)
        dest[i] = (source1.at(i).y() + source2.at(i).y()) / 2.0;
}

void Indicator::calculateMedianCurve(const float *source1, const float *source2, double *dest, int size) const
{
    for (int i = 0; i < size; ++i)
        dest[i] = (source1[i] + source2[i]) / 2.0;
}

double Indicator::median(const Array &price, int per, int bar)
{
    double temp;
    double temparray[per];
    for (int k = per - 1; k >= 0; k--)
    {
        temparray[k] = price[bar + k - (per - 1)];
    }

    temp = 0.0;
    for (int k = per - 1; k > 0; k--)
    {
        for (int j = per - 1; j > 0; j--)
        {
            if (temparray[j - 1] > temparray[j])
            {
                temp = temparray[j - 1];
                temparray[j - 1] = temparray[j];
                temparray[j] = temp;
            }
        }
    }

    return temparray[per - 1 - (per / 2) ];
}

double Indicator::median(const double *price, int per, int bar) const
{
    double temp;
    double temparray[per];
    for (int k = per - 1; k >= 0; k--)
    {
        temparray[k] = price[bar + k - (per - 1)];
    }

    temp = 0.0;
    for (int k = per - 1; k > 0; k--)
    {
        for (int j = per - 1; j > 0; j--)
        {
            if (temparray[j - 1] > temparray[j])
            {
                temp = temparray[ j - 1 ];
                temparray[j - 1] = temparray[j];
                temparray[j] = temp;
            }
        }
    }

    return temparray[per - 1 - (per / 2)];
}

void Indicator::zoomToSelectedItem(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
    struct transaction previousTransaction;
    struct transaction currentTransaction;
    struct transaction nextTransaction;

    if (previousRow < 0 || previousColumn < 0)
        return;

    (void) currentColumn;
    (void) previousColumn;

    if (!privateData->transactionTableForm.tableWidget)
        return;

    QTableWidgetItem *currentItem = privateData->transactionTableForm.tableWidget->item(currentRow, 0);
    QTableWidgetItem *previousItem = privateData->transactionTableForm.tableWidget->item(previousRow, 0);
    QTableWidgetItem *nextItem = privateData->transactionTableForm.tableWidget->item(currentRow + 1, 0);

    if (!currentItem)
        return;

    double preferedPeriodsInZoom = (indexPlot->periods / (privateData->transactionTableForm.tableWidget->rowCount() + 1)) * 5 ;
    if (previousItem)
    {
        previousTransaction.x = previousItem->text().toDouble();
        previousItem = privateData->transactionTableForm.tableWidget->item(previousRow, 3);
        if (previousItem->text() == tr("Long"))
            previousTransaction.type = Long;
        else if (previousItem->text() == tr("Short"))
            previousTransaction.type = Short;
        else if (previousItem->text() == tr("Exit"))
            previousTransaction.type = Exit;

        if (markerPoints)
            markerPoints->unHighlightMarker(previousTransaction);
    }

    currentTransaction.x = currentItem->text().toDouble();
    currentItem = privateData->transactionTableForm.tableWidget->item(currentRow, 3);
    if (currentItem->text() == tr("Long"))
        currentTransaction.type = Long;
    else if (currentItem->text() == tr("Short"))
        currentTransaction.type = Short;
    else if (currentItem->text() == tr("Exit"))
        currentTransaction.type = Exit;

    if (markerPoints)
        markerPoints->highlightMarker(currentTransaction);

    if (nextItem)
    {
        nextTransaction.x = nextItem->text().toDouble();
        nextItem = privateData->transactionTableForm.tableWidget->item(currentRow + 1, 3);
        if (nextItem->text() == tr("Long"))
            nextTransaction.type = Long;
        else if (nextItem->text() == tr("Short"))
            nextTransaction.type = Short;
        else if (nextItem->text() == tr("Exit"))
            nextTransaction.type = Exit;
        indexPlot->zoomGoto(preferedPeriodsInZoom, currentTransaction.x - 5, nextTransaction.x);
    }
    else
        indexPlot->zoomGoto(preferedPeriodsInZoom, currentTransaction.x - 5, indexPlot->periods);


}

/*inline void Indicator::processTransaction(struct transaction *data)
{
    double transactionCosts;

    privateData->totalTransactions += 1;
    if ((data->type == Long) || (data->type == Short))
    {
        transactionStartIndexValue = data->value;
        privateData->transactionHighIndexValue = data->value;
        privateData->transactionLowIndexValue = data->value;

        //transactionCosts = finalCapital - ( ( ( finalCapital - strategiePage->fixedTransactionCosts ) / ( finalCapital * ( 1.0 + strategiePage->variableTransactionCosts ) ) ) * finalCapital );
        transactionCosts = (((finalCapital - priceChart->fixedTransactionCosts) / (1.0 + (priceChart->variableTransactionCosts))) * priceChart->variableTransactionCosts) + priceChart->fixedTransactionCosts;
        if (transactionCosts < priceChart->minimumTransactionCosts)
            transactionCosts = priceChart->minimumTransactionCosts;
        else if (transactionCosts > priceChart->maximumTransactionCosts)
            transactionCosts = priceChart->maximumTransactionCosts;

        privateData->totalTransactionCosts += transactionCosts;
        finalCapital = finalCapital - transactionCosts;
        finalCapital = finalCapital - (finalCapital * (priceChart->averageSpread / 2.0));

        return;
    }

    if (previousTransactionType == Short)
    {
        data->relativeProfits = ((transactionStartIndexValue / data->value) - 1) * 100.0;
    }
    else if (previousTransactionType == Long)
    {
        data->relativeProfits = ((data->value / transactionStartIndexValue) - 1) * 100.0;
    }
    else
        return;

    double previousCapital = finalCapital;
    finalCapital = ((finalCapital * data->relativeProfits) / 100.0) + finalCapital;

    finalCapital = finalCapital - (finalCapital * (priceChart->averageSpread / 2.0));

    transactionCosts = (finalCapital * priceChart->variableTransactionCosts) + priceChart->fixedTransactionCosts;

    if (transactionCosts < priceChart->minimumTransactionCosts)
        transactionCosts = priceChart->minimumTransactionCosts;
    else if (transactionCosts > priceChart->maximumTransactionCosts)
        transactionCosts = priceChart->maximumTransactionCosts;

    privateData->totalTransactionCosts += transactionCosts;
    finalCapital = finalCapital - transactionCosts;

    data->currentCapital = finalCapital;
    data->relativeProfits = ((finalCapital / previousCapital) - 1) * 100.0;

    transactionStartIndexValue = data->value;
    privateData->transactionHighIndexValue = data->value;
    privateData->transactionLowIndexValue = data->value;
    privateData->totalTrades += 1;
    if (data->relativeProfits < 0)
        privateData->totalLossTrades += 1;

    privateData->profitsTakenIndexValue = 0.0;
}

inline double Indicator::calculateTransactionCosts(double capital) const
{
    double transactionCosts = (((capital - priceChart->fixedTransactionCosts) / (1.0 + (priceChart->variableTransactionCosts))) * priceChart->variableTransactionCosts) + priceChart->fixedTransactionCosts;

    if (transactionCosts < priceChart->minimumTransactionCosts)
        transactionCosts = priceChart->minimumTransactionCosts;
    else if (transactionCosts > priceChart->maximumTransactionCosts)
        transactionCosts = priceChart->maximumTransactionCosts;

    return transactionCosts;
}*/

void Indicator::addTransaction(float price, float spread, int bar, TransactionType transactionType, TransactionType previousTransactionType, bool stopLoss, bool takeProfits, bool finished)
{
    TradeResults *results = &privateData->tradeResults;
    float relativeProfits = 0.0f;

    results->totalTransactions += 1;


    if (transactionType == Long || transactionType == Short)
    {
    }

    if (transactionType == Long || transactionType == Short)
    {
        privateData->transactionStartValue = price;
        float transactionCosts = (((results->finalCapital - privateData->transactionCosts.fixedTransactionCosts) / (1.0 + (privateData->transactionCosts.variableTransactionCosts))) * priceChart->variableTransactionCosts) + priceChart->fixedTransactionCosts;

        if (transactionCosts < privateData->transactionCosts.minimumTransactionCosts)
            transactionCosts = privateData->transactionCosts.minimumTransactionCosts;
        else if (transactionCosts > privateData->transactionCosts.maximumTransactionCosts)
            transactionCosts = privateData->transactionCosts.maximumTransactionCosts;

        results->transactionCosts = results->transactionCosts + transactionCosts;
        results->finalCapital = results->finalCapital - transactionCosts;
        results->finalCapital = results->finalCapital - (results->finalCapital * privateData->transactionCosts.averageSpread);
    }
    else
    {
        if (previousTransactionType == Short)
            relativeProfits = (1.0 - (price / privateData->transactionStartValue)) * 100.0f;
        else
            relativeProfits = ((price / privateData->transactionStartValue) - 1.0f) * 100.0f;

        float previousCapital = results->finalCapital;
        results->finalCapital = ((results->finalCapital * relativeProfits) / 100.0f) + results->finalCapital;
        results->finalCapital = results->finalCapital - (results->finalCapital * privateData->transactionCosts.averageSpread);

        float transactionCosts = (results->finalCapital * privateData->transactionCosts.variableTransactionCosts) + privateData->transactionCosts.fixedTransactionCosts;

        if (transactionCosts < privateData->transactionCosts.minimumTransactionCosts)
            transactionCosts = privateData->transactionCosts.minimumTransactionCosts;
        else if (transactionCosts > privateData->transactionCosts.maximumTransactionCosts)
            transactionCosts = privateData->transactionCosts.maximumTransactionCosts;

        results->transactionCosts += transactionCosts;
        results->finalCapital = results->finalCapital - transactionCosts;
        results->profits = results->finalCapital - results->startCapital;
        results->relativeProfits = ((results->finalCapital / results->startCapital) - 1.0f) * 100.0f;
        results->totalTrades += 1;

        relativeProfits = ((results->finalCapital / previousCapital) - 1.0f) * 100.0f;
        if (relativeProfits < 0)
            results->totalLossTrades += 1;
        else
            results->totalProfitTrades += 1;

        if (results->totalTrades == 0)
            results->successRate = 0.0f;
        else
            results->successRate = ((float) results->totalProfitTrades / results->totalTrades) * 100.0f;

        privateData->transactionStartValue = price;
    }

    if (processMode() == Indicator::View)
    {
        addTransactionToTransactionTable(price, bar, transactionType, relativeProfits, stopLoss, takeProfits, finished);

        if (showTransactionMarkers && finished)
        {
            struct transaction transaction;
            transaction.value = price;
            transaction.x = bar;
            transaction.stopLoss = stopLoss;
            transaction.takeProfits = takeProfits;
            transaction.type = transactionType;
            transaction.previousType = previousTransactionType;
            markerPoints->addMarker(transaction);
        }
    }

    if (!autoOptimizerMode())
    {
        if (bar == (indexPlot->periods - 1) &&
            indexPlot->timeScaleData().size() >= 1 &&
            indexPlot->timeScaleData().last() != privateData->lastAlarmTrigger.second &&
            transactionType != privateData->lastAlarmTrigger.first)
        {
            priceChart->strategieOverviewWidget()->triggerAlarm();
            QPair<TransactionType, QDateTime> alarmTrigger;
            alarmTrigger.first = transactionType;
            alarmTrigger.second = indexPlot->timeScaleData().last();
            privateData->lastAlarmTrigger = alarmTrigger;
        }
    }
}

const TradeResults &Indicator::tradeResults(void) const
{
    return privateData->tradeResults;
}

void Indicator::addTransactionToTransactionTable(float price, int bar, TransactionType type, float relativeProfits, bool stopLoss, bool takeProfits, bool finished)
{
    TradeResults *results = &privateData->tradeResults;

    bool found = false;
    for (int i = 0; i < priceChart->transactionTabWidget()->count(); ++i)
    {
        if (priceChart->transactionTabWidget()->widget(i) == privateData->transactionTableWidget)
        {
            priceChart->transactionTabWidget()->setCurrentWidget(privateData->transactionTableWidget);
            found = true;
            break;
        }
    }

    if (!found)
    {
        priceChart->transactionTabWidget()->addTab(privateData->transactionTableWidget, windowTitle());
        priceChart->transactionTabWidget()->setCurrentWidget(privateData->transactionTableWidget);
        privateData->transactionTableWidget->show();
    }

    double totalTransactions = results->totalTransactions;
    double totalFinishedTransaction = results->totalTrades;
    if (!finished)
    {
        totalTransactions -= 1;
        totalFinishedTransaction -= 1;
    }

    privateData->transactionTableForm.ttvLabel->setText(QString("%L1").arg(totalTransactions, 0, 'f', 0));
    privateData->transactionTableForm.ftvLabel->setText(QString("%L1").arg(totalFinishedTransaction, 0, 'f', 0));
    privateData->transactionTableForm.ltvLabel->setText(QString("%L1").arg(results->totalLossTrades));
    privateData->transactionTableForm.tcvLabel->setText(QString(QString(euroSign) + " " + QString("%L1").arg(results->transactionCosts, 0, 'f', 2)));
    privateData->transactionTableForm.srvLabel->setText("% " + QString("%L1").arg(results->successRate, 0, 'f', 0));


    if (type != Exit)
    {
        int transactionTableRow = privateData->transactionTableForm.tableWidget->rowCount();
        privateData->transactionTableForm.tableWidget->setRowCount(transactionTableRow + 1);

        QTableWidgetItem *typeItem = new QTableWidgetItem();
        typeItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        typeItem->setText((type == Long) ? tr("Long") : tr("Short"));
        privateData->transactionTableForm.tableWidget->setItem(transactionTableRow , 3, typeItem);

        QTableWidgetItem *XItem = new QTableWidgetItem(QString().setNum(bar, 10));
        XItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        privateData->transactionTableForm.tableWidget->setItem(transactionTableRow, 0, XItem);

        QTableWidgetItem *dateItem;
        int address = indexPlot->findAddressForX(bar);
        if (address >= 0 && address < indexPlot->timeScaleData().size())
            dateItem = new QTableWidgetItem(indexPlot->timeScaleData().at(address).date().toString());
        else
            dateItem = new QTableWidgetItem(QString().setNum(bar, 10));
        dateItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        privateData->transactionTableForm.tableWidget->setItem(transactionTableRow, 1, dateItem);

        QTableWidgetItem *startValueItem = new QTableWidgetItem(QString("%L1").arg(price, 0, 'f', 2));
        startValueItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        privateData->transactionTableForm.tableWidget->setItem(transactionTableRow, 8, startValueItem);
    }
    else
    {
        int transactionTableRow = privateData->transactionTableForm.tableWidget->rowCount() - 1;
        if (finished)
        {
            QTableWidgetItem *dateItem;
            int address = indexPlot->findAddressForX(bar);
            if (address >= 0 && address < indexPlot->timeScaleData().size())
                dateItem = new QTableWidgetItem(indexPlot->timeScaleData().at(address).date().toString());
            else
                dateItem = new QTableWidgetItem(QString().setNum(bar, 10));
            dateItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            privateData->transactionTableForm.tableWidget->setItem(transactionTableRow, 2, dateItem);
        }

        QTableWidgetItem *relativeProfitsItem = new QTableWidgetItem(QString("%L1").arg(relativeProfits, 0, 'f', 2));
        relativeProfitsItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        privateData->transactionTableForm.tableWidget->setItem(transactionTableRow, 4, relativeProfitsItem);

        QTableWidgetItem *currentCapitalItem = new QTableWidgetItem(QString("%L1").arg(results->finalCapital, 0, 'f', 2));
        currentCapitalItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        privateData->transactionTableForm.tableWidget->setItem(transactionTableRow, 5, currentCapitalItem);

        QTableWidgetItem *stopLossItem = new QTableWidgetItem();
        stopLossItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        stopLossItem->setText(stopLoss ? tr("Ja") : tr("Nee"));
        privateData->transactionTableForm.tableWidget->setItem(transactionTableRow, 6, stopLossItem);

        QTableWidgetItem *takeProfitsItem = new QTableWidgetItem();
        takeProfitsItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        takeProfitsItem->setText(takeProfits ? tr("Ja") : tr("Nee"));
        privateData->transactionTableForm.tableWidget->setItem(transactionTableRow, 7, takeProfitsItem);

        QTableWidgetItem *endValueItem = new QTableWidgetItem(QString("%L1").arg(price, 0, 'f', 2));
        endValueItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        privateData->transactionTableForm.tableWidget->setItem(transactionTableRow, 9, endValueItem);

        privateData->transactionTableForm.scvLabel->setText(QString(QString(euroSign) + " " + QString("%L1").arg(results->startCapital, 0, 'f', 2)));
        privateData->transactionTableForm.fcvLabel->setText(QString(QString(euroSign) + " " + QString("%L1").arg(results->finalCapital, 0, 'f', 2)));
        privateData->transactionTableForm.pvLabel->setText(QString(QString(euroSign) + " " + QString("%L1").arg(results->finalCapital - results->startCapital, 0, 'f', 2)));
        privateData->transactionTableForm.rpvLabel->setText("% " + QString("%L1").arg(((results->finalCapital / results->startCapital) - 1) * 100.0, 0, 'f', 2));

        if (!finished)
        {
            QBrush brush;
            brush.setColor(Qt::darkRed);

            QTableWidgetItem *item;
            for (int i = 0; i < privateData->transactionTableForm.tableWidget->columnCount(); ++i)
            {
                item = privateData->transactionTableForm.tableWidget->item(transactionTableRow, i);
                if (item)
                    item->setForeground(brush);
            }
        }
    }
}

void Indicator::createStopLossCache(bool reuseOldCache)
{
    quint64 key = 0;
    quint8 *ptr = (quint8 *)&key;
    quint8 type = static_cast<qint8>(privateData->stopLossOptions.type);
    memcpy(ptr, &type, sizeof(type));
    memcpy(ptr + 2, &privateData->stopLossOptions.isRelative, sizeof(privateData->stopLossOptions.isRelative));
    if (privateData->stopLossOptions.isRelative)
        memcpy(ptr + 4, &privateData->stopLossOptions.relativeValue, sizeof(privateData->stopLossOptions.relativeValue));
    else
        memcpy(ptr + 4, &privateData->stopLossOptions.absoluteValue, sizeof(privateData->stopLossOptions.absoluteValue));

    if (privateData->stopLossCache.contains(key))
    {
        //        qDebug() << "createStopLossCache(): reusing key " << key;
        privateData->stopLossCacheEntry = privateData->stopLossCache.object(key);
        return;
    }

    //    qDebug() << "createStopLossCache(): creating key " << key << " reuseOldCache=" << reuseOldCache;
    SignalCacheOject *object = new SignalCacheOject(indexPlot->periods, true, reuseOldCache ? privateData->stopLossCacheEntry : NULL);
    privateData->stopLossCache.insert(key, object);
    privateData->stopLossCacheEntry = object;
}

void Indicator::clearStopLossCache(void)
{
    privateData->stopLossCache.clear();
    privateData->stopLossCacheEntry = NULL;
}

void Indicator::createTakeProfitsCache(bool reuseOldCache)
{
    quint64 key = 0;
    quint8 *ptr = (quint8 *)&key;
    memcpy(ptr + 2, &privateData->takeProfitOptions.isRelative, sizeof(privateData->takeProfitOptions.isRelative));
    if (privateData->takeProfitOptions.isRelative)
        memcpy(ptr + 4, &privateData->takeProfitOptions.relativeValue, sizeof(privateData->takeProfitOptions.relativeValue));
    else
        memcpy(ptr + 4, &privateData->takeProfitOptions.absoluteValue, sizeof(privateData->takeProfitOptions.absoluteValue));

    if (privateData->takeProfitsCache.contains(key))
    {
        //        qDebug() << "createTakeProfitsCache(): reusing key " << key;
        privateData->takeProfitsCacheEntry = privateData->takeProfitsCache.object(key);
        return;
    }

    //        qDebug() << "createTakeProfitsCache(): creating key " << key << " reuseOldCache=" << reuseOldCache;
    SignalCacheOject *object = new SignalCacheOject(indexPlot->periods, false, reuseOldCache ? privateData->takeProfitsCacheEntry : NULL);
    privateData->takeProfitsCache.insert(key, object);
    privateData->takeProfitsCacheEntry = object;
}

void Indicator::clearTakeProfitsCache(void)
{
    privateData->takeProfitsCache.clear();
    privateData->takeProfitsCacheEntry = NULL;
}

inline float Indicator::setLosingTradePrice(float startPrice, TransactionType transactionType, int currentBar, int transactionStart) const
{
    if (privateData->stopLossOptions.isRelative)
    {
        if (qFuzzyIsNull(privateData->stopLossOptions.relativeValue))
            return 0.0f;

        if (transactionType == Long)
        {
            privateData->stopLossCacheEntry->highCache()[transactionStart] = startPrice;
            return startPrice - (startPrice * privateData->stopLossOptions.relativeValue);
        }
        else if (transactionType == Short)
        {
            privateData->stopLossCacheEntry->lowCache()[transactionStart] = startPrice;
            return startPrice + (startPrice * privateData->stopLossOptions.relativeValue);
        }

    }
    else
    {
        if (qFuzzyIsNull(privateData->stopLossOptions.absoluteValue))
            return 0.0f;

        if (transactionType == Long)
        {
            privateData->stopLossCacheEntry->highCache()[transactionStart] = startPrice;
            return startPrice - privateData->stopLossOptions.absoluteValue;
        }
        else if (transactionType == Short)
        {
            privateData->stopLossCacheEntry->lowCache()[transactionStart] = startPrice;
            return startPrice + privateData->stopLossOptions.absoluteValue;
        }
    }

    return 0.0f;
}

inline bool Indicator:: isLosingTrade(float &losingPrice, float askHigh, float bidLow, TransactionType transactionType, int currentBar, int transactionStart) const
{
    if (qFuzzyIsNull(losingPrice))
        return false;

    if (currentBar < privateData->stopLossCacheEntry->signalCache()[transactionType][transactionStart])
        return false;

    if (privateData->stopLossOptions.type == StopLossOptions::Trailing)
    {
        if (transactionType == Long && askHigh > privateData->stopLossCacheEntry->highCache()[transactionStart])
            losingPrice = setLosingTradePrice(askHigh, transactionType, currentBar, transactionStart);
        else if (transactionType == Short && bidLow < privateData->stopLossCacheEntry->lowCache()[transactionStart])
            losingPrice = setLosingTradePrice(bidLow, transactionType, currentBar, transactionStart);
    }

    bool result = false;
    if (transactionType == Long)
    {
        if (bidLow < losingPrice)
            result = true;
    }
    else if (transactionType == Short)
    {
        if (askHigh > losingPrice)
            result = true;
    }

    if (currentBar > privateData->stopLossCacheEntry->signalCache()[transactionType][transactionStart])
        privateData->stopLossCacheEntry->signalCache()[transactionType][transactionStart] = currentBar;

    return result;
}

inline float Indicator::losingTradePrice(float losingPrice, float askOpen, float bidOpen, TransactionType transactionType) const
{
    if (transactionType == Long)
    {
        if (bidOpen < losingPrice)
            return bidOpen;
        else
            return losingPrice;
    }
    else if (transactionType == Short)
    {
        if (askOpen > losingPrice)
            return askOpen;
        else
            return losingPrice;
    }

    return FLT_MAX;
}

inline float Indicator::setWinningTradePrice(float startPrice, TransactionType transactionType) const
{
    if (privateData->takeProfitOptions.isRelative)
    {
        if (qFuzzyIsNull(privateData->takeProfitOptions.relativeValue))
            return 0.0f;

        if (transactionType == Long)
            return startPrice + (startPrice * privateData->takeProfitOptions.relativeValue);
        else if (transactionType == Short)
            return startPrice - (startPrice * privateData->takeProfitOptions.relativeValue);
    }
    else
    {
        if (qFuzzyIsNull(privateData->takeProfitOptions.absoluteValue))
            return 0.0f;

        if (transactionType == Long)
            return startPrice + privateData->takeProfitOptions.absoluteValue;
        else if (transactionType == Short)
            return startPrice - privateData->takeProfitOptions.absoluteValue;
    }

    return 0.0f;
}

inline bool Indicator::isWinningTrade(float winningPrice, float bidHigh, float askLow, TransactionType transactionType, int currentBar, int transactionStart) const
{
    if (qFuzzyIsNull(winningPrice))
        return false;

    if (currentBar < privateData->takeProfitsCacheEntry->signalCache()[transactionType][transactionStart])
        return false;

    bool result = false;
    if (transactionType == Long)
    {
        if (bidHigh > winningPrice)
            result = true;
    }
    else if (transactionType == Short)
    {
        if (askLow < winningPrice)
            result = true;
    }

    if (currentBar > privateData->takeProfitsCacheEntry->signalCache()[transactionType][transactionStart])
        privateData->takeProfitsCacheEntry->signalCache()[transactionType][transactionStart] = currentBar;

    return result;
}

inline float Indicator::winningTradePrice(float winningPrice, float bidOpen, float askOpen, TransactionType transactionType) const
{
    if (transactionType == Long)
    {
        if (bidOpen > winningPrice)
            return bidOpen;
        else
            return winningPrice;
    }
    else if (transactionType == Short)
    {
        if (askOpen < winningPrice)
            return askOpen;
        else
            return winningPrice;
    }

    return -FLT_MAX;
}

inline bool Indicator::isLosingStrategy(int currentBar, float price, TransactionType transactionType, const SessionInfo &sessionInfo) const
{
    if (sessionInfo.maximumAllowedCapitalLoss > 0.0f)
    {
        float relativeProfits = ((tradeResults().finalCapital / tradeResults().startCapital) - 1.0f) * 100.0f;
        if (relativeProfits < (sessionInfo.maximumAllowedCapitalLoss * -1.0f))
            return true;
    }

    if (sessionInfo.deadStrategyCutOffRate > 0.0f)
    {
        if (tradeResults().totalTransactions == 0 && currentBar >= sessionInfo.deadStrategyCutOffPoint)
            return true;
    }

#if USE_SUSPECTED_DEAD_STRATEGIE_SKIPPING
    if (currentBar >= (indexPlot->periods / 10))
    {
        if (sessionInfo.minimumRequieredTrades > 0 && (tradeResults().totalTrades * 10) < sessionInfo.minimumRequieredTrades)
            return true;
    }

    if (currentBar >= (indexPlot->periods / 2))
    {
        if (sessionInfo.minimumRequieredSuccessRate > 0.0f && (tradeResults().successRate) < sessionInfo.minimumRequieredSuccessRate)
            return true;
    }
#endif

    if (autoOptimizer().maximumResultsVisible() != sessionInfo.tradeResultsCount || sessionInfo.tradeResultsCount <= 0)
        return false;

#if USE_SUSPECTED_DEAD_STRATEGIE_SKIPPING
    //    float periodProfitRate = powf((sessionInfo.bestTradeResults.relativeProfits + 100.0f) / 100.0f, 1.0f / static_cast<float>(indexPlot->periods));
    //    float periodsLeft = static_cast<float>(indexPlot->periods - currentBar);
    //    float availableProfitRate = powf(periodProfitRate, periodsLeft);
    long double periodProfitRate = powl_fast((sessionInfo.bestTradeResults.relativeProfits + 100.0L) / 100.0L, 1.0L / static_cast<long double>(indexPlot->periods));
    long double periodsLeft = static_cast<long double>(indexPlot->periods - currentBar);
    float availableProfitRate = static_cast<float>(powl_fast(periodProfitRate, periodsLeft));

#else
    if (currentBar <= (indexPlot->periods / 10))
        return false;

    const QPointF &currentProfitsMapPoint = sessionInfo.maximumProfitsMap.at(currentBar);
    const QPointF &lastProfitsMapPoint = sessionInfo.maximumProfitsMap.last();
    float availableProfitRate = static_cast<float>(lastProfitsMapPoint.y() / currentProfitsMapPoint.y());
#endif
    float newFinalCapital = tradeResults().finalCapital;
    if (transactionType == Short)
    {
        float relativeProfits = (1.0f - (price / privateData->transactionStartValue)) * 100.0f;
        newFinalCapital = ((tradeResults().finalCapital * relativeProfits) / 100.0f) + tradeResults().finalCapital;
    }

    else if (transactionType == Long)
    {
        float relativeProfits = ((price / privateData->transactionStartValue) - 1.0f) * 100.0f;
        newFinalCapital = ((tradeResults().finalCapital * relativeProfits) / 100.0f) + tradeResults().finalCapital;
    }

    float maximumFinalCapital = newFinalCapital * availableProfitRate;
    if (maximumFinalCapital < sessionInfo.worstTradeResults.finalCapital)
        return true;

    return false;
}

void Indicator::setToolTipForItem(QwtPlotSvgItem &item)
{
    QTableWidget *table = privateData->transactionTableForm.tableWidget;
    if (!table)
        return;

    double x = item.boundingRect().center().x() - 0.2;
    int found = -1;
    for (int i = 0; i < table->rowCount(); ++i)
    {
        if (table->item(i, 0)->text() == QString::number(x))
        {
            found = i;
            break;
        }
    }

    if (found < 0)
        return;

    QByteArray icon;
    if (table->item(found, 3)->text() == tr("Long"))
    {
        icon = markerPoints->svgIcon(MarkerPoints::IconLong);
    }
    else
    {
        icon = markerPoints->svgIcon(MarkerPoints::IconShort);
    }

    QString text;
    text.sprintf("%s: %s.<br />%s: %s.<br />%s: %s <br />%s: %s <br />Type: %s <br />Winst: %s \% <br />Kapitaal: %s %s <br />%s: %s <br />%s: %s<br />",
                 tr("Start datum").toAscii().data(),
                 table->item(found, 1)->text().toAscii().data(),
                 tr("Start koers").toAscii().data(),
                 table->item(found, 8)->text().toAscii().data(),
                 tr("Eind datum").toAscii().data(),
                 table->item(found, 2) ? table->item(found, 2)->text().toAscii().data() : "?",
                 tr("Eind koers").toAscii().data(),
                 table->item(found, 9)->text().toAscii().data(),
                 table->item(found, 3)->text().contains("L") ? "Long" : "Short",
                 table->item(found, 4)->text().toAscii().data(),
                 QString(euroSign).toLocal8Bit().data(),
                 table->item(found, 5)->text().toAscii().data(),
                 tr("Stoploss geraakt").toAscii().data(),
                 table->item(found, 6)->text().toAscii().data(),
                 tr("Winst genomen").toAscii().data(),
                 table->item(found, 7)->text().toAscii().data());

    QString toolTip = QString("<html><h1>" + title() + "</h1><br />" + "<img src=\"data:image/svg;base64," + icon.toBase64() + "\"ALIGN=RIGHT/>" + text + "</html>");
    indexPlot->setToolTip(toolTip);
}

void Indicator::unsetToolTipForItem(QwtPlotSvgItem &item)
{
    QTableWidget *table = privateData->transactionTableForm.tableWidget;
    if (!table)
        return;

    double x = item.boundingRect().center().x() - 0.2;
    int found = -1;
    for (int i = 0; i < table->rowCount(); ++i)
    {
        if (table->item(i, 0)->text() == QString::number(x))
        {
            found = i;
            break;
        }
    }

    if (found < 0)
        return;

    indexPlot->setToolTip("");
    QToolTip::hideText();
}

void Indicator::setColorButtonHidden(bool hidden)
{
    privateData->colorButtonHidden = hidden;
}

void Indicator::setIndicatorSourceComboBoxHidden(bool hidden)
{
    privateData->indicatorSourceComboBoxHidden = hidden;
}

void Indicator::setIndicatorBackGroundColor(QColor color)
{
    if (indicatorPlot == indexPlot)
        return;

    indicatorPlot->setCanvasBackground(color);
}

void Indicator::setIndicatorLegendColor(QColor color)
{
    if (indicatorPlot == indexPlot)
        return;

    QPalette palette = indicatorPlot->legend()->palette();
    palette.setBrush(QPalette::Text, color);
    indicatorPlot->legend()->setPalette(palette);
}

void Indicator::setIndicatorMajGridColor(QColor color)
{
    if (indicatorPlot == indexPlot)
        return;

    for (int i = 0; i < indicatorPlot->itemList().size(); i++)
    {
        if (!indicatorPlot->itemList().at(i)->rtti() == QwtPlotItem::Rtti_PlotGrid)
            continue;

        QwtPlotGrid *grid = dynamic_cast<QwtPlotGrid *>(indicatorPlot->itemList().at(i));
        if (!grid)
            continue;
        ;
        QPen pen = grid->majPen();
        pen.setColor(color);
        grid->setMajPen(pen);
        return;
    }
}

void Indicator::setIndicatorMinGridColor(QColor color)
{
    if (indicatorPlot == indexPlot)
        return;

    for (int i = 0; i < indicatorPlot->itemList().size(); i++)
    {
        if (!indicatorPlot->itemList().at(i)->rtti() == QwtPlotItem::Rtti_PlotGrid)
            continue;

        QwtPlotGrid *grid = dynamic_cast<QwtPlotGrid *>(indicatorPlot->itemList().at(i));
        if (!grid)
            continue;

        QPen pen = grid->minPen();
        pen.setColor(color);
        grid->setMinPen(pen);
        return;
    }
}

void Indicator::setTrackerColor(QColor color)
{
    Tracker *tracker = indicatorPlot->tracker();

    QPen pen = tracker->trackerPen();
    pen.setColor(color);
    tracker->setTrackerPen(pen);
}

bool Indicator::blockSettingsGUISignals()
{
    return privateData->blockSettingsGUISignals;
}

void Indicator::setBlockSettingsGUISignals(bool block)
{
    privateData->blockSettingsGUISignals = block;
}

AutoOptimizerOption *Indicator::addAutoOptimizerOption(const QString &title, bool *option, int min, int max, AutoOptimizerOption::optionTypes type, const QStringList &valueTitles)
{
    return privateData->autoOptimizer->addOption(this, title, option, min, max, type, valueTitles);
}

AutoOptimizerOption *Indicator::addAutoOptimizerOption(const QString &title, int *option, int min, int max, int stepSize, bool useAccelleration, AutoOptimizerOption::optionTypes type, const QStringList &valueTitles)
{
    return privateData->autoOptimizer->addOption(this, title, option, min, max, stepSize, useAccelleration, type, valueTitles);
}

AutoOptimizerOption *Indicator::addAutoOptimizerOption(const QString &title, float *option, float min, float max, double stepSize, bool useAccelleration, bool percentualNotation, AutoOptimizerOption::optionTypes type)
{
    return  privateData->autoOptimizer->addOption(this, title, option, min, max, stepSize, useAccelleration, percentualNotation, type);
}

Indicator::ProcessMode Indicator::processMode(void) const
{
    return privateData->processMode;
}

void Indicator::setProcessMode(Indicator::ProcessMode mode)
{
    if (processMode() == mode)
        return;

    privateData->processMode = mode;

    if (indexPlot != indicatorPlot)
    {
        if (indexPlot->zoomer()->zoomRectIndex() != 0)
        {
            QStack<QRectF> indexStack = indexPlot->zoomer()->zoomStack();

            QRectF rect;
            for (unsigned int i = 0; i < (indexPlot->zoomer()->zoomRectIndex() + 1); ++i)
            {
                rect = indexStack.at(i);
                rect.setBottom(100);
                rect.setTop(0);
                indexStack[ i ] = rect;
            }

            indicatorPlot->zoomer()->setZoomStack(indexStack);
        }

        for (int i = 0; i < priceChart->plotList().size(); i++)
        {
            Plot *plot = priceChart->plotList().at(i);
            if (plot == indicatorPlot)
                continue;

            plot->canvas()->removeEventFilter(indicatorPlot->tracker());
            indicatorPlot->canvas()->removeEventFilter(plot->tracker());

            if (processMode() != Indicator::Hidden)
            {
                plot->canvas()->installEventFilter(indicatorPlot->tracker());
                indicatorPlot->canvas()->installEventFilter(plot->tracker());
            }
        }

        if (processMode() == Indicator::Hidden)
        {
            int index = priceChart->plotSplitterWidget()->indexOf(indicatorPlot);
            if (index > 0)
            {
                indicatorPlot->setParent(NULL);
                indicatorPlot->setVisible(false);
            }

            if (priceChart->plotList().contains(indicatorPlot))
                priceChart->plotList().removeAll(indicatorPlot);

            disconnect(indexPlot, SIGNAL(zoomedIn()), indicatorPlot, SLOT(zoomIn()));
            disconnect(indexPlot, SIGNAL(zoomedIn(int)), indicatorPlot, SLOT(zoomIn(int)));
            disconnect(indexPlot, SIGNAL(zoomedOut()), indicatorPlot, SLOT(zoomOut()));
            disconnect(indexPlot, SIGNAL(zoomedGoto(double, double, double)), indicatorPlot, SLOT(zoomGoto(double, double, double)));
            disconnect(indexPlot, SIGNAL(zoomedReset()), indicatorPlot, SLOT(zoomReset()));
            disconnect(indexPlot->zoomer(), SIGNAL(zoomed(const QwtDoubleRect &)), this, SLOT(setZoomPositionToMainPlot(const QwtDoubleRect &)));
        }
        else
        {
            if (priceChart->plotSplitterWidget()->indexOf(indicatorPlot) < 0)
            {
                QList<int> sizesList = priceChart->plotSplitterWidget()->sizes();
                sizesList << 100;
                priceChart->plotSplitterWidget()->addWidget(indicatorPlot);
                priceChart->plotSplitterWidget()->setSizes(sizesList);
                indicatorPlot->setVisible(true);
            }

            if (!priceChart->plotList().contains(indicatorPlot))
                priceChart->plotList().append(indicatorPlot);

            connect(indexPlot, SIGNAL(zoomedIn()), indicatorPlot, SLOT(zoomIn()), Qt::UniqueConnection);
            connect(indexPlot, SIGNAL(zoomedIn(int)), indicatorPlot, SLOT(zoomIn(int)), Qt::UniqueConnection);
            connect(indexPlot, SIGNAL(zoomedOut()), indicatorPlot, SLOT(zoomOut()), Qt::UniqueConnection);
            connect(indexPlot, SIGNAL(zoomedGoto(double, double, double)), indicatorPlot, SLOT(zoomGoto(double, double, double)), Qt::UniqueConnection);
            connect(indexPlot, SIGNAL(zoomedReset()), indicatorPlot, SLOT(zoomReset()), Qt::UniqueConnection);
            connect(indexPlot->zoomer(), SIGNAL(zoomed(const QwtDoubleRect &)), this, SLOT(setZoomPositionToMainPlot(const QwtDoubleRect &)), Qt::UniqueConnection);
        }

        for (int i = 0; i < priceChart->plotList().size(); i++)
        {
            Plot *plot = priceChart->plotList().at(i);
            if (i == (priceChart->plotList().size() - 1))
                plot->enableAxis(QwtPlot::xBottom, true);
            else
                plot->enableAxis(QwtPlot::xBottom, false);
        }
    }

    delete markerPoints;
    markerPoints = NULL;

    if (processMode() != Indicator::Hidden)
    {
        connect(priceChart , SIGNAL(dataAppended()), this, SLOT(plotCurve()), Qt::UniqueConnection);
        connect(priceChart , SIGNAL(dataSet()), this, SLOT(plotCurve()), Qt::UniqueConnection);
        connect(priceChart , SIGNAL(dataCleared()), this, SLOT(plotCurve()), Qt::UniqueConnection);
        connect(priceChart, SIGNAL(calculateProfits()), this, SLOT(executeTransactionRules()), Qt::UniqueConnection);
    }
    else
    {
        disconnect(priceChart , SIGNAL(dataAppended()), this, SLOT(plotCurve()));
        disconnect(priceChart , SIGNAL(dataSet()), this, SLOT(plotCurve()));
        disconnect(priceChart , SIGNAL(dataCleared()), this, SLOT(plotCurve()));
        disconnect(priceChart, SIGNAL(calculateProfits()), this, SLOT(executeTransactionRules()));
    }

    for (int i = 0; i < curveList.size(); i++)
    {
        PlotCurve *curve = curveList.at(i);
        curve->setProcessMode((processMode() == Indicator::View) ? PlotCurve::View : PlotCurve::Data);
    }

    if (privateData->propertiesWidget)
    {
        if (processMode() == Indicator::Hidden)
        {
            priceChart->indicatorStackWidget()->removeWidget(privateData->propertiesWidget);
            privateData->propertiesWidget->setVisible(false);
        }
        else
        {
            if (priceChart->indicatorStackWidget()->indexOf(privateData->propertiesWidget) < 0)
            {
                priceChart->indicatorStackWidget()->insertWidget(0, privateData->propertiesWidget);
                priceChart->indicatorStackWidget()->setCurrentIndex(0);
                privateData->propertiesWidget->setVisible(true);
            }
        }
    }

    if (processMode() == Indicator::View)
        executeTransactionRules();
}

bool Indicator::autoOptimizerMode(void)
{
    return privateData->autoOptimizerMode;
}

void Indicator::setAutoOptimizerMode(bool enabled)
{
    privateData->autoOptimizerMode = enabled;
}

AutoOptimizer &Indicator::autoOptimizer(void)
{
    return *privateData->autoOptimizer;
}

const AutoOptimizer &Indicator::autoOptimizer(void) const
{
    return *privateData->autoOptimizer;
}

void Indicator::setSettingsGUIEnabled(bool enabled)
{
    privateData->propertiesForm.globalWidget->setEnabled(enabled);
    privateData->propertiesForm.tradeWidget->setEnabled(enabled);
    privateData->propertiesForm.rulesWidget->setEnabled(enabled);
    privateData->propertiesForm.stopLossWidget->setEnabled(enabled);
    privateData->propertiesForm.takeProfitsWidget->setEnabled(enabled);
}
