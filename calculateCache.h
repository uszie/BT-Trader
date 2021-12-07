#ifndef CALCULATECACHE_H
#define CALCULATECACHE_H

#include <QObject>
#include <QVector>
#include <QPoint>
#include <QString>
#include <iostream>

class CalculateCache : public QObject
{
        Q_OBJECT

    public:
        CalculateCache(void);
        ~CalculateCache(void);

        const QVector<QPointF> &findItem(const QVector<QPointF> &source, const QString &configKey) const;
        void addItem(const QVector<QPointF> &source, const QVector<QPointF> &data, const QString &configKey);

    public slots:
        void clearCache(void);

    private:
        class PrivateData;
        PrivateData *privateData;
};
#endif // CALCULATECACHE_H
