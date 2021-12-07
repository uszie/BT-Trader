#include "dataconnection.h"
#include <QMutex>
#include <iostream>

#define GET_INFO_TEMPLATE "SELECT * FROM \"INFO\" WHERE isin = '$ISIN';"
#define GET_PRICE_ENTRIES_CNT_TEMPLATE "SELECT * FROM \"$OBJECT\" ORDER BY datetime DESC LIMIT $CNT;"
#define GET_PRICE_ENTRIES_BTW_TEMPLATE "SELECT * FROM \"$OBJECT\" WHERE datetime>='$START' AND datetime<='$END' ORDER BY datetime DESC;"
#define GET_PRICE_ENTRIES_AFT_TEMPLATE "SELECT * FROM \"$OBJECT\" WHERE datetime>='$START' ORDER BY datetime DESC;"
#define GET_PRICE_ENTRIES_BFR_CNT_TEMPLATE "SELECT * FROM \"$OBJECT\" WHERE datetime<='$END' ORDER BY datetime DESC LIMIT $CNT;"
#define GET_FIRST_ENTRY_TEMPLATE "SELECT * FROM \"$OBJECT\" WHERE datetime>='1800-01-01' ORDER BY datetime ASC LIMIT 1;"
#define GET_LAST_ENTRY_TEMPLATE "SELECT * FROM \"$OBJECT\" WHERE datetime<='2200-01-01' ORDER BY datetime DESC LIMIT 1;"

QDataStream &operator<<(QDataStream &stream, const IsinInfo &info)
{
    stream << info.isin << info.name << info.symbol << info.market << info.type << info.parent << info.valuta << info.providesBidAsk << info.availableTimeScales << info.timeStamp;

    return stream;
}


QDataStream &operator>>(QDataStream &stream, IsinInfo &info)
{
    stream >> info.isin >> info.name >> info.symbol >> info.market >> info.type >> info.parent >> info.valuta >> info.providesBidAsk >> info.availableTimeScales >> info.timeStamp;

    return stream;
}

class DataConnection::PrivateData
{
    public:
        QSqlDatabase database;
        QCache<QString, IsinInfo> isinCache;
        QList<QPair<QString, QList<const QObject *>  > > subscriberList;
        QThread *dataFetcherThread;
        DataFetcher *dataFetcherWorker;
};

DataConnection::DataConnection(QObject *parent) :
    QObject(parent)
{
    qRegisterMetaType<ResultList>("ResultList");
    qRegisterMetaType<QueryList>("QueryList");
    qRegisterMetaType<DataConnection::QueryType>("DataConnection::QueryType");
    qRegisterMetaType<QPointer<QObject> >("QPointer<QObject>");

    privateData = new PrivateData;
    privateData->dataFetcherThread = new QThread;
    privateData->dataFetcherWorker = new DataFetcher(this);
    privateData->dataFetcherWorker->moveToThread(privateData->dataFetcherThread);
    connect(privateData->dataFetcherThread, SIGNAL(started()), privateData->dataFetcherWorker, SLOT(connectToDatabase()));
    connect(privateData->dataFetcherThread, SIGNAL(finished()), privateData->dataFetcherWorker, SLOT(disconnectFromDatabase()));
    connect(privateData->dataFetcherWorker, SIGNAL(priceEntriesQueryFinished(ResultList, QueryList, QPointer<QObject>, const char *)), this, SLOT(handlePriceEntriesQuery(ResultList, QueryList, QPointer<QObject>, const char *)));
    connect(privateData->dataFetcherWorker, SIGNAL(firstAvailableDateTimeQueryFinished(ResultList, QueryList, QPointer<QObject>, const char *)), this, SLOT(handleFirstAvailableDateTimeQuery(ResultList, QueryList, QPointer<QObject>, const char *)));
    connect(privateData->dataFetcherWorker, SIGNAL(lastAvailableDateTimeQueryFinished(ResultList, QueryList, QPointer<QObject>, const char *)), this, SLOT(handleLastAvailableDateTimeQuery(ResultList, QueryList, QPointer<QObject>, const char *)));
    connect(this, SIGNAL(queryAvailable(QStringList, DataConnection::QueryType, QPointer<QObject>, const char *)), privateData->dataFetcherWorker, SLOT(query(QStringList, DataConnection::QueryType, QPointer<QObject>, const char *)));
    connect(this, SIGNAL(subscribeRequestAvailable(QString)), privateData->dataFetcherWorker, SLOT(subscribeToNotification(QString)));
    connect(this, SIGNAL(unsubscribeRequestAvailable(QString)), privateData->dataFetcherWorker, SLOT(unsubscribeFromNotification(QString)));
    privateData->dataFetcherThread->start();
}

