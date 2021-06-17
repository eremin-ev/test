
/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <QDebug>
#include <QList>
#include <QPair>
#include <QString>

static void test1()
{
    QStringList list{
        QStringLiteral("First"),
        QStringLiteral("Second"),
    };
    QVariant arg(list);
    qDebug() << arg;
}

static void test2()
{
    QVariant arg2((QStringList){
        QStringLiteral("First"),
        QStringLiteral("Second"),
    });
    qDebug() << arg2;
}

namespace {
struct StringVariant {
    QString s;
    QVariant v;
};
}

Q_DECLARE_METATYPE(StringVariant)

static void test3()
{
    const QList<StringVariant> vl = {
        {
            QStringLiteral("First"),
            QVariant(),
        },
    };
    QVariant arg = QVariant::fromValue(vl);
    qDebug() << arg;
}

static void test4()
{
    const QList<QMap<QString,QVariant>> vl = {
        {
            { QStringLiteral("First"), QVariant() },
            { QStringLiteral("Second"), QVariant() },
        },
    };
    QVariant arg = QVariant::fromValue(vl);
    qDebug() << arg;
}

static void test5()
{
    const QMap<QString,QVariant> vm = {
        { QStringLiteral("FirstUnused"), QVariant(QStringLiteral("FirstValue")) },
        { QStringLiteral("SecondUnused"), QVariant(QStringLiteral("SecondValue")) },
    };
    QVariant arg = QVariant::fromValue(vm);
    qDebug() << arg;
}

static void test6(const QString &arg)
{
    bool ok;
    qDebug() << arg.toInt(&ok) << ok;
}

int main()
{
    test1();
    test2();
    test3();
    test4();
    test5();
    test6(QStringLiteral("123"));
    test6(QStringLiteral("-123"));

    return EXIT_SUCCESS;
}
