/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <QDebug>
#include <QList>
#include <QPair>
#include <QString>
#include <QTime>

#include "test_dayofweek.h"

static QList<Qt::DayOfWeek> parseDays(const QString &dayStr)
{
	const QMap<QString, Qt::DayOfWeek> strToDayOfWeek = {
		{ "Mo", Qt::Monday },
		{ "Tu", Qt::Tuesday },
		{ "We", Qt::Wednesday },
		{ "Th", Qt::Thursday },
		{ "Fr", Qt::Friday },
		{ "Sa", Qt::Saturday },
		{ "Su", Qt::Sunday }
	};

    const int ndays = strToDayOfWeek.size();
    int dayIsPresented[ndays] = { 0 };

    QStringList dayList = dayStr.split(',');

    for (QStringList::const_iterator i = dayList.constBegin();
                                     i != dayList.constEnd(); ++i) {
        QMap<QString, Qt::DayOfWeek>::const_iterator d = strToDayOfWeek.find(*i);
        if (d != strToDayOfWeek.constEnd()) {
            //qDebug() << "Ok found:" << d.key() << d.value();
            dayIsPresented[d.value() - Qt::Monday] = 1;
        }
    }

    QList<Qt::DayOfWeek> days;
    for (int day = 0; day < ndays; day++) {
        if (dayIsPresented[day]) {
            days.append(static_cast<Qt::DayOfWeek>(day + Qt::Monday));
        }
    }

    return days;
}

struct TimeRange {
    QTime from;
    QTime to;
};

TimeRange parseTime(const QStringList &l)
{
    TimeRange timeRange;
    if (l.size() == 2 && l.at(1).size() != 0) {
        timeRange.from = QTime::fromString(l.at(0), QStringLiteral("HH.mm"));
        timeRange.to = QTime::fromString(l.at(1), QStringLiteral("HH.mm"));
        qDebug() << timeRange.from << timeRange.to;
    } else {
        qDebug() << "Time syntax error";
    }

    return timeRange;
}

void test_1(const QString &s_good)
{
    QStringList p_good = s_good.split(':');
    QList<Qt::DayOfWeek> l_good = parseDays(p_good.at(0));
    TimeRange t;

    //qDebug() << p_good << p_good.size();

    if (p_good.size() > 1) {
        t = parseTime(p_good.at(1).split('-'));
    }

    qDebug() << p_good << l_good << t.from << t.to;;
}

void test_2()
{
    QDate date = QDate::currentDate();

    enum WeekDaySettings::DayOfWeek day1 = (enum WeekDaySettings::DayOfWeek)date.dayOfWeek();
    QVariant vDay1 = QVariant::fromValue(day1);
    QString sDay1 = vDay1.toString();
    qDebug() << date << day1 << vDay1 << sDay1;

    enum Qt::DayOfWeek day2 = (enum Qt::DayOfWeek)date.dayOfWeek();
    QVariant vDay2 = QVariant::fromValue(day2);
    QString sDay2 = vDay2.toString();
    qDebug() << date << day2 << vDay2 << sDay2;
}

bool test_3()
{
    const QMap<Qt::DayOfWeek, QString> d2s = {
        { Qt::Monday,    "Mo" },
        { Qt::Tuesday,   "Tu" },
        { Qt::Wednesday, "We" },
        { Qt::Thursday,  "Th" },
        { Qt::Friday,    "Fr" },
        { Qt::Saturday,  "Sa" },
        { Qt::Sunday,    "Su" },
    };

    QString day = d2s.value((Qt::DayOfWeek)QDate::currentDate().dayOfWeek());

    if (day.isNull()) {
        qDebug("%s: bad day of week", __PRETTY_FUNCTION__);
        return false;
    }

    qDebug("%s: %s", __PRETTY_FUNCTION__, day.toUtf8().constData());

    return true;
}

bool test_4()
{
    const QString d2s[] = {
        QStringLiteral(""),
        QStringLiteral("Mo"),
        QStringLiteral("Tu"),
        QStringLiteral("We"),
        QStringLiteral("Th"),
        QStringLiteral("Fr"),
        QStringLiteral("Sa"),
        QStringLiteral("Su"),
    };

    const QString &day = d2s[QDate::currentDate().dayOfWeek()];

    if (day.isNull()) {
        qDebug("%s: bad day of week", __PRETTY_FUNCTION__);
        return false;
    }

    qDebug("%s: %s", __PRETTY_FUNCTION__, day.toUtf8().constData());

    return true;
}

typedef QMap<QString, bool> MapData;

void test_5()
{
    QVector<QPair<QString, MapData>> mapData = {
        {
            QStringLiteral("first.conf"), {
                { QStringLiteral("AddApples"), true },
                { QStringLiteral("AddStawberry"), false },
                { QStringLiteral("AddGrape"), true },
            }
        }
    };

    for (const auto &d : mapData) {
        qDebug() << d;
    }
}

void test_6()
{
    struct MapData {
        QString filename;
        QMap<QString, bool> mapData;
    };

    QVector<MapData> mapData = {
        {
            QStringLiteral("first.conf"), {
                { QStringLiteral("AddApples"), true },
                { QStringLiteral("AddStawberry"), false },
                { QStringLiteral("AddGrape"), true },
            }
        }
    };

    for (const auto &d : mapData) {
        qDebug() << d.filename << d.mapData;
    }
}

int main()
{
    test_1(QStringLiteral(""));
    test_1(QStringLiteral("Mo,Th,Su"));
    test_1(QStringLiteral("Mo,Th,Su:14.00-15.00"));
    test_1(QStringLiteral("Mo,Th,Su:14.00-"));
    test_1(QStringLiteral("Wa,Th,Qe,Error,Su,Mo"));

    test_2();
    test_3();
    test_4();
    test_5();
    test_6();

#if 0
    QStringList p_bad = s_bad.split(':');
    if (p_bad.size() < 1) {
        qDebug() << "Bad syntax:" << s_bad;
    } else {
        QString d_bad = p_bad.at(0);
        QString t_bad = p_bad.at(1);
        qDebug() << p_bad << t_bad;
    }

    QList<Qt::DayOfWeek> l_good = parseDays(d_good);
    QList<Qt::DayOfWeek> l_bad = parseDays(d_bad);

    qDebug() << "Good" << d_good << "== Bad" << d_bad << ":" << (d_good == d_bad);
#endif

    return 0;
}
