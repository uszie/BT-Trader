#ifndef RULE_EVALUATION_H
#define RULE_EVALUATION_H

#include "bttrader.h"
#include "indicator.h"
#include "rule.h"

bool isCrossingUp(int currentBar, const float *curve, const float *reference, const RuleFilter &filter) ;
bool isCrossingUp(int currentBar, const float *curve, float reference, const RuleFilter &filter);
bool isCrossingDown(int currentBar, const float *curve, const float *reference, const RuleFilter &filter);
bool isCrossingDown(int currentBar, const float *curve, float reference, const RuleFilter &filter);
bool isAbove(int currentBar, const float *curve, const float *reference, const RuleFilter &filter);
bool isAbove(int currentBar, const float *curve, float reference, const RuleFilter &filter);
bool isUnder(int currentBar, const float *curve, const float *reference, const RuleFilter &filter);
bool isUnder(int currentBar, const float *curve, float reference, const RuleFilter &filter);
bool isRising(int currentBar, const float *curve, const RuleFilter &filter);
bool isFalling(int currentBar, const float *curve, const RuleFilter &filter);

#endif // RULE_EVALUATION_H
