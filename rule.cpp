#include "rule.h"
#include "bttrader.h"
#include "indicator.h"

Rule::Rule(void)
{
    parent = NULL;
    indicator = NULL;
    sourceIndicator = NULL;
    sourceCurve = NULL;
    referenceIndicator = NULL;
    referenceCurve = NULL;
    referenceValue = 0;
    enabled = false;
    compareType = (Rule::CompareType) - 1;
    logicalOperator = (Rule::LogicalOperator) - 1;
    offset = 0;
    previousTransactionType = (TransactionType) - 1;
    filter.type = RuleFilter::NoFilter;
    filter.periods = 1;
    filter.relativeDifference = 0.0;
    filter.absoluteDifference = 0.0;
}

Rule::~Rule(void)
{
}

Rule::CompareType Rule::compareTypeFromString(const QString &compareType)
{
    if (compareType.compare("Hoger dan", Qt::CaseInsensitive) == 0 || compareType.compare(QApplication::tr("Hoger dan"), Qt::CaseInsensitive) == 0)
        return IsHigher;
    else if (compareType.compare("Lager dan", Qt::CaseInsensitive) == 0 || compareType.compare(QApplication::tr("Lager dan"), Qt::CaseInsensitive) == 0)
        return IsHigher;
    else if (compareType.compare("Kruist omhoog", Qt::CaseInsensitive) == 0 || compareType.compare(QApplication::tr("Kruist omhoog"), Qt::CaseInsensitive) == 0)
        return IsCrossingUp;
    else if (compareType.compare("Kruist omlaag", Qt::CaseInsensitive) == 0 || compareType.compare(QApplication::tr("Kruist omlaag"), Qt::CaseInsensitive) == 0)
        return IsCrossingDown;
    else if (compareType.compare("Stijgt", Qt::CaseInsensitive) == 0 || compareType.compare(QApplication::tr("Stijgt"), Qt::CaseInsensitive) == 0)
        return IsRising;
    else if (compareType.compare("Daalt", Qt::CaseInsensitive) == 0 || compareType.compare(QApplication::tr("Daalt"), Qt::CaseInsensitive) == 0)
        return IsFalling;

    return (Rule::CompareType) - 1;
}

QString Rule::compareTypeTitle(Rule::CompareType type)
{
    switch (type)
    {
        case Rule::IsHigher:
            return QApplication::tr("Hoger dan");

        case Rule::IsLower:
            return QApplication::tr("Lager dan");

        case Rule::IsCrossingUp:
            return QApplication::tr("Kruist omhoog");

        case Rule::IsCrossingDown:
            return QApplication::tr("Kruist omlaag");

        case Rule::IsRising:
            return QApplication::tr("Stijgt");

        case Rule::IsFalling:
            return QApplication::tr("Daalt");
    }

    return QApplication::tr("Onbekende vergelijkings methode");
}

Rule::LogicalOperator Rule::logicalOperatorFromString(const QString logicalOperator)
{
    if (logicalOperator.compare("En", Qt::CaseInsensitive) == 0 || logicalOperator.compare(QApplication::tr("En"), Qt::CaseInsensitive) == 0)
        return AND;
    else if (logicalOperator.compare("Of", Qt::CaseInsensitive) == 0 || logicalOperator.compare(QApplication::tr("Of"), Qt::CaseInsensitive) == 0)
        return OR;

    return (Rule::LogicalOperator) - 1;
}

QString Rule::logicalOperatorTitle(Rule::LogicalOperator logicalOperator)
{
    switch (logicalOperator)
    {
        case AND:
            return QApplication::tr("En");

        case OR:
            return QApplication::tr("Of");
    }

    return QApplication::tr("Onbekende logische operator");
}

TransactionType Rule::previousTransactionTypeFromString(const QString &previousTransactionType)
{
    if (previousTransactionType.compare("Long", Qt::CaseInsensitive) == 0 || previousTransactionType.compare(QApplication::tr("Long"), Qt::CaseInsensitive) == 0)
        return Long;
    else if (previousTransactionType.compare("Short", Qt::CaseInsensitive) == 0 || previousTransactionType.compare(QApplication::tr("Short"), Qt::CaseInsensitive) == 0)
        return Short;
    else if (previousTransactionType.compare("Exit", Qt::CaseInsensitive) == 0 || previousTransactionType.compare(QApplication::tr("Exit"), Qt::CaseInsensitive) == 0)
        return Exit;
    else if (previousTransactionType.compare("Allen", Qt::CaseInsensitive) == 0 || previousTransactionType.compare(QApplication::tr("Allen"), Qt::CaseInsensitive) == 0)
        return Undefined;

    return (TransactionType) - 1;
}

