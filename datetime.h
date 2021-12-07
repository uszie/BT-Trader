#ifndef DATETIME_H
#define DATETIME_H

#include <qdatetime.h>


class DateTime : public QDateTime
{
    public:
        enum Days
        {
            Monday = 1,
            Tuesday = 2,
            Wensday = 4,
            Thursday = 8,
            Friday = 16,
            Saturday = 32,
            Sunday = 64
        };

        DateTime(void);
        ~DateTime(void);

        int allowedSecsTo(const DateTime &other);
        int allowedDaysTo(const DateTime &other);
        DateTime allowedAddSecs(int seconds);
        DateTime allowedAddDays(int days);
        DateTime allowedAddMonths(int months);
        void setAllowedDays(DateTime::Days days);
        int allowedDays(void) { return dt_allowedDays; };
        int allowedDaysCount(void);
        void setMinimumAllowedTime(const QTime &minimumTime);
        void setMaximumAllowedTime(const QTime &maximumTime);
        const QTime &minimumAllowedTime(void) { return dt_minimumTime; };
        const QTime &maximumAllowedTime(void) { return dt_maximumtime; };
        DateTime firstAllowedDayOfMonth(void);
        DateTime firstAllowedDayOfWeek(void);

    private:
        int dt_allowedDays;
        QTime dt_minimumTime;
        QTime dt_maximumtime;
};

#endif // DATETIME_H