DataConnection::~DataConnection(void)
{
    privateData->dataFetcherThread->quit();
    privateData->dataFetcherThread->wait();
    delete privateData->dataFetcherWorker;
    delete privateData->dataFetcherThread;
    delete privateData;
}

bool DataConnection::connectToDatabase(void)
{
    privateData->database = QSqlDatabase::addDatabase("QPSQL");
    privateData->database.setConnectOptions(QString("connect_timeout=") + QString::number(DATABASE_CONNECTION_TIMEOUT));
    privateData->database.setHostName(DATABASE_ADDRESS);
    privateData->database.setDatabaseName(DATABASE_NAME);
    privateData->database.setUserName(DATABASE_USER);
    privateData->database.setPassword(DATABASE_PASSWORD);

    bool ok = privateData->database.open();
    if (!ok)
    {
        qWarning("DataConnection::connect(): Failed to connect to database: %s", privateData->database.lastError().text().toAscii().data());
        return false;
    }

    return true;
}

void DataConnection::disconnectFromDatabase(void)
{
    privateData->database.close();
}

bool DataConnection::getPriceEntries(QObject *receiver, const char *slot, const IsinInfo &info, const QDateTime &startDateTime, const QDateTime &endDateTime, int timeScale, int count)
{
    if (!info.isValid())
    {
        qWarning("DataConnection::getPriceEntries(): invalid IsinInfo");
        return false;
    }

    QString queryString;
    if (!startDateTime.isValid() && !endDateTime.isValid())
    {
        queryString = GET_PRICE_ENTRIES_CNT_TEMPLATE;
        queryString.replace("$CNT", QString::number(count));
    }
    else if (!startDateTime.isValid() && endDateTime.isValid())
    {
        queryString = GET_PRICE_ENTRIES_BFR_CNT_TEMPLATE;
        queryString.replace("$END", endDateTime.toString("yyyy-MM-dd hh:mm:ss"));
        queryString.replace("$CNT", QString::number(count));
    }
    else if (startDateTime.isValid() && !endDateTime.isValid())
    {
        queryString = GET_PRICE_ENTRIES_AFT_TEMPLATE;
        queryString.replace("$START", startDateTime.toString("yyyy-MM-dd hh:mm:ss"));
    }
    else if (startDateTime.isValid() && endDateTime.isValid())
    {
        queryString = GET_PRICE_ENTRIES_BTW_TEMPLATE;
        queryString.replace("$START", startDateTime.toString("yyyy-MM-dd hh:mm:ss"));
        queryString.replace("$END", endDateTime.toString("yyyy-MM-dd hh:mm:ss"));
    }
    else
    {
        qWarning("DataConnection::getPriceEntrie(): invalid arguments");
        return false;
    }

    QString oName;
    if (info.providesBidAsk)
        oName = QString(info.isin + "_ASK_" + timeScaleIntToString(timeScale));
    else
        oName = QString(info.isin + "_" + timeScaleIntToString(timeScale));

    queryString.replace("$OBJECT", oName);
    QStringList queryStringList;
    queryStringList << queryString;

    if (info.providesBidAsk)
    {
        QString oldoName = oName;
        oName = QString(info.isin + "_BID_" + timeScaleIntToString(timeScale));
        queryString.replace(oldoName, oName);
        queryStringList << queryString;
    }

    QPointer<QObject> receiverPointer = receiver;
    emit queryAvailable(queryStringList, DataConnection::PriceEntriesQuery, receiverPointer, slot);

    return true;
}

