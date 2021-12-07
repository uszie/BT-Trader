#ifndef RULE_H
#define RULE_H

#include "intrinsics.h"
#include "bttrader.h"

class Indicator;
class PlotCurve;
class AutoOptimizerOption;

class RuleFilter
{
    public:
        enum Filter
        {
            NoFilter,
            ClosesInRow,
            AbsoluteDifference,
            RelativeDifference
        };

        int type;
        int periods;
        float absoluteDifference;
        float relativeDifference;
};

class Rule
{
    public:
        enum CompareType
        {
            IsHigher,
            IsLower,
            IsCrossingUp,
            IsCrossingDown,
            IsRising,
            IsFalling
        };

        enum LogicalOperator
        {
            AND = 0, // Deze moeten 0 en 1 zijn ivm met een menu dat daar op ingesteld is.
            OR = 1
        };

        Rule(void);
        ~Rule(void);

        static Rule::CompareType compareTypeFromString(const QString &compareType);
        static QString compareTypeTitle(Rule::CompareType type);
        static LogicalOperator logicalOperatorFromString(const QString logicalOperator);
        static QString logicalOperatorTitle(Rule::LogicalOperator logicalOperator);
        static TransactionType previousTransactionTypeFromString(const QString &previousTransactionType);
        static QString previousTransactionTypeTitle(TransactionType previousTransactionType);
        void updateTitle();
        bool isValid(void) const;
        bool operator==(const Rule &rule) const;

        Rule *parent;
        QVector<Rule *> children;
        Indicator *indicator;
        QList<AutoOptimizerOption *> autoOptimizerOptionList;
        QString title;
        QString humanReadableTitle;
        Indicator *sourceIndicator;
        Indicator *referenceIndicator;
        PlotCurve *sourceCurve;
        PlotCurve *referenceCurve;
        float referenceValue;
        int offset;
        CompareType compareType;
        LogicalOperator logicalOperator;
        TransactionType previousTransactionType;
        mutable bool enabled;
        RuleFilter filter;
};

Q_DECLARE_TYPEINFO(Rule *, Q_MOVABLE_TYPE);
Q_DECLARE_METATYPE(Rule *)
QDataStream &operator<<(QDataStream &stream, const Rule *rule);
QDataStream &operator>>(QDataStream &stream, Rule *&rule);


#endif // RULE_H
