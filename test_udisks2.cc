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

    if (reply.type() == QDBusMessage::ReplyMessage) {
        qDebug() << QStringLiteral("Ok") << reply;
    } else if (reply.type() == QDBusMessage::ErrorMessage) {
        qDebug()
            << QStringLiteral("Failed to send")
            << method
            << qPrintable(reply.errorMessage());
    } else {
        qDebug("Failed reply %i", reply.type());
        exit(EXIT_FAILURE);
    }
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

#if 0
	QDBusMessage enumRes = devEnum.call("EnumerateDevices");
    // TODO/FIXME: error checking
//    if (enumRes.type() == QDBusMessage::ErrorMessage)
//    {
//        fprintf(stderr, "ERROR: Can't call EnumerateDevices\n");
//        fprintf(stderr, "       %s : %s\n", qPrintable(enumRes.errorName()), qPrintable(enumRes.errorMessage()));
//    }
//
//    if (enumRes.type() != QDBusMessage::ReplyMessage || !enumRes.arguments().at(0).canConvert<QDBusArgument>())
//    {
//        fprintf(stderr, "ERROR: Unexpected result type of EnumerateDevices call\n");
//    }
//
    const QDBusArgument enumArg = enumRes.arguments().at(0).value<QDBusArgument>();
//    if (enumArg.currentType() != QDBusArgument::ArrayType)
//    {
//        fprintf(stderr, "ERROR: Unexpected argument type of EnumerateDevices call\n");
//    }

    enumArg.beginArray();
    while (!enumArg.atEnd())
    {
        addDevice(qdbus_cast<QDBusObjectPath>(enumArg));
    }
    enumArg.endArray();
#endif

    return EXIT_SUCCESS;
}