void DataConnection::handlePriceEntriesQuery(ResultList resultList, QueryList queryList, QPointer<QObject> receiver, const char *slot)
{
    if (!receiver)
    {
        qDebug() << "dataConnection::handlePriceEntriesQuery(): receiver object is not valid anymore";
        return;
    }

    connect(this, SIGNAL(priceEntriesLookupFinished(QList<PriceEntry> &)), receiver, slot, Qt::DirectConnection);
    QList<PriceEntry> emptyList;

    if (queryList.size() < 1 || resultList.size() < 1)
    {
        qWarning("DataConnection::handlePriceEntriesQuery(): Failed to query database: received a empty query list");
        emit priceEntriesLookupFinished(emptyList);
        disconnect(this, SIGNAL(priceEntriesLookupFinished(QList<PriceEntry> &)), receiver, slot);
        return;
    }

    QSqlQuery askQuery = queryList.at(0);
    if (!resultList.at(0))
    {
        qWarning("DataConnection::handlePriceEntriesQuery(): Failed to query database: %s", askQuery.lastError().text().toAscii().data());
        emit priceEntriesLookupFinished(emptyList);
        disconnect(this, SIGNAL(priceEntriesLookupFinished(QList<PriceEntry> &)), receiver, slot);
        return;
    }

    if (askQuery.size() < 1)
    {
        qWarning("DataConnection::handlePriceEntriesQuery(): Empty query");
        emit priceEntriesLookupFinished(emptyList);
        disconnect(this, SIGNAL(priceEntriesLookupFinished(QList<PriceEntry> &)), receiver, slot);
        return;
    }

    QList<PriceEntry> priceList;
    askQuery.last();
    do
    {
        PriceEntry entry;
        entry.dateTime = askQuery.value(0).toDateTime();
        entry.volume = askQuery.value(5).toFloat();
        entry.openAsk = askQuery.value(1).toFloat();
        entry.openBid = askQuery.value(1).toFloat();
        entry.openAverage = (entry.openAsk + entry.openBid) / 2.0f;
        entry.highAsk = askQuery.value(2).toFloat();
        entry.highBid = askQuery.value(2).toFloat();
        entry.highAverage = (entry.highAsk + entry.highBid) / 2.0f;
        entry.lowAsk = askQuery.value(3).toFloat();
        entry.lowBid = askQuery.value(3).toFloat();
        entry.lowAverage = (entry.lowAsk + entry.lowBid) / 2.0f;
        entry.closeAsk = askQuery.value(4).toFloat();
        entry.closeBid = askQuery.value(4).toFloat();
        entry.closeAverage = (entry.closeAsk + entry.closeBid) / 2.0f;
        priceList.append(entry);
    }
    while (askQuery.previous());

    if (queryList.size() == 2)
    {
        QSqlQuery bidQuery = queryList.at(1);
        if (!resultList.at(1))
        {
            qWarning("DataConnection::handlePriceEntriesQuery(): Failed to query database: %s", bidQuery.lastError().text().toAscii().data());
            emit priceEntriesLookupFinished(emptyList);
            disconnect(this, SIGNAL(priceEntriesLookupFinished(QList<PriceEntry> &)), receiver, slot);
            return;
        }

        if (askQuery.size() != bidQuery.size())
        {
            qWarning("DataConnection::handlePriceEntriesQuery(): Corrupt data ask and bid count are not the same");
            emit priceEntriesLookupFinished(emptyList);
            disconnect(this, SIGNAL(priceEntriesLookupFinished(QList<PriceEntry> &)), receiver, slot);
            return;
        }

        bidQuery.last();
        for (int i = 0; i < priceList.size(); i++)
        {
            PriceEntry &entry = priceList[i];
            entry.openBid = bidQuery.value(1).toFloat();
            entry.openAverage = (entry.openAsk + entry.openBid) / 2.0f;
            entry.highBid = bidQuery.value(2).toFloat();
            entry.highAverage = (entry.highAsk + entry.highBid) / 2.0f;
            entry.lowBid = bidQuery.value(3).toFloat();
            entry.lowAverage = (entry.lowAsk + entry.lowBid) / 2.0f;
            entry.closeBid = bidQuery.value(4).toFloat();
            entry.closeAverage = (entry.closeAsk + entry.closeBid) / 2.0f;

            if (entry.dateTime != bidQuery.value(0).toDateTime())
            {
                qWarning("DataConnection::handlePriceEntriesQuery(): Corrupt data ask and bid date time are not the same");
                emit priceEntriesLookupFinished(emptyList);
                disconnect(this, SIGNAL(priceEntriesLookupFinished(QList<PriceEntry> &)), receiver, slot);
                return;
            }
            bidQuery.previous();
        }
    }

    emit priceEntriesLookupFinished(priceList);
    disconnect(this, SIGNAL(priceEntriesLookupFinished(QList<PriceEntry> &)), receiver, slot);
}

