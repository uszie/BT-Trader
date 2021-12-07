#ifndef STOPLOSSOPTIONS_H
#define STOPLOSSOPTIONS_H

#include "bttrader.h"

class StopLossOptions
{
    public:
        enum StopLossType
        {
            Normal,
            Trailing
        };

        void setProperties(StopLossType type, bool isRelative, float relativeValue, float absoluteValue);
        static StopLossOptions defaultProperties(void);
        operator QVariant() const;
        bool operator ==(const StopLossOptions &other) const;

        StopLossType type;
        bool isRelative;
        float relativeValue;
        float absoluteValue;
};

Q_DECLARE_METATYPE(StopLossOptions)
QDataStream &operator<<(QDataStream &stream, const StopLossOptions &options);
QDataStream &operator>>(QDataStream &stream, StopLossOptions &options);

#endif // STOPLOSSOPTIONS_H
