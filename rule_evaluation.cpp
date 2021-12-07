#include "rule_evaluation.h"

bool isCrossingUp(int currentBar, const float *curve, const float *reference, const RuleFilter &filter)
{
    int previousBar;

    if (filter.type == RuleFilter::ClosesInRow)
    {
        int offsetBar = currentBar - filter.periods;
        if (offsetBar < 0)
            return false;

        if (curve[ offsetBar ] == IGNORE_VALUE || reference[ offsetBar ] == IGNORE_VALUE)
            return false;

        if (curve[ offsetBar ] > reference[ offsetBar ])
            return false;
    }
    else
    {
        previousBar = currentBar - 1;
        if (previousBar < 0)
            return false;

        if (curve[ previousBar ] == IGNORE_VALUE || reference[ previousBar ] == IGNORE_VALUE)
            return false;

        if (curve[ previousBar ] > reference[ previousBar ])
            return false;
    }

    if (filter.type == RuleFilter::NoFilter)
    {
        if (curve[currentBar] == IGNORE_VALUE || reference[currentBar] == IGNORE_VALUE)
            return false;

        if (curve[ currentBar ] <= reference[ currentBar ])
            return false;
    }
    else if (filter.type == RuleFilter::ClosesInRow)
    {
        for (int i = 0; i < filter.periods; ++i)
        {
            if (curve[currentBar - i] == IGNORE_VALUE || reference[currentBar - i] == IGNORE_VALUE)
                return false;

            if (curve[ currentBar - i ] <= reference[ currentBar - i ])
                return false;
        }
    }
    else if (filter.type == RuleFilter::RelativeDifference)
    {
        if (curve[currentBar] == IGNORE_VALUE || reference[currentBar] == IGNORE_VALUE)
            return false;

        float factor = reference[ currentBar ] * filter.relativeDifference;
        if (curve[ currentBar ] <= (reference[ currentBar ] + factor))
            return false;
    }
    else if (filter.type == RuleFilter::AbsoluteDifference)
    {
        if (curve[ currentBar ] <= (reference[ currentBar ] + filter.absoluteDifference))
            return false;
    }

    return true;
}

bool isCrossingUp(int currentBar, const float *curve, float reference, const RuleFilter &filter)
{
    int previousBar;

    if (filter.type == RuleFilter::ClosesInRow)
    {
        int offsetBar = currentBar - filter.periods;
        if (offsetBar < 0)
            return false;

        if (curve[ offsetBar ] == IGNORE_VALUE)
            return false;

        if (curve[ offsetBar ] > reference)
            return false;
    }
    else
    {
        previousBar = currentBar - 1;
        if (previousBar < 0)
            return false;

        if (curve[ previousBar ] == IGNORE_VALUE)
            return false;

        if (curve[ previousBar ] > reference)
            return false;
    }

    if (filter.type == RuleFilter::NoFilter)
    {
        if (curve[currentBar] == IGNORE_VALUE || reference == IGNORE_VALUE)
            return false;

        if (curve[ currentBar ] <= reference)
            return false;
    }
    else if (filter.type == RuleFilter::ClosesInRow)
    {
        for (int i = 0; i < filter.periods; ++i)
        {
            if (curve[currentBar - i] == IGNORE_VALUE || reference == IGNORE_VALUE)
                return false;

            if (curve[ currentBar - i ] <= reference)
                return false;
        }
    }
    else if (filter.type == RuleFilter::RelativeDifference)
    {
        float factor = reference * filter.relativeDifference;
        if (curve[currentBar] == IGNORE_VALUE || reference == IGNORE_VALUE)
            return false;

        if (curve[ currentBar ] <= (reference + factor))
            return false;
    }
    else if (filter.type == RuleFilter::AbsoluteDifference)
    {
        if (curve[currentBar] == IGNORE_VALUE || reference == IGNORE_VALUE)
            return false;

        if (curve[ currentBar ] <= (reference + filter.absoluteDifference))
            return false;
    }

    return true;
}

