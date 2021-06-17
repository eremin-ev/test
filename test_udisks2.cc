/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <QCoreApplication>
#include <QtDBus>
#include <QDebug>

template <typename ...Args>
static void test_call(QDBusInterface *iface, const QString &method, Args && ... args)
{
    if (!iface->isValid()) {
        qCritical("No D-Bus interface %s found!", iface->interface().toUtf8().constData());
        exit(EXIT_FAILURE);
    }

    QDBusMessage reply = iface->call(method, args...);

    if (reply.type() == QDBusMessage::ErrorMessage) {
        qDebug()
            << QStringLiteral("Failed to send")
            << method
            << qPrintable(reply.errorMessage());
        exit(EXIT_FAILURE);
    } else if (reply.type() != QDBusMessage::ReplyMessage) {
        qDebug("Bad reply type %d", reply.type());
        exit(EXIT_FAILURE);
    }

    // Expect array of object paths
    const QDBusArgument &ao = reply.arguments().at(0).value<QDBusArgument>();
    if (ao.currentType() != QDBusArgument::ArrayType) {
        fprintf(stderr, "ERROR: Unexpected argument type of EnumerateDevices call\n");
    }

    ao.beginArray();
    while (!ao.atEnd()) {
        qDebug() << QStringLiteral("Path") << qdbus_cast<QDBusObjectPath>(ao).path();
    }
    ao.endArray();
}

int main(int argc, char **argv)
{
    QCoreApplication a(argc, argv);

    const QDBusConnection &c = QDBusConnection::systemBus();

    if (!c.isConnected()) {
        qCritical() << "Cannot connect to the D-Bus system bus.\n";
        return EXIT_FAILURE;
    }

    QDBusInterface iface(QStringLiteral("org.freedesktop.UDisks2"),
                         QStringLiteral("/org/freedesktop/UDisks2/Manager"),
                         QStringLiteral("org.freedesktop.UDisks2.Manager"),
                         c);

    const QMap<QString,QVariant> vm = {
        { QStringLiteral("FirstUnused"), QVariant(QStringLiteral("FirstValue")) },
        { QStringLiteral("SecondUnused"), QVariant(QStringLiteral("SecondValue")) },
    };
    QVariant arg = QVariant::fromValue(vm);
    qDebug() << arg;

    test_call(&iface, QStringLiteral("GetBlockDevices"), arg);

    return EXIT_SUCCESS;
}