const QSqlDatabase &DataConnection::database() const
{
    return privateData->database;
}

IsinInfo DataConnection::getIsinInfo(const QString &isin) const
{
    IsinInfo *cached = privateData->isinCache.object(isin);
    if (cached)
    {
        if (cached->timeStamp.secsTo(QTime::currentTime()) < CACHE_EXPIRE_TIME)
            return *cached;

        privateData->isinCache.remove(isin);
    }

    QString queryString = GET_INFO_TEMPLATE;
    queryString.replace("$ISIN", isin);
    QSqlQuery query(privateData->database);
    bool ok = query.exec(queryString);
    if (!ok)
    {
        qWarning("DataConnection::getIsinInfo: Failed to query database: \"%s\"", query.lastError().text().toAscii().data());
        return IsinInfo();
    }

    if (query.size() < 1)
    {
        qWarning("DataConnection::getIsinInfo(): ISIN \"%s\" does not exist", isin.toAscii().data());
        return IsinInfo();
    }
    else if (query.size() > 1)
    {
        qWarning("DataConnection::getIsinInfo(): found multiple entries for this ISIN \"%s\", returning the first", isin.toAscii().data());
    }

    query.first();
    IsinInfo *info = new IsinInfo;
    info->isin = query.value(0).toString();
    info->name = query.value(1).toString();
    info->symbol = query.value(2).toString();
    info->market = query.value(3).toString();
    info->type = query.value(4).toString();
    info->parent = query.value(5).toString();
    info->valuta = query.value(10).toString();
    info->providesBidAsk = query.value(11).toBool();
    info->availableTimeScales = 0xffffffff;
    info->timeStamp = QTime::currentTime();

    privateData->isinCache.insert(isin, info);

    return *info;
}

QString DataConnection::timeScaleIntToString(int timeScale) const
{
    QString tScale;
    switch (timeScale)
    {
        case TickByTick:
            tScale = "TBT";
            break;

        case MinuteByMinute:
            tScale = "MBM";
            break;

        case HourByHour:
            tScale = "HBH";
            break;

        case DayByDay:
            tScale = "DBD";
            break;

        case WeekByWeek:
            tScale = "WBW";
            break;

        case MonthByMonth:
            tScale = "MOBMO";
            break;

        default:
            tScale = "HBH";
            break;
    }

    return tScale;
}

int DataConnection::timeScaleStringToInt(const QString &timeScale) const
{
    if (timeScale == "TBT")
        return TickByTick;
    else if (timeScale == "MBM")
        return MinuteByMinute;
    else if (timeScale == "HBH")
        return HourByHour;
    else if (timeScale == "DBD")
        return DayByDay;
    else if (timeScale == "WBW")
        return WeekByWeek;
    else if (timeScale == "MOBMO")
        return MonthByMonth;

    return HourByHour;
}

bool DataConnection::getFirstAvailableDateTime(QObject *receiver, const char *slot, const IsinInfo &info, int timeScale)
{
    if (!info.isValid())
    {
        qWarning("DataConnection::getFirstAvailableDateTimeThreaded: invalid IsinInfo");
        return false;
    }

    QString queryString = GET_FIRST_ENTRY_TEMPLATE;
    QString oName;
    if (info.providesBidAsk)
        oName = QString(info.isin + "_ASK_" + timeScaleIntToString(timeScale));
    else
        oName = QString(info.isin + "_" + timeScaleIntToString(timeScale));

    queryString.replace("$OBJECT", oName);
    QStringList queryStringList;
    queryStringList << queryString;

    QPointer<QObject> receiverPointer = receiver;
    emit queryAvailable(queryStringList, DataConnection::FirstAvailableDateTimeQuery, receiverPointer, slot);

    return true;
}

