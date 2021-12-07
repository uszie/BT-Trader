#ifndef TAKEPROFITOPTIONS_H
#define TAKEPROFITOPTIONS_H

#include "bttrader.h"

class TakeProfitOptions
{
    public:
        void setProperties(bool isRelative, float relativeValue, float absoluteValue);
        static TakeProfitOptions defaultProperties(void);
        operator QVariant() const
        {
            return QVariant::fromValue(*this);
        }

        bool isRelative;
        float relativeValue;
        float absoluteValue;
};

Q_DECLARE_METATYPE(TakeProfitOptions)
QDataStream &operator<<(QDataStream &stream, const TakeProfitOptions &options);
QDataStream &operator>>(QDataStream &stream, TakeProfitOptions &options);

#endif // TAKEPROFITOPTIONS_H
