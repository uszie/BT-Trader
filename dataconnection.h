#ifndef DATACONNECTION_H
#define DATACONNECTION_H

#include "bttrader.h"
#include <QtSql>
#include <QObject>
#include <QDateTime>

#define DATABASE_ADDRESS "1"
#define DATABASE_NAME "1"
#define DATABASE_USER "1"
#define DATABASE_PASSWORD "1"
#define DATABASE_CONNECTION_TIMEOUT 10 //seconds
#define CACHE_EXPIRE_TIME 600

class PriceEntry
{
    public:
        QDateTime dateTime;
        float volume;
        float openAverage;
        float openAsk;
        float openBid;
        float highAverage;
        float highAsk;
        float highBid;
        float lowAverage;
        float lowAsk;
        float lowBid;
        float closeAverage;
        float closeAsk;
        float closeBid;
};

class IsinInfo
{
    public:
        bool isValid() const
        {
            if (isin.isNull() || name.isNull() || symbol.isNull() || market.isNull() || type.isNull() || valuta.isNull())
                return false;

            return true;
        }

        QString isin;
        QString name;
        QString symbol;
        QString market;
        QString type;
        QString parent;
        QString valuta;
        bool providesBidAsk;
        int availableTimeScales;
        QTime timeStamp;
};

Q_DECLARE_METATYPE(IsinInfo)
QDataStream &operator<<(QDataStream &stream, const IsinInfo &info);
QDataStream &operator>>(QDataStream &stream, IsinInfo &info);

typedef QList<QSqlQuery> QueryList;
typedef QList<bool> ResultList;

class DataConnection : public QObject
{
        Q_OBJECT

    public:
        enum QueryType
        {
            PriceEntriesQuery,
            FirstAvailableDateTimeQuery,
            LastAvailableDateTimeQuery
        };

        explicit DataConnection(QObject *parent = 0);
        ~DataConnection(void);

        bool connectToDatabase(void);
        void disconnectFromDatabase(void);
        const QSqlDatabase &database() const;
        bool getPriceEntries(QObject *receiver, const char *slot, const IsinInfo &info, const QDateTime &startDateTime = QDateTime(), const QDateTime &endDateTime = QDateTime(), int timeScale = HourByHour, int count = 300);
        IsinInfo getIsinInfo(const QString &isin) const;
        QString timeScaleIntToString(int timeScale) const;
        int timeScaleStringToInt(const QString &timeScale) const;
        bool getFirstAvailableDateTime(QObject *receiver, const char *slot, const IsinInfo &info, int timeScale = HourByHour);
        bool getLastAvailableDateTime(QObject *receiver, const char *slot, const IsinInfo &info, int timeScale = HourByHour);
        void subscribeToNotification(const QString &name, const QObject *object);
        void unsubscribeFromNotification(const QString &name, const QObject *object);

    signals:
        void notification(const QString &name);
        void queryAvailable(const QStringList &queryStringList, DataConnection::QueryType queryType, QPointer<QObject> receiver, const char *slot);
        void subscribeRequestAvailable(const QString &name);
        void unsubscribeRequestAvailable(const QString &name);
        void priceEntriesLookupFinished(QList<PriceEntry> &list);
        void firstAvailableDateTimeLookupFinished(QDateTime &dateTime);
        void lastAvailableDateTimeLookupFinished(QDateTime &dateTime);


    private slots:
        void handlePriceEntriesQuery(ResultList resultList, QueryList queryList, QPointer<QObject> receiver, const char *slot);
        void handleFirstAvailableDateTimeQuery(ResultList resultList, QueryList queryList, QPointer<QObject> receiver, const char *slot);
        void handleLastAvailableDateTimeQuery(ResultList resultList, QueryList queryList, QPointer<QObject> receiver, const char *slot);

    private:
        class PrivateData;
        PrivateData *privateData;
};

class DataFetcher : public QObject
{
        Q_OBJECT

    public:
        DataFetcher(DataConnection *dataConnection);
        ~DataFetcher(void);

    public slots:
        void query(const QStringList &queryStringList, DataConnection::QueryType queryType, QPointer<QObject> receiver, const char *slot);
        bool connectToDatabase();
        void disconnectFromDatabase();

        void subscribeToNotification(const QString &name);
        void unsubscribeFromNotification(const QString &name);
    signals:
        void priceEntriesQueryFinished(ResultList resultList, QueryList queryList, QPointer<QObject> receiver, const char *slot);
        void firstAvailableDateTimeQueryFinished(ResultList resultList, QueryList queryList, QPointer<QObject> receiver, const char *slot);
        void lastAvailableDateTimeQueryFinished(ResultList resultList, QueryList queryList, QPointer<QObject> receiver, const char *slot);

    private:
        class PrivateData;
        PrivateData *privateData;
};

#endif // DATACONNECTION_H