bool isCrossingDown(int currentBar, const float *curve, const float *reference, const RuleFilter &filter)
{
    int previousBar;

    if (filter.type == RuleFilter::ClosesInRow)
    {
        int offsetBar = currentBar - filter.periods;
        if (offsetBar < 0)
            return false;

        if (curve[ offsetBar ] == IGNORE_VALUE || reference[ offsetBar ] == IGNORE_VALUE)
            return false;

        if (curve[ offsetBar ] < reference[ offsetBar ])
            return false;
    }
    else
    {
        previousBar = currentBar - 1;
        if (previousBar < 0)
            return false;

        if (curve[ previousBar ] == IGNORE_VALUE || reference[ previousBar ] == IGNORE_VALUE)
            return false;

        if (curve[ previousBar ] < reference[ previousBar ])
            return false;
    }

    if (filter.type == RuleFilter::NoFilter)
    {
        if (curve[currentBar] == IGNORE_VALUE || reference[currentBar] == IGNORE_VALUE)
            return false;

        if (curve[ currentBar ] >= reference[ currentBar ])
            return false;
    }
    else if (filter.type == RuleFilter::ClosesInRow)
    {
        for (int i = 0; i < filter.periods; ++i)
        {
            if (curve[currentBar - i] == IGNORE_VALUE || reference[currentBar - i] == IGNORE_VALUE)
                return false;

            if (curve[ currentBar - i ] >= reference[ currentBar - i ])
                return false;
        }
    }
    else if (filter.type == RuleFilter::RelativeDifference)
    {
        if (curve[currentBar] == IGNORE_VALUE || reference[currentBar] == IGNORE_VALUE)
            return false;

        float factor = reference[ currentBar ] * filter.relativeDifference;
        if (curve[ currentBar ] >= (reference[ currentBar ] - factor))
            return false;
    }
    else if (filter.type == RuleFilter::AbsoluteDifference)
    {
        if (curve[currentBar] == IGNORE_VALUE || reference[currentBar] == IGNORE_VALUE)
            return false;

        if (curve[ currentBar ] >= (reference[ currentBar ] - filter.absoluteDifference))
            return false;
    }

    return true;
}

bool isCrossingDown(int currentBar, const float *curve, float reference, const RuleFilter &filter)
{
    int previousBar;

    if (filter.type == RuleFilter::ClosesInRow)
    {
        int offsetBar = currentBar - filter.periods;
        if (offsetBar < 0)
            return false;

        if (curve[ offsetBar ] == IGNORE_VALUE)
            return false;

        if (curve[ offsetBar ] < reference)
            return false;
    }
    else
    {
        previousBar = currentBar - 1;
        if (previousBar < 0)
            return false;

        if (curve[ previousBar ] == IGNORE_VALUE)
            return false;

        if (curve[ previousBar ] < reference)
            return false;
    }

    if (filter.type == RuleFilter::NoFilter)
    {
        if (curve[currentBar] == IGNORE_VALUE || reference == IGNORE_VALUE)
            return false;

        if (curve[ currentBar ] >= reference)
            return false;
    }
    else if (filter.type == RuleFilter::ClosesInRow)
    {
        for (int i = 0; i < filter.periods; ++i)
        {
            if (curve[currentBar - i] == IGNORE_VALUE || reference == IGNORE_VALUE)
                return false;

            if (curve[ currentBar - i ] >= reference)
                return false;
        }
    }
    else if (filter.type == RuleFilter::RelativeDifference)
    {
        if (curve[currentBar] == IGNORE_VALUE || reference == IGNORE_VALUE)
            return false;

        float factor = reference * filter.relativeDifference;
        if (curve[ currentBar ] >= (reference - factor))
            return false;
    }
    else if (filter.type == RuleFilter::AbsoluteDifference)
    {
        if (curve[currentBar] == IGNORE_VALUE || reference == IGNORE_VALUE)
            return false;

        if (curve[ currentBar ] >= (reference - filter.absoluteDifference))
            return false;
    }

    return true;
}

bool isAbove(int currentBar, const float *curve, const float *reference, const RuleFilter &filter)
{
    if (filter.type == RuleFilter::ClosesInRow)
    {
        int offsetBar = currentBar - filter.periods;
        if (offsetBar < 0)
            return false;

        if (curve[ offsetBar ] == IGNORE_VALUE || reference[ offsetBar ] == IGNORE_VALUE)
            return false;
    }
    else
    {
        if (curve[ currentBar ] == IGNORE_VALUE || reference[ currentBar ] == IGNORE_VALUE)
            return false;
    }

    if (filter.type == RuleFilter::NoFilter)
    {
        if (curve[currentBar] == IGNORE_VALUE || reference[currentBar] == IGNORE_VALUE)
            return false;

        if (curve[ currentBar ] <= reference[ currentBar ])
            return false;
    }
    else if (filter.type == RuleFilter::ClosesInRow)
    {
        for (int i = 0; i < filter.periods; ++i)
        {
            if (curve[currentBar - i] == IGNORE_VALUE || reference[currentBar - i] == IGNORE_VALUE)
                return false;

            if (curve[ currentBar - i ] <= reference[ currentBar - i ])
                return false;
        }
    }
    else if (filter.type == RuleFilter::RelativeDifference)
    {
        if (curve[currentBar] == IGNORE_VALUE || reference[currentBar] == IGNORE_VALUE)
            return false;

        float factor = reference[ currentBar ] * filter.relativeDifference;
        if (curve[ currentBar ] <= (reference[ currentBar ] + factor))
            return false;
    }
    else if (filter.type == RuleFilter::AbsoluteDifference)
    {
        if (curve[currentBar] == IGNORE_VALUE || reference[currentBar] == IGNORE_VALUE)
            return false;

        if (curve[ currentBar ] <= (reference[ currentBar ] + filter.absoluteDifference))
            return false;
    }

    return true;
}

