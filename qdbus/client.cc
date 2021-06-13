#include <QCoreApplication>
#include <QtDBus>
#include <QDebug>

#include "addr.h"

#if (SERV_DBUS_TYPE == SERV_DBUS_TYPE_SYSTEM) || \
    (SERV_DBUS_TYPE == SERV_DBUS_TYPE_SESSION)
static QDBusInterface &connectToBus(const QDBusConnection &c)
{
    if (!c.isConnected()) {
        qCritical() << "Cannot connect to the D-Bus";
        exit(EXIT_FAILURE);
    }

    static
    QDBusInterface iface(QStringLiteral(SERV_DBUS_SERVICE),
                         QStringLiteral(SERV_DBUS_OBJECT_PATH),
                         QStringLiteral(SERV_DBUS_INTERFACE),
                         c);

    return iface;
}
#elif (SERV_DBUS_TYPE == SERV_DBUS_TYPE_P2P)
static QDBusInterface &connectToPeer()
{
    QDBusConnection c = QDBusConnection::connectToPeer(QStringLiteral(SERV_DBUS_P2P_ADDRESS),
                                                       QStringLiteral("serv_p2p_name"));

    if (!c.isConnected()) {
        qCritical() << "Cannot connect to the D-Bus p2p bus.\n";
        exit(EXIT_FAILURE);
    }

    static
    QDBusInterface iface(QString(),
                         QStringLiteral(SERV_DBUS_OBJECT_PATH),
                         QStringLiteral(SERV_DBUS_INTERFACE),
                         c);

    return iface;
}
#else
    #error "Wrong SERV_DBUS_TYPE: " #SERV_DBUS_TYPE
#endif

int main(int argc, char **argv)
{
    QCoreApplication a(argc, argv);

#if (SERV_DBUS_TYPE == SERV_DBUS_TYPE_SYSTEM)
    QDBusInterface &iface = connectToBus(QDBusConnection::systemBus());
#elif  (SERV_DBUS_TYPE == SERV_DBUS_TYPE_SESSION)
    QDBusInterface &iface = connectToBus(QDBusConnection::sessionBus());
#elif (SERV_DBUS_TYPE == SERV_DBUS_TYPE_P2P)
    QDBusInterface &iface = connectToPeer();
#else
    #error "Wrong SERV_DBUS_TYPE: " #SERV_DBUS_TYPE
#endif

	if (!iface.isValid()) {
        qCritical("No D-Bus interface %s found!", QStringLiteral(SERV_DBUS_INTERFACE).toUtf8().constData());
        return EXIT_FAILURE;
	}

    QDBusReply<double> reply = iface.call("multiply", 2.0, 3.3);
    if (reply.isValid()) {
        printf("Reply from multiply was: %e\n", reply.value());
    } else {
        qCritical() << "Call to multiply failed:" << qPrintable(reply.error().message());
        return EXIT_FAILURE;
    }

    reply = iface.call("divide", 5.2, 1.3);
    if (reply.isValid()) {
        printf("Reply from divide was: %e\n", reply.value());
    } else {
        qCritical() << "Call to divide failed:" << qPrintable(reply.error().message());
        return EXIT_FAILURE;
    }

    if (argc > 1 && ::strncmp(argv[1], "quit", 4) == 0) {
        QDBusReply<int> replyb;
        replyb = iface.call("quit");
        if (replyb.isValid()) {
            printf("Reply from quit was: %d\n", replyb.value());
        } else {
            qCritical() << "Call to quit failed:" << qPrintable(replyb.error().message());
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
