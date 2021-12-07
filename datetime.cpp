#include "datetime.h"
#include "strategie.h"

DateTime::DateTime(void): QDateTime()
{
    dt_allowedDays = DateTime::Monday | DateTime::Tuesday | DateTime::Wensday | DateTime::Thursday | DateTime::Friday;
    dt_minimumTime.setHMS(8, 0, 0);
    dt_maximumtime.setHMS(18, 0, 0);
}

DateTime::~DateTime(void)
{

}

int DateTime::allowedSecsTo(const DateTime &other)
{
    DateTime tmp = *this;
    DateTime store;

    int seconds = secsTo(other);
    if (seconds < 0)
    {
        seconds = 0;
        while (tmp > other)
        {
            if (other.date() < (store = tmp.allowedAddDays(-1)).date())
            {
                tmp = store;
                seconds -= dt_minimumTime.secsTo(dt_maximumtime);
            }
            else if (other < (store = tmp.allowedAddSecs(-3600)))
            {
                tmp = store;
                seconds -= 3600;
            }
            else
            {
                tmp = tmp.allowedAddSecs(-1);
                --seconds;
            }
        }
    }
    else
    {
        seconds = 0;
        while (tmp < other)
        {
            if (other.date() > (store = tmp.allowedAddDays(1)).date())
            {
                tmp = store;
                seconds += dt_minimumTime.secsTo(dt_maximumtime);
            }
            else if (other > (store = tmp.allowedAddSecs(3600)))
            {
                tmp = store;
                seconds += 3600;
            }
            else
            {
                tmp = tmp.allowedAddSecs(1);
                ++seconds;
            }
        }
    }

    return seconds;
}

int DateTime::allowedDaysTo(const DateTime &other)
{
    DateTime tmp = *this;

    int days = daysTo(other);
    if (days < 0)
    {
        days = 0;
        while (tmp.date() > other.date())
        {
            tmp = tmp.allowedAddDays(-1);
            --days;
        }
    }
    else
    {
        days = 0;
        while (tmp.date() < other.date())
        {
            tmp = tmp.allowedAddDays(1);
            ++days;
        }
    }

    return days;
}

DateTime DateTime::allowedAddSecs(int seconds)
{
    DateTime dateTime = *this;
    DateTime tmp;

    int counter = seconds;
    if (seconds < 0)
    {
        while (counter != 0)
        {
            tmp = dateTime;
            tmp.setTime(dt_minimumTime);
            if (counter > dateTime.secsTo(tmp))
            {
                QDateTime dt = dateTime.addSecs(counter);
                dateTime.setDate(dt.date());
                dateTime.setTime(dt.time());
                return dateTime;
            }

            counter -= dateTime.secsTo(tmp);
            dateTime = dateTime.allowedAddDays(-1);
            dateTime.setTime(dt_maximumtime);
        }
    }
    else
    {
        while (counter != 0)
        {
            tmp = dateTime;
            tmp.setTime(dt_maximumtime);
            if (counter < dateTime.secsTo(tmp))
            {
                QDateTime dt = dateTime.addSecs(counter);
                dateTime.setDate(dt.date());
                dateTime.setTime(dt.time());
                return dateTime;
            }

            counter -= dateTime.secsTo(tmp);
            dateTime = dateTime.allowedAddDays(1);
            dateTime.setTime(dt_minimumTime);
        }
    }

    return dateTime;
}

DateTime DateTime::allowedAddDays(int days)
{
    DateTime dateTime = *this;
    QDateTime tmp;

    int counter = days;
    while (counter != 0)
    {
        if (days < 0)
            tmp = dateTime.addDays(-1);
        else
            tmp = dateTime.addDays(1);

        dateTime.setDate(tmp.date());
        dateTime.setTime(tmp.time());

        switch (dateTime.date().dayOfWeek())
        {
            case Qt::Monday:
                if (!(dt_allowedDays & DateTime::Monday))
                    continue;
                break;

            case Qt::Tuesday:
                if (!(dt_allowedDays & DateTime::Tuesday))
                    continue;
                break;

            case Qt::Wednesday:
                if (!(dt_allowedDays & DateTime::Wensday))
                    continue;
                break;

            case Qt::Thursday:
                if (!(dt_allowedDays & DateTime::Thursday))
                    continue;
                break;

            case Qt::Friday:
                if (!(dt_allowedDays & DateTime::Friday))
                    continue;
                break;

            case Qt::Saturday:
                if (!(dt_allowedDays & DateTime::Saturday))
                    continue;
                break;

            case Qt::Sunday:
                if (!(dt_allowedDays & DateTime::Sunday))
                    continue;
                break;
        }
        if (days < 0)
            counter += 1;
        else
            counter -= 1;
    }

    return dateTime;
}

DateTime DateTime::allowedAddMonths(int months)
{
    DateTime dateTime = *this;
    dateTime.setDate(dateTime.addMonths(months).date());

    return dateTime.firstAllowedDayOfMonth();
}

void DateTime::setAllowedDays(DateTime::Days days)
{
    dt_allowedDays = days;
}

int DateTime::allowedDaysCount(void)
{
    int count = 0;

    if (dt_allowedDays & DateTime::Monday)
        count++;

    if (dt_allowedDays & DateTime::Tuesday)
        count++;

    if (dt_allowedDays & DateTime::Wensday)
        count++;

    if (dt_allowedDays & DateTime::Thursday)
        count++;

    if (dt_allowedDays & DateTime::Friday)
        count++;

    if (dt_allowedDays & DateTime::Saturday)
        count++;

    if (dt_allowedDays & DateTime::Sunday)
        count++;

    return count;
}

void DateTime::setMinimumAllowedTime(const QTime &minimumTime)
{
    dt_minimumTime = minimumTime;
}

void DateTime::setMaximumAllowedTime(const QTime &maximumTime)
{
    dt_maximumtime = maximumTime;
}

DateTime DateTime::firstAllowedDayOfMonth(void)
{
    DateTime dateTime = *this;
    dateTime.setDate(QDate(this->date().year(), this->date().month(), 1));
    dateTime.setDate(dateTime.addDays(-1).date());
    dateTime = dateTime.allowedAddDays(1);

    return dateTime;
}

DateTime DateTime::firstAllowedDayOfWeek(void)
{
    DateTime dateTime = *this;
    dateTime.setDate(dateTime.addDays(-dateTime.date().dayOfWeek()).date());
    dateTime = dateTime.allowedAddDays(1);

    return dateTime;
}