bool isAbove(int currentBar, const float *curve, const float reference, const RuleFilter &filter)
{
    if (filter.type == RuleFilter::ClosesInRow)
    {
        int offsetBar = currentBar - filter.periods;
        if (offsetBar < 0)
            return false;

        if (curve[ offsetBar ] == IGNORE_VALUE)
            return false;
    }
    else
    {
        if (curve[ currentBar ] == IGNORE_VALUE)
            return false;
    }

    if (filter.type == RuleFilter::NoFilter)
    {
        if (curve[currentBar] == IGNORE_VALUE || reference == IGNORE_VALUE)
            return false;

        if (curve[ currentBar ] <= reference)
            return false;
    }
    else if (filter.type == RuleFilter::ClosesInRow)
    {
        for (int i = 0; i < filter.periods; ++i)
        {
            if (curve[currentBar - i] == IGNORE_VALUE || reference == IGNORE_VALUE)
                return false;

            if (curve[ currentBar - i ] <= reference)
                return false;
        }
    }
    else if (filter.type == RuleFilter::RelativeDifference)
    {
        if (curve[currentBar] == IGNORE_VALUE || reference == IGNORE_VALUE)
            return false;

        float factor = reference * filter.relativeDifference;
        if (curve[ currentBar ] <= (reference + factor))
            return false;
    }
    else if (filter.type == RuleFilter::AbsoluteDifference)
    {
        if (curve[currentBar] == IGNORE_VALUE || reference == IGNORE_VALUE)
            return false;

        if (curve[ currentBar ] <= (reference + filter.absoluteDifference))
            return false;
    }

    return true;
}

bool isUnder(int currentBar, const float *curve, const float *reference, const RuleFilter &filter)
{
    if (filter.type == RuleFilter::ClosesInRow)
    {
        int offsetBar = currentBar - filter.periods;
        if (offsetBar < 0)
            return false;

        if (curve[ offsetBar ] == IGNORE_VALUE || reference[ offsetBar ] == IGNORE_VALUE)
            return false;
    }
    else
    {
        if (curve[ currentBar ] == IGNORE_VALUE || reference[ currentBar ] == IGNORE_VALUE)
            return false;
    }

    if (filter.type == RuleFilter::NoFilter)
    {
        if (curve[currentBar] == IGNORE_VALUE || reference[currentBar] == IGNORE_VALUE)
            return false;

        if (curve[ currentBar ] >= reference[ currentBar ])
            return false;
    }
    else if (filter.type == RuleFilter::ClosesInRow)
    {
        for (int i = 0; i < filter.periods; ++i)
        {
            if (curve[currentBar - i] == IGNORE_VALUE || reference[currentBar - i] == IGNORE_VALUE)
                return false;

            if (curve[ currentBar - i ] >= reference[ currentBar - i ])
                return false;
        }
    }
    else if (filter.type == RuleFilter::RelativeDifference)
    {
        if (curve[currentBar] == IGNORE_VALUE || reference[currentBar] == IGNORE_VALUE)
            return false;

        float factor = reference[ currentBar ] * filter.relativeDifference;
        if (curve[ currentBar ] >= (reference[ currentBar ] + factor))
            return false;
    }
    else if (filter.type == RuleFilter::AbsoluteDifference)
    {
        if (curve[currentBar] == IGNORE_VALUE || reference[currentBar] == IGNORE_VALUE)
            return false;

        if (curve[ currentBar ] >= (reference[ currentBar ] + filter.absoluteDifference))
            return false;
    }

    return true;
}

