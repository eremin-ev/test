/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <QDebug>
#include <QList>
#include <QString>

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

    QList<Qt::DayOfWeek> days;
    QStringList dayList = dayStr.split(',');

    qDebug() << __PRETTY_FUNCTION__;

    for (QStringList::const_iterator i = dayList.constBegin();
                                     i != dayList.constEnd(); ++i) {
        QMap<QString, Qt::DayOfWeek>::const_iterator d = strToDayOfWeek.find(*i);
        if (d != strToDayOfWeek.constEnd()) {
            qDebug() << "Ok found:" << d.key() << d.value();
            dayIsPresented[d.value() - Qt::Monday] = 1;
        } else {
            qDebug() << "Syntax error:" << *i;
        }
    }

    for (int day = 0; day < ndays; day++) {
        if (dayIsPresented[day]) {
            days.append(static_cast<Qt::DayOfWeek>(day + Qt::Monday));
        }
    }

    return days;
}

int main()
{
    QString s_good = QStringLiteral("Mo,Th,Su");
    QString s_bad = QStringLiteral("Wa,Th,Qe,Error,Su,Mo");

    QList<Qt::DayOfWeek> d_good = parseDays(s_good);
    QList<Qt::DayOfWeek> d_bad = parseDays(s_bad);

    qDebug() << "Good" << d_good << "== Bad" << d_bad << ":" << (d_good == d_bad);

    return 0;
}