QString Rule::previousTransactionTypeTitle(TransactionType previousTransactionType)
{
    switch (previousTransactionType)
    {
        case Long:
            return QApplication::tr("Long");

        case Short:
            return QApplication::tr("Short");

        case Exit:
            return QApplication::tr("Exit");

        case Undefined:
            return QApplication::tr("Allen");

        default:
            return QApplication::tr("Allen");
    }

    return QApplication::tr("Onbekende transactie type");
}

void Rule::updateTitle(void)
{
    title.clear();
    title.append("SourceIndicator=");
    if (!sourceIndicator)
        title.append("NULL");
    else if (sourceIndicator == indicator)
        title.append("this");
    else if (sourceIndicator)
        title.append(sourceIndicator->windowTitle());
    else
        title.append("NULL");
    title.append(";");

    title.append("SourceCurve=");
    if (sourceCurve)
        title.append(sourceCurve->title().text());
    else
        title.append("NULL");
    title.append(";");

    title.append("RefereneIndicator=");
    if (!referenceIndicator)
        title.append("NULL");
    else if (referenceIndicator == indicator)
        title.append("this");
    else if (referenceIndicator)
        title.append(referenceIndicator->windowTitle());
    else
        title.append("NULL");
    title.append(";");

    title.append("ReferenceCurve=");
    if (referenceCurve)
        title.append(referenceCurve->title().text());
    else
        title.append("NULL");
    title.append(";");

    title.append(QString("ReferenceValue=") + QString::number(referenceValue) + QString(";"));
    title.append(QString("Offset=") + QString::number(offset) + QString(";"));
    title.append(QString("CompareType=") + compareTypeTitle(compareType) + QString(";"));
    title.append(QString("LogicalOperator=") + logicalOperatorTitle(logicalOperator) + QString(";"));
    title.append(QString("PreviousTransactionType=") + previousTransactionTypeTitle(previousTransactionType) + QString(";"));
    title.append(QString("Enabled=") + QString::number(enabled));

    humanReadableTitle.clear();
    if (logicalOperator == Rule::OR)
        humanReadableTitle.append(QApplication::tr("Of"));
    else
        humanReadableTitle.append(QApplication::tr("En"));
    humanReadableTitle.append(" ");

    if (sourceIndicator && sourceCurve)
        humanReadableTitle.append(sourceIndicator->title() + QString("->") + sourceCurve->title().text() + " ");
    else if (sourceCurve)
        humanReadableTitle.append(sourceCurve->title().text() + " ");
    else
        humanReadableTitle.append(QApplication::tr("Onbekende Indicator en/of curve"));

    humanReadableTitle.append(compareTypeTitle(compareType) + QString(" "));

    if (referenceIndicator && referenceCurve)
        humanReadableTitle.append(referenceIndicator->title() + QString("->") + referenceCurve->title().text() + " ");
    else if (referenceCurve)
        humanReadableTitle.append(referenceCurve->title().text() + " ");
    else
        humanReadableTitle.append(QApplication::tr("Waarde=") + QString::number(referenceValue) + QString(" "));

    humanReadableTitle.append(QApplication::tr("Offset=") + QString::number(offset) + QString(" "));
    humanReadableTitle.append(QApplication::tr("Vorige transactie=") + previousTransactionTypeTitle(previousTransactionType));
}

bool Rule::isValid() const
{
    if (sourceCurve &&
        offset >= 0 &&
        (compareType >= Rule::IsHigher && compareType <= Rule::IsFalling) &&
        (logicalOperator >= Rule::AND && logicalOperator <= Rule::OR) &&
        (previousTransactionType == Short || previousTransactionType == Long || previousTransactionType == Exit || previousTransactionType == Undefined))
        return true;

    return false;
}

bool Rule::operator ==(const Rule &rule) const
{

    if (parent != rule.parent)
        return false;

    if (indicator != rule.indicator)
        return false;

    if (title != rule.title)
        return false;

    if (humanReadableTitle != rule.humanReadableTitle)
        return false;

    if (sourceIndicator != rule.sourceIndicator)
        return false;

    if (referenceIndicator != rule.referenceIndicator)
        return false;

    if (sourceCurve != rule.sourceCurve)
        return false;

    if (referenceCurve != rule.referenceCurve)
        return false;

    if (referenceValue != rule.referenceValue)
        return false;

    if (offset != rule.offset)
        return false;

    if (compareType != rule.compareType)
        return false;

    if (logicalOperator != rule.logicalOperator)
        return false;

    if (previousTransactionType != rule.previousTransactionType)
        return false;

    if (enabled != rule.enabled)
        return false;

    return true;
}

QDataStream &operator<<(QDataStream &stream, const Rule *rule)
{
    stream << rule->title << rule->children;
    return stream;
}

QDataStream &operator>>(QDataStream &stream, Rule *&rule)
{
    rule = new Rule;
    stream >> rule->title >> rule->children;
    return stream;
}