bool isUnder(int currentBar, const float *curve, const float reference, const RuleFilter &filter)
{
    if (filter.type == RuleFilter::ClosesInRow)
    {
        int offsetBar = currentBar - filter.periods;
        if (offsetBar < 0)
            return false;

        if (curve[ offsetBar ] == IGNORE_VALUE)
            return false;
    }
    else
    {
        if (curve[ currentBar ] == IGNORE_VALUE)
            return false;
    }

    if (filter.type == RuleFilter::NoFilter)
    {
        if (curve[currentBar] == IGNORE_VALUE || reference == IGNORE_VALUE)
            return false;

        if (curve[ currentBar ] >= reference)
            return false;
    }
    else if (filter.type == RuleFilter::ClosesInRow)
    {
        for (int i = 0; i < filter.periods; ++i)
        {
            if (curve[currentBar - i] == IGNORE_VALUE || reference == IGNORE_VALUE)
                return false;

            if (curve[ currentBar - i ] >= reference)
                return false;
        }
    }
    else if (filter.type == RuleFilter::RelativeDifference)
    {
        if (curve[currentBar] == IGNORE_VALUE || reference == IGNORE_VALUE)
            return false;

        float factor = reference * filter.relativeDifference;
        if (curve[ currentBar ] >= (reference + factor))
            return false;
    }
    else if (filter.type == RuleFilter::AbsoluteDifference)
    {
        if (curve[currentBar] == IGNORE_VALUE || reference == IGNORE_VALUE)
            return false;

        if (curve[ currentBar ] >= (reference + filter.absoluteDifference))
            return false;
    }

    return true;
}

bool isRising(int currentBar, const float *curve, const RuleFilter &filter)
{
    int previousBar;

    if (filter.type == RuleFilter::ClosesInRow)
    {
        int offsetBar = currentBar - filter.periods;
        if (offsetBar < 0)
            return false;

        if (curve[ offsetBar ] == IGNORE_VALUE)
            return false;
    }
    else
    {
        previousBar = currentBar - 1;
        if (previousBar < 0)
            return false;

        if (curve[ previousBar ] == IGNORE_VALUE)
            return false;
    }

    if (filter.type == RuleFilter::NoFilter)
    {
        if (curve[previousBar] == IGNORE_VALUE || curve[currentBar] == IGNORE_VALUE)
            return false;

        if (curve[ previousBar ] >= curve[ currentBar ])
            return false;
    }
    else if (filter.type == RuleFilter::ClosesInRow)
    {
        for (int i = 0; i < filter.periods; ++i)
        {
            if (curve[currentBar - (i + 1)] == IGNORE_VALUE || curve[currentBar] == IGNORE_VALUE)
                return false;

            if (curve[ currentBar - (i + 1) ] >= curve[ currentBar ])
                return false;
        }
    }
    else if (filter.type == RuleFilter::RelativeDifference)
    {
        if (curve[previousBar] == IGNORE_VALUE || curve[currentBar] == IGNORE_VALUE)
            return false;

        float factor = curve[ currentBar ] * filter.relativeDifference;
        if (curve[ previousBar ] >= (curve[ currentBar ] - factor))
            return false;
    }
    else if (filter.type == RuleFilter::AbsoluteDifference)
    {
        if (curve[previousBar] == IGNORE_VALUE || curve[currentBar] == IGNORE_VALUE)
            return false;

        if (curve[ previousBar ] >= (curve[ currentBar ] - filter.absoluteDifference))
            return false;
    }

    return true;
}

bool isFalling(int currentBar, const float *curve, const RuleFilter &filter)
{
    int previousBar;

    if (filter.type == RuleFilter::ClosesInRow)
    {
        int offsetBar = currentBar - filter.periods;
        if (offsetBar < 0)
            return false;

        if (curve[ offsetBar ] == IGNORE_VALUE)
            return false;
    }
    else
    {
        previousBar = currentBar - 1;
        if (previousBar < 0)
            return false;

        if (curve[ previousBar ] == IGNORE_VALUE)
            return false;
    }

    if (filter.type == RuleFilter::NoFilter)
    {
        if (curve[previousBar] == IGNORE_VALUE || curve[currentBar] == IGNORE_VALUE)
            return false;

        if (curve[ previousBar ] <= curve[ currentBar ])
            return false;
    }
    else if (filter.type == RuleFilter::ClosesInRow)
    {
        for (int i = 0; i < filter.periods; ++i)
        {
            if (curve[currentBar - (i + 1)] == IGNORE_VALUE || curve[currentBar] == IGNORE_VALUE)
                return false;

            if (curve[ currentBar - (i + 1) ] <= curve[ currentBar ])
                return false;
        }
    }
    else if (filter.type == RuleFilter::RelativeDifference)
    {
        if (curve[previousBar] == IGNORE_VALUE || curve[currentBar] == IGNORE_VALUE)
            return false;

        float factor = curve[ currentBar ] * filter.relativeDifference;
        if (curve[ previousBar ] <= (curve[ currentBar ] - factor))
            return false;
    }
    else if (filter.type == RuleFilter::AbsoluteDifference)
    {
        if (curve[previousBar] == IGNORE_VALUE || curve[currentBar] == IGNORE_VALUE)
            return false;

        if (curve[ previousBar ] <= (curve[ currentBar ] - filter.absoluteDifference))
            return false;
    }

    return true;
}