void DataConnection::handleFirstAvailableDateTimeQuery(ResultList resultList, QueryList queryList, QPointer<QObject> receiver, const char *slot)
{
    if (!receiver)
    {
        qDebug() << "dataConnection::handleFirstAvailableDateTimeQuery(): receiver object is not valid anymore";
        return;
    }

    connect(this, SIGNAL(firstAvailableDateTimeLookupFinished(QDateTime &)), receiver, slot, Qt::DirectConnection);
    QDateTime emptyDateTime;

    if (queryList.size() < 1 || resultList.size() < 1)
    {
        qWarning("DataConnection::handleFirstAvailableDateTimeQuery(): Failed to query database: received a empty query list");
        emit firstAvailableDateTimeLookupFinished(emptyDateTime);
        disconnect(this, SIGNAL(firstAvailableDateTimeLookupFinished(QDateTime &)), receiver, slot);
        return;
    }

    QSqlQuery askQuery = queryList.at(0);
    if (!resultList.at(0))
    {
        qWarning("DataConnection::handleFirstAvailableDateTimeQuery(): Failed to query database: %s", askQuery.lastError().text().toAscii().data());
        emit firstAvailableDateTimeLookupFinished(emptyDateTime);
        disconnect(this, SIGNAL(firstAvailableDateTimeLookupFinished(QDateTime &)), receiver, slot);
        return;
    }

    if (askQuery.size() < 1)
    {
        qWarning("DataConnection::handleFirstAvailableDateTimeQuery(): Empty query");
        emit firstAvailableDateTimeLookupFinished(emptyDateTime);
        disconnect(this, SIGNAL(firstAvailableDateTimeLookupFinished(QDateTime &)), receiver, slot);
        return;
    }

    askQuery.last();

    QDateTime firstAvailableDateTime = askQuery.value(0).toDateTime();
    emit firstAvailableDateTimeLookupFinished(firstAvailableDateTime);
    disconnect(this, SIGNAL(firstAvailableDateTimeLookupFinished(QDateTime &)), receiver, slot);
}

bool DataConnection::getLastAvailableDateTime(QObject *receiver, const char *slot, const IsinInfo &info, int timeScale)
{
    if (!info.isValid())
    {
        qWarning("DataConnection::getLastAvailableDateTimeThreaded: invalid IsinInfo");
        return false;
    }

    QString queryString = GET_LAST_ENTRY_TEMPLATE;
    QString oName;
    if (info.providesBidAsk)
        oName = QString(info.isin + "_ASK_" + timeScaleIntToString(timeScale));
    else
        oName = QString(info.isin + "_" + timeScaleIntToString(timeScale));

    queryString.replace("$OBJECT", oName);
    QStringList queryStringList;
    queryStringList << queryString;

    QPointer<QObject> receiverPointer = receiver;
    emit queryAvailable(queryStringList, DataConnection::LastAvailableDateTimeQuery, receiverPointer, slot);

    return true;
}

void DataConnection::handleLastAvailableDateTimeQuery(ResultList resultList, QueryList queryList, QPointer<QObject> receiver, const char *slot)
{
    if (!receiver)
    {
        qDebug() << "dataConnection::handleLastAvailableDateTimeQuery(): receiver object is not valid anymore";
        return;
    }

    connect(this, SIGNAL(lastAvailableDateTimeLookupFinished(QDateTime &)), receiver, slot, Qt::DirectConnection);
    QDateTime emptyDateTime;

    if (queryList.size() < 1 || resultList.size() < 1)
    {
        qWarning("DataConnection::handleLastAvailableDateTimeQuery(): Failed to query database: received a empty query list");
        emit lastAvailableDateTimeLookupFinished(emptyDateTime);
        disconnect(this, SIGNAL(lastAvailableDateTimeLookupFinished(QDateTime &)), receiver, slot);
        return;
    }

    QSqlQuery askQuery = queryList.at(0);
    if (!resultList.at(0))
    {
        qWarning("DataConnection::handleLastAvailableDateTimeQuery(): Failed to query database: %s", askQuery.lastError().text().toAscii().data());
        emit lastAvailableDateTimeLookupFinished(emptyDateTime);
        disconnect(this, SIGNAL(lastAvailableDateTimeLookupFinished(QDateTime &)), receiver, slot);
        return;
    }

    if (askQuery.size() < 1)
    {
        qWarning("DataConnection::handleLastAvailableDateTimeQuery(): Empty query");
        emit lastAvailableDateTimeLookupFinished(emptyDateTime);
        disconnect(this, SIGNAL(lastAvailableDateTimeLookupFinished(QDateTime &)), receiver, slot);
        return;
    }

    askQuery.last();

    QDateTime lastAvailableDateTime = askQuery.value(0).toDateTime();
    emit lastAvailableDateTimeLookupFinished(lastAvailableDateTime);
    disconnect(this, SIGNAL(lastAvailableDateTimeLookupFinished(QDateTime &)), receiver, slot);
}

