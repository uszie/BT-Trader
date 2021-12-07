#include "stoplossoptions.h"

void StopLossOptions::setProperties(StopLossType type, bool isRelative, float relativeValue, float absoluteValue)
{
    this->type = type;
    this->isRelative = isRelative;
    this->relativeValue = relativeValue;
    this->absoluteValue = absoluteValue;
}

StopLossOptions StopLossOptions::defaultProperties(void)
{
    StopLossOptions properties;
    properties.type = StopLossOptions::Normal;
    properties.isRelative = false;
    properties.relativeValue = 0.0;
    properties.absoluteValue = 0.0;

    return properties;
}

StopLossOptions::operator QVariant() const
{
    return QVariant::fromValue(*this);
}

bool StopLossOptions::operator ==(const StopLossOptions &other) const
{
    if (type != other.type)
        return false;

    if (isRelative != other.isRelative)
        return false;

    if (isRelative && relativeValue != other.relativeValue)
        return false;

    if (!isRelative && absoluteValue != other.absoluteValue)
        return false;

    return true;
}


QDataStream &operator<<(QDataStream &stream, const StopLossOptions &options)
{
    stream << options.type << options.isRelative << options.relativeValue << options.absoluteValue;

    return stream;
}

QDataStream &operator>>(QDataStream &stream, StopLossOptions &options)
{
    int type;
    stream >> type >> options.isRelative >> options.relativeValue >> options.absoluteValue;
    options.type = (StopLossOptions::StopLossType)type;

    return stream;
}
