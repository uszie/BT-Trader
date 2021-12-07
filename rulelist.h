#ifndef RULELIST_H
#define RULELIST_H

#include "bttrader.h"
#include <QString>

class PriceChart;
class Indicator;
class Rule;
class PlotCurve;

class SignalCacheOject
{
    public:
        SignalCacheOject(int size, bool useHighLowCache = false, SignalCacheOject *oldCache = NULL)
        {
            this->size = size;

            sCache = NULL;
            rCache = NULL;
            hCache = NULL;
            lCache = NULL;
            if (size == 0)
                return;

            sCache = new int *[3];
            sCache[0] = (int *) _mm_malloc(sizeof(int) * size, 16);
            sCache[1] = (int *) _mm_malloc(sizeof(int) * size, 16);
            sCache[2] = (int *) _mm_malloc(sizeof(int) * size, 16);

            if (oldCache)
            {
                memcpy(sCache[0], oldCache->signalCache()[0], size * sizeof(int));
                memcpy(sCache[1], oldCache->signalCache()[1], size * sizeof(int));
                memcpy(sCache[2], oldCache->signalCache()[2], size * sizeof(int));
            }
            else
            {
                memset(sCache[0], 0, size * sizeof(int));
                memset(sCache[1], 0, size * sizeof(int));
                memset(sCache[2], 0, size * sizeof(int));
            }

            rCache = new int *[3];
            rCache[0] = (int *) _mm_malloc(sizeof(int) * size, 16);
            rCache[1] = (int *) _mm_malloc(sizeof(int) * size, 16);
            rCache[2] = (int *) _mm_malloc(sizeof(int) * size, 16);

            if (oldCache)
            {
                memcpy(rCache[0], oldCache->resultsCache()[0], size * sizeof(int));
                memcpy(rCache[1], oldCache->resultsCache()[1], size * sizeof(int));
                memcpy(rCache[2], oldCache->resultsCache()[2], size * sizeof(int));
                for (int i = 0; i < size; i++)
                {
                    for (int j = 0; j < 3; j++)
                    {
                        if (sCache[j][i] != 0)
                            rCache[j][sCache[j][i]] = -1;
                    }
                }
            }
            else
            {
                memset(rCache[0], -1, size * sizeof(int));
                memset(rCache[1], -1, size * sizeof(int));
                memset(rCache[2], -1, size * sizeof(int));
            }

            if (useHighLowCache)
            {
                hCache = (float *) _mm_malloc(sizeof(float) * size, 16);
                if (oldCache)
                    memcpy(hCache, oldCache->highCache(), size * sizeof(float));
                else
                {
                    for (int i = 0; i < size; i++)
                        hCache[i] = -FLT_MAX;
                    //                    memset(hCache, (int) - FLT_MAX, size * sizeof(float));
                }

                lCache = (float *) _mm_malloc(sizeof(float) * size, 16);
                if (oldCache)
                    memcpy(lCache, oldCache->lowCache(), size * sizeof(float));
                else
                {
                    for (int i = 0; i < size; i++)
                        lCache[i] = FLT_MAX;
                    //                    memset(lCache, (int)FLT_MAX, size * sizeof(float));
                }
            }
        }

        ~SignalCacheOject(void)
        {
            for (int i = 0; i < 3; i++)
            {
                _mm_free(sCache[i]);
                _mm_free(rCache[i]);
            }

            delete [] sCache;
            delete [] rCache;

            if (hCache)
                _mm_free(hCache);

            if (lCache)
                _mm_free(lCache);
        }

        inline int **signalCache(void)
        {
            return sCache;
        }

        inline int **resultsCache(void)
        {
            return rCache;
        }

        inline float *highCache(void)
        {
            return hCache;
        }

        inline float *lowCache(void)
        {
            return lCache;
        }

    private:
        int size;
        int **sCache;
        int **rCache;
        float *hCache;
        float *lCache;
};

struct RuleListResult
{
    bool result;
    bool onlyContainsDisabledRules;
    bool positiveAND;
    bool positiveOR;
};

struct SIMDRuleListResult
{
    ui4vector result;
    bool onlyContainsDisabledRules;
    bool positiveAND;
    bool positiveOR;
};

class RuleList
{
    public:
        explicit RuleList(PriceChart *priceChart, Indicator *parent, TransactionType type);
        virtual ~RuleList(void);

        void read(const QString &configKey);
        void write(const QString &configKey);
        Rule *addRule(const QString &ruleString, Rule *parent = NULL);
        void deleteRule(Rule *rule);
        void clear(void);
        const QVector<Rule *> &items(void) const;
        RuleListResult resolveRuleList(int currentBar, TransactionType transactionType, int transactionStart) const;
        void createRuleResolvementCache();
        void clearRuleResolvementCache();
        void updateSignalCacheForTransaction(int currentBar, TransactionType transactionType, int transactionStart);
        SignalCacheOject *currentCacheObject(void) const;

    private:
        void initRuleList(const QVector<Rule *> *ruleList, Rule *parent = NULL);
        bool resolveRule(int currentBar, const Rule *rule, TransactionType transactionType) const;
        Rule *addRule(Rule *rule, Rule *parent = NULL);
        void disconnectRuleList(const QVector<Rule *> *ruleList = NULL) const;
        void disconnectRuleIndicators(Rule *rule) const;
        PlotCurve *findCurve(const QString &title, const Indicator *indicator) const;
        Indicator *findIndicator(const QString &title) const;
        QString writeRulesToString(QVector<Rule *> *ruleList);
        QByteArray createCacheKey(QVector<Rule *> *ruleList);
        RuleListResult doResolveRuleList(int currentBar, TransactionType transactionType, const QVector<Rule *> *ruleList) const;

        class PrivateData;
        PrivateData *privateData;
};

#endif // RULELIST_H
