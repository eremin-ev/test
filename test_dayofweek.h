#ifndef DAYOFWEEK_H
#define DAYOFWEEK_H

#include <QObject>

class WeekDaySettings : public QObject {
    Q_OBJECT
public:
    enum DayOfWeek {
        Mon = 1,    // Monday
        Tue,        // Tuesday
        Wed,        // Wednesday
        Thu,        // Thursday
        Fri,        // Friday
        Sat,        // Saturday
        Sun         // Sunday
    };
    Q_ENUM(DayOfWeek);
};

#endif  /* DAYOFWEEK_H */