void DataConnection::subscribeToNotification(const QString &name, const QObject *object)
{
    for (int i = 0; i < privateData->subscriberList.size(); i++)
    {
        QPair<QString, QList<const QObject *> > subscriber = privateData->subscriberList.at(i);
        if (subscriber.first == name)
        {
            if (subscriber.second.indexOf(object) == -1)
                subscriber.second << object;

            return;
        }
    }

    QPair <QString, QList<const QObject *> > newSubscriber;
    newSubscriber.first = name;
    newSubscriber.second << object;

    privateData->subscriberList << newSubscriber;

    emit subscribeRequestAvailable(name);
}

void DataConnection::unsubscribeFromNotification(const QString &name, const QObject *object)
{
    for (int i = 0; i < privateData->subscriberList.size(); i++)
    {
        QPair<QString, QList<const QObject *> > subscriber = privateData->subscriberList.at(i);
        if (subscriber.first == name)
        {
            subscriber.second.removeAll(object);
            if (subscriber.second.isEmpty())
            {
                privateData->subscriberList.removeAt(i);
                emit unsubscribeRequestAvailable(name);
            }

            return;
        }
    }
}

class DataFetcher::PrivateData
{
    public:
        DataConnection *dataConnection;
        QSqlDatabase database;
};

DataFetcher::DataFetcher(DataConnection *dataConnection)
{
    privateData = new PrivateData;
    privateData->dataConnection = dataConnection;
}

DataFetcher::~DataFetcher(void)
{
    delete privateData;
}

bool DataFetcher::connectToDatabase(void)
{
    privateData->database = QSqlDatabase::addDatabase("QPSQL", "Query Thread");
    privateData->database.setConnectOptions(QString("connect_timeout=") + QString::number(DATABASE_CONNECTION_TIMEOUT));
    privateData->database.setHostName(DATABASE_ADDRESS);
    privateData->database.setDatabaseName(DATABASE_NAME);
    privateData->database.setUserName(DATABASE_USER);
    privateData->database.setPassword(DATABASE_PASSWORD);

    bool ok = privateData->database.open();
    if (!ok)
    {
        qWarning("DataFetcher::connect(): Failed to connect to database: %s", privateData->database.lastError().text().toAscii().data());
        return false;
    }

    connect(privateData->database.driver(), SIGNAL(notification(QString)), privateData->dataConnection, SIGNAL(notification(QString)), Qt::UniqueConnection);

    return true;
}

void DataFetcher::disconnectFromDatabase(void)
{
    disconnect(privateData->database.driver(), SIGNAL(notification(QString)), privateData->dataConnection, SIGNAL(notification(QString)));
    privateData->database.close();
}

void DataFetcher::query(const QStringList &queryStringList, DataConnection::QueryType queryType, QPointer<QObject> receiver, const char *slot)
{
    QList<QSqlQuery> queryList;
    QList<bool> resultList;

    for (int i = 0; i < queryStringList.size(); i++)
    {
        QSqlQuery sqlQuery(privateData->database);
        bool result = sqlQuery.exec(queryStringList.at(i));
        queryList << sqlQuery;
        resultList << result;
    }

    if (queryStringList.isEmpty())
        return;

    switch (queryType)
    {
        case DataConnection::PriceEntriesQuery:
            emit priceEntriesQueryFinished(resultList, queryList, receiver, slot);
            break;

        case DataConnection::FirstAvailableDateTimeQuery:
            emit firstAvailableDateTimeQueryFinished(resultList, queryList, receiver, slot);
            break;

        case DataConnection::LastAvailableDateTimeQuery:
            emit lastAvailableDateTimeQueryFinished(resultList, queryList, receiver, slot);
            break;

        default:
            break;
    }
}

void DataFetcher::subscribeToNotification(const QString &name)
{
    if (!privateData->database.isOpen())
        qDebug() << "DataFetcher::subscribeToNotification(): database is not open";

    privateData->database.driver()->subscribeToNotification(name);
}

void DataFetcher::unsubscribeFromNotification(const QString &name)
{
    if (!privateData->database.isOpen())
        qDebug() << "DataFetcher::unsubscribeFromNotification(): database is not open";

    privateData->database.driver()->unsubscribeFromNotification(name);
}

