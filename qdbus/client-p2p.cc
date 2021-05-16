#include <QCoreApplication>
#include <QtDBus>
#include <QDebug>

#include "addr.h"

int main(int ac, char **av)
{
    QCoreApplication a(ac, av);

    QDBusConnection c = QDBusConnection::connectToPeer(QStringLiteral(SERV_DBUS_P2P_ADDRESS),
                                                       QStringLiteral("serv_p2p_name"));

    if (!c.isConnected()) {
        qCritical() << "Cannot connect to the D-Bus p2p bus.\n";
        return EXIT_FAILURE;
    }

    QDBusInterface iface(QString(),
                         QStringLiteral(SERV_DBUS_OBJECT_PATH),
                         QStringLiteral(SERV_DBUS_INTERFACE),
                         c);

    if (iface.isValid()) {
        QDBusReply<double> reply;

        reply = iface.call("multiply", 2.0, 3.3);
        if (reply.isValid()) {
            printf("Reply from multiply was: %e\n", reply.value());
        } else {
            qCritical() << "Call to multiply failed:" << qPrintable(reply.error().message());
            return EXIT_FAILURE;
        }

        reply = iface.call("divide", 5.2, 1.3);
        if (reply.isValid()) {
            printf("Reply from divide was: %e\n", reply.value());
            return EXIT_SUCCESS;
        } else {
            qCritical() << "Call to divide failed:" << qPrintable(reply.error().message());
            return EXIT_FAILURE;
        }
    }

    qCritical("No D-Bus interface %s found!", QStringLiteral(SERV_DBUS_INTERFACE).toUtf8().constData());

    return EXIT_FAILURE;
}
