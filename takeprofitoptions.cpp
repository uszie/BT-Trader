#include "takeprofitoptions.h"

void TakeProfitOptions::setProperties(bool isRelative, float relativeValue, float absoluteValue)
{
    this->isRelative = isRelative;
    this->relativeValue = relativeValue;
    this->absoluteValue = absoluteValue;
}

TakeProfitOptions TakeProfitOptions::defaultProperties(void)
{
    TakeProfitOptions properties;
    properties.isRelative = false;
    properties.relativeValue = 0.0;
    properties.absoluteValue = 0.0;

    return properties;
}

QDataStream &operator<<(QDataStream &stream, const TakeProfitOptions &options)
{
    stream << options.isRelative <<  options.relativeValue << options.absoluteValue;

    return stream;
}

QDataStream &operator>>(QDataStream &stream, TakeProfitOptions &options)
{
    stream >> options.isRelative >> options.relativeValue >> options.absoluteValue;

    return stream;
}
