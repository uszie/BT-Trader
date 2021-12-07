#include "bttrader.h"
#include "calculateCache.h"

class CalculateCache::PrivateData
{
    public:
        QVector<QString> configKeyList;
        QVector<QVector<QPointF> > dataList;
        QVector<QVector<QPointF> > referenceList;
        QVector<QPointF> emptyValue;
        QMutex lock;
};

CalculateCache::CalculateCache(void)
{
    privateData = new PrivateData;
    privateData->configKeyList.clear();
    privateData->dataList.clear();
    privateData->referenceList.clear();
}

CalculateCache::~CalculateCache(void)
{
    clearCache();

    delete privateData;
}

const QVector<QPointF> &CalculateCache::findItem(const QVector<QPointF> &source, const QString &configKey) const
{
    if (USE_CALCULATION_CACHE <= 0)
        return privateData->emptyValue;

    privateData->lock.lock();
    QVector<QString>::const_iterator it = privateData->configKeyList.constBegin();
    QVector<QVector<QPointF> >::const_iterator dataIt = privateData->dataList.constBegin();
    QVector<QVector<QPointF> >::const_iterator referenceIt = privateData->referenceList.constBegin();
    const QVector<QString>::const_iterator end = privateData->configKeyList.constEnd();
    for (; it != end; it++, dataIt++, referenceIt++)
    {
        const QString &key = *it;
        const QVector<QPointF> &reference = *referenceIt;
        if (reference == source && key == configKey)
        {
            privateData->lock.unlock();
            return *dataIt;
        }
    }

    privateData->lock.unlock();
    return privateData->emptyValue;
}

void CalculateCache::addItem(const QVector<QPointF> &source, const QVector<QPointF> &data, const QString &configKey)
{
    if (USE_CALCULATION_CACHE <= 0)
        return;

    privateData->lock.lock();
    QVector<QString>::const_iterator keyIt = privateData->configKeyList.constBegin();
    const QVector<QString>::const_iterator keyEnd = privateData->configKeyList.constEnd();
    QVector<QVector<QPointF> >::const_iterator referenceIt = privateData->referenceList.constBegin();
    for (; keyIt != keyEnd; keyIt++, referenceIt++)
    {
        const QString &key = *keyIt;
        const QVector<QPointF> &reference = *referenceIt;
        if (reference == source && key == configKey)
        {
            privateData->lock.unlock();
            return;
        }
    }

    privateData->configKeyList.append(configKey);
    privateData->dataList.append(data);
    privateData->referenceList.append(source);
    privateData->lock.unlock();
}

void CalculateCache::clearCache(void)
{
    privateData->lock.lock();
    privateData->configKeyList.clear();
    for (int i = 0; i < privateData->dataList.size(); i++)
    {
        QVector<QPointF> data = privateData->dataList.at(i);
        data.clear();
    }

    privateData->dataList.clear();
    privateData->referenceList.clear();
    privateData->lock.unlock();
}
