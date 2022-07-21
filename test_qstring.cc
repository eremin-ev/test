/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <QDebug>
#include <QString>
#include <QRegularExpression>

static void log_fmt(uid_t uid, const QString &key, const QString &value)
{
    QString entry = QString("%1 changed to %2 for uid %3").arg(key)
                                                          .arg(value)
                                                          .arg(QString::number(uid));

    qDebug() << qPrintable(entry);
    qDebug() << entry;
}

static void test_ternary(bool enabled)
{
    qDebug() << "Enabled" << (enabled ? QStringLiteral("true") : QStringLiteral("false"));
}

bool isValidName0(const QString &name)
{
    const QRegExp r("^\\w[\\w. \\-]*$");
    if (r.exactMatch(name))
        return true;

    return false;
}

bool isValidName1(const QString &name)
{
    const QRegExp r("^[A-Za-zА-Яа-яЁё][A-Za-zА-Яа-я0-9Ёё. \\-]*$");
    if (r.exactMatch(name))
        return true;

    return false;
}

//
// QRegularExpression Class [1] allows using Unicode Categories [2], see
// "Handling accented letters in QRegularExpressions in Qt5" [3] for details
//
// [1] QRegularExpression Class
//     https://doc.qt.io/qt-5/qregularexpression.html
//
// [2] Unicode Regular Expressions: Unicode Categories
//     https://www.regular-expressions.info/unicode.html#category
//
// [3] Handling accented letters in QRegularExpressions in Qt5
//     https://stackoverflow.com/questions/38001256/handling-accented-letters-in-qregularexpressions-in-qt5
//
bool isValidName2(const QString &name)
{
    const QRegularExpression r("^\\p{L}[\\p{L}\\d. \\-]*$");
    if (r.match(name).hasMatch())
        return true;

    return false;
}

void testRegExp()
{
    const struct {
        QString name;
        bool expect;
    } names[] = {
        { .name = QStringLiteral("-name"), .expect = false },
        { .name = QStringLiteral("%name"), .expect = false },
        { .name = QStringLiteral("name-one"), .expect = true },
        { .name = QStringLiteral("name-one%"), .expect = false },
        { .name = QStringLiteral("test+%"), .expect = false },
        { .name = QStringLiteral("+Ерёмин"), .expect = false },
        { .name = QStringLiteral("Ерёмин-Е-В-"), .expect = true },
        { .name = QStringLiteral("Ерёмин-Е.В."), .expect = true },
        { .name = QStringLiteral("Ерёмин Е. В."), .expect = true },
        { .name = QStringLiteral("Ерёмин+%"), .expect = false },
        { .name = QStringLiteral("#Ерёмин123+%"), .expect = false },
        { .name = QStringLiteral("Ерёмин123"), .expect = true },
        { .name = QStringLiteral("Ерёмин-123"), .expect = true },
        { .name = QStringLiteral("Ерёмин123."), .expect = true },
        { .name = QStringLiteral("Ерёмин123+%"), .expect = false },
    };

    for (const auto &n : names) {
        bool r = isValidName0(n.name);
        qDebug() << "isValidName0(" << n.name << ")"
                 << r << (r == n.expect ? "Ok" : "Failed");
    }

    for (const auto &n : names) {
        bool r = isValidName1(n.name);
        qDebug() << "isValidName1(" << n.name << ")"
                 << r << (r == n.expect ? "Ok" : "Failed");
    }

    for (const auto &n : names) {
        bool r = isValidName2(n.name);
        qDebug() << "isValidName2(" << n.name << ")"
                 << r << (r == n.expect ? "Ok" : "Failed");
    }
}

bool present(const QVector<QString> &names, const QString &name)
{
    return std::any_of(names.begin(), names.end(), [&name](const QString &l) {
        return l.toLower() == name.toLower();
    });
}

void testPresence()
{
    const QVector<QString> names = {
        QStringLiteral("name-one"),
        QStringLiteral("name-two"),
        QStringLiteral("test-three"),
        QStringLiteral("Ерёмин1"),
        QStringLiteral("Ерёмин.Е.В."),
        QStringLiteral("Ерёмин Е. В."),
        QStringLiteral("Ерёмин123"),
    };

    const QString n1 = QStringLiteral("ерёмин е. в.");
    const QString n2 = QStringLiteral("ерёмин");

    qDebug() << "present(" << n1 << ")" << present(names, n1);
    qDebug() << "present(" << n2 << ")" << present(names, n2);

    qDebug() << "contains(" << n1 << ")" << names.contains(n1);
    qDebug() << "contains(" << n2 << ")" << names.contains(n2);
}

void testEmpty()
{
    QString a = QString();
    QString b = QStringLiteral("B test string");
    QString c = QStringLiteral("C test string");

    printf("%s A p %p len %d\n", __func__,
           a.toUtf8().constData(), a.size());

    printf("%s B p %p s '%s' len %d\n", __func__,
           b.toUtf8().constData(), b.toUtf8().constData(), b.size());

    printf("%s C p %p s '%s' len %d\n", __func__,
           b.toUtf8(), b.toUtf8(), b.size());
}

void testToNumber()
{
    QString a("QDBusServer-ffffffffec30d180");
    QString b("QDBusServer-ffffffffec30fdf0");

    bool ok = false;
    printf("%s a %lx, ok %d\n", __func__,
           a.remove(QStringLiteral("QDBusServer-")).toULong(&ok, 16), ok);
    printf("%s ok %d\n", __func__, ok);

    ok = false;
    printf("%s b %lx, ok %d\n", __func__,
           b.remove(QStringLiteral("QDBusServer-")).toULong(&ok, 16), ok);
    printf("%s ok %d\n", __func__, ok);
}

int main()
{
    log_fmt(1000, QStringLiteral("option1"), QStringLiteral("value1"));
    log_fmt(1001, QStringLiteral("option2"), QStringLiteral("value2"));
    log_fmt(1002, QStringLiteral("option3"), QStringLiteral("value3"));

    test_ternary(true);
    test_ternary(false);

    testRegExp();

    testPresence();

    testEmpty();

    testToNumber();

    return 0;
}
