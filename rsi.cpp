#include "rsi.h"

RSI::RSI(Plot *plot, PriceChart *priceChart, bool setupDialog, const QString &configGroup, Indicator::ProcessMode processMode) :
    Indicator(plot, priceChart, setupDialog, configGroup, processMode)
{
    rsiCurve = new PlotCurve("RSI 14,SMA", indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    rsiCurve->setBrush(QBrush(Qt::SolidPattern));
    curveList.append(rsiCurve);

    maCurve = new PlotCurve("SMA 5", indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    QPen pen = maCurve->pen();
    pen.setStyle(Qt::DotLine);
    pen.setColor(Qt::red);
    maCurve->setPen(pen);
    curveList.append(maCurve);

    erCurve = new PlotCurve("ER 14", indicatorPlot, static_cast<PlotCurve::ProcessMode>(processMode));
    erCurve->detach();
    curveList.append(erCurve);

    QwtPlotCurve *buyLine = new QwtPlotCurve;
    buyLine->setItemAttribute(QwtPlotItem::Legend, false);
    buyLine->attach(indicatorPlot);
    pen = QPen();
    pen.setColor(Qt::green);
    pen.setWidth(1);
    pen.setStyle(Qt::DashLine);
    buyLine->setPen(pen);
    QVector<QPointF> vector;
    vector << QPointF(-1, 30.0) << QPointF(100000.0, 30.0);
    buyLine->setSamples(vector);

    QwtPlotCurve *sellLine = new QwtPlotCurve;
    sellLine->setItemAttribute(QwtPlotItem::Legend, false);
    sellLine->attach(indicatorPlot);
    pen = QPen();
    pen.setColor(Qt::red);
    pen.setWidth(1);
    pen.setStyle(Qt::DashLine);
    sellLine->setPen(pen);
    vector.clear();
    vector << QPointF(-1, 70.0) << QPointF(100000.0, 70.0);
    sellLine->setSamples(vector);

    indicatorPlot->setAxisScale(QwtPlot::yLeft, 0, 100);

    rsiOptions.registerWithAutoOptimizer(this, tr("RSI:"), true);
    maOptions.registerWithAutoOptimizer(this, tr("MA:"), true);
}

RSI::~RSI(void)
{
}

void RSI::readSettingsPrivate(void)
{
    Indicator::readSettingsPrivate();
    rsiOptions = settings.value("RSIMovingAverageOptions", qVariantFromValue(MovingAverageOptions::defaultOptions(14))).value<MovingAverageOptions>();
    maOptions = settings.value("MovingAverageOptions", qVariantFromValue(MovingAverageOptions::defaultOptions(5))).value<MovingAverageOptions>();
}

void RSI::writeSettingsPrivate(void)
{
    Indicator::writeSettingsPrivate();

    settings.setValue("Type", RSIIndicator);
    settings.setValue("RSIMovingAverageOptions", qVariantFromValue(rsiOptions));
    settings.setValue("MovingAverageOptions", qVariantFromValue(maOptions));
}

void RSI::loadDefaultRules(void)
{
    clearRuleLists();

    /*    QString ruleFormula;
        IndicatorRule *rule;
        ruleFormula = QString("OR<>this->" + erCurve->title().text() + "<>Hoger dan<>Waarde<>0.2<>3<>0");
        rule = addRule(tr(ruleFormula.toAscii().data()), longRuleList, true);
        if (rule)
        {
            ruleFormula = QString("AND<>this->" + rsiCurve->title().text() + "<>Kruist omhoog<>Waarde<>40<>3<>0");
            addRule(tr(ruleFormula.toAscii().data()), rule->list, true);

            ruleFormula = QString("OR<>this->" + erCurve->title().text() + "<>Lager dan<>Waarde<>0.2<>3<>0");
            rule = addRule(tr(ruleFormula.toAscii().data()), longRuleList, true);
            if (rule)
            {
                ruleFormula = QString("AND<>this->" + erCurve->title().text() + "<>Hoger dan<>Waarde<>-0.2<>3<>0");
                rule = addRule(tr(ruleFormula.toAscii().data()), rule->list, true);
                if (rule)
                {
                    ruleFormula = QString("AND<>this->" + rsiCurve->title().text() + "<>Kruist omhoog<>Waarde<>30<>3<>0");
                    addRule(tr(ruleFormula.toAscii().data()), rule->list, true);
                }
            }
        }

        ruleFormula = QString("OR<>this->" + erCurve->title().text() + "<>Lager dan<>Waarde<>-0.2<>3<>0");
        rule = addRule(tr(ruleFormula.toAscii().data()), shortRuleList, true);
        if (rule)
        {
            ruleFormula = QString("AND<>this->" + rsiCurve->title().text() + "<>Kruist omlaag<>Waarde<>60<>3<>0");
            addRule(tr(ruleFormula.toAscii().data()), rule->list, true);

            ruleFormula = QString("OR<>this->" + erCurve->title().text() + "<>Lager dan<>Waarde<>0.2<>3<>0");
            rule = addRule(tr(ruleFormula.toAscii().data()), shortRuleList, true);
            if (rule)
            {
                ruleFormula = QString("AND<>this->" + erCurve->title().text() + "<>Hoger dan<>Waarde<>-0.2<>3<>0");
                rule = addRule(tr(ruleFormula.toAscii().data()), rule->list, true);
                if (rule)
                {
                    ruleFormula = QString("AND<>this->" + rsiCurve->title().text() + "<>Kruist omlaag<>Waarde<>70<>3<>0");
                    addRule(tr(ruleFormula.toAscii().data()), rule->list, true);
                }
            }
        }*/

    Indicator::loadDefaultRules();
}

void RSI::plotCurve(void)
{
    QPointF point;

    fallData.clear();
    fallCurve.clear();
    riseData.clear();
    riseCurve.clear();
    rsiData.clear();
    rsiCurve->clearData();
    maCurve->clearData();
    erCurve->clearData();

    if (indicatorSourceCurve->dataSize() < 1)
    {
        Indicator::plotCurve();
        return;
    }

    point.setX(indicatorSourceCurve->x(0));
    point.setY(0);
    riseData.append(point);
    fallData.append(point);


    for (int i = 1; i < indicatorSourceCurve->dataSize(); ++i)
    {

        point.setX(indicatorSourceCurve->x(i));
        if (indicatorSourceCurve->y(i) > indicatorSourceCurve->y(i - 1))
        {
            point.setY(indicatorSourceCurve->y(i) - indicatorSourceCurve->y(i - 1));
            riseData.append(point);
            point.setY(0);
            fallData.append(point);
        }
        else if (indicatorSourceCurve->y(i) < indicatorSourceCurve->y(i - 1))
        {
            point.setY(0);
            riseData.append(point);
            point.setY(indicatorSourceCurve->y(i - 1) - indicatorSourceCurve->y(i));
            fallData.append(point);
        }
        else
        {
            point.setY(0);
            riseData.append(point);
            fallData.append(point);
        }

    }

    if (rsiOptions.type == Simple || rsiOptions.type == Exponential || rsiOptions.type == LinearWeighted || rsiOptions.type == Adaptive)
    {
        calculateMACurve(riseData, riseCurve, rsiOptions);
        calculateMACurve(fallData, fallCurve, rsiOptions);
    }
    else
    {
        calculateMACurve(riseData, riseCurve, rsiOptions.periods, rsiOptions.type, -1, rsiOptions.slowSmoothing, rsiOptions.fastSmoothing);
        calculateMACurve(fallData, fallCurve, rsiOptions.periods, rsiOptions.type, -1, rsiOptions.slowSmoothing, rsiOptions.fastSmoothing);
    }

    for (int i = 0; i < riseCurve.size(); ++i)
    {
        point.setX(riseCurve[ i ].x());
        point.setY(100 - (100 / (1 + (riseCurve[ i ].y() / fallCurve[ i ].y()))));
        rsiData.append(point);
    }

    rsiCurve->setData(rsiData);


    if (maOptions.type == Simple || maOptions.type == Exponential || maOptions.type == LinearWeighted || maOptions.type == Adaptive)
        calculateMACurve(*rsiCurve, *maCurve, maOptions);
    else
        calculateMACurve(*rsiCurve, *maCurve, maOptions.periods, maOptions.type, -1, maOptions.slowSmoothing, maOptions.fastSmoothing);


    PeriodOptions options = PeriodOptions::defaultOptions();
    options.periods = rsiOptions.periods;
    calculateERCurve(*indicatorSourceCurve, *erCurve, options);

    setXAxisScaleToCurve(*indicatorSourceCurve);

    Indicator::plotCurve();
}

void RSI::updateGUI(void)
{
    bool block = blockSettingsGUISignals();
    setBlockSettingsGUISignals(true);

    rsiForm->setOptions(rsiOptions);
    maForm->setOptions(maOptions);

    setBlockSettingsGUISignals(block);

    Indicator::updateGUI();
}

QWidget *RSI::customIndicatorWidget(void)
{
    rsiForm = new MovingAverageUI(NULL, tr("RSI"));
    rsiForm->setOptions(rsiOptions);
    rsiForm->setUseHalfCycle(false);
    connect(rsiForm, SIGNAL(propertiesChanged()), this, SLOT(setProperties()));
    rsiCurve->setTitle(QString(tr("RSI ") + rsiForm->maTitle()));

    maForm = new MovingAverageUI(NULL, tr("MA"));
    maForm->setOptions(maOptions);
    maForm->setUseHalfCycle(true);
    connect(maForm, SIGNAL(propertiesChanged()), this, SLOT(setProperties()));
    maCurve->setTitle(maForm->maTitle());

    QWidget *ciWidget = new QWidget;
    ciWidget->setLayout(new QVBoxLayout);
    ciWidget->layout()->addWidget(rsiForm);
    ciWidget->layout()->addWidget(maForm);

    setTitle(QString(tr("RSI ") + rsiForm->maTitle() + "," + maForm->maTitle()));

    return ciWidget;
}

void RSI::setProperties(void)
{
    if (blockSettingsGUISignals())
        return;

    rsiForm->options(rsiOptions);
    rsiCurve->setTitle(QString(tr("RSI ") + rsiForm->maTitle()));

    maForm->options(maOptions);
    maCurve->setTitle(maForm->maTitle());

    setTitle(QString(tr("RSI ") + rsiForm->maTitle() + "," + maForm->maTitle()));

    Indicator::setProperties();
}
