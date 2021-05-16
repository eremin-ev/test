#include <QCoreApplication>
#include <QtDBus>
#include <QDebug>

#include "addr.h"

int main(int ac, char **av)
{
	QCoreApplication a(ac, av);

	if (!QDBusConnection::sessionBus().isConnected()) {
		qCritical() << "Cannot connect to the D-Bus session bus.\n";
		return EXIT_FAILURE;
	}

    QDBusInterface iface(QStringLiteral(SERV_DBUS_SERVICE),
                         QStringLiteral(SERV_DBUS_OBJECT_PATH),
                         QStringLiteral(SERV_DBUS_INTERFACE),
                         QDBusConnection::sessionBus());

	if (iface.isValid()) {
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
			return EXIT_SUCCESS;
		} else {
			qCritical() << "Call to divide failed:" << qPrintable(reply.error().message());
			return EXIT_FAILURE;
		}
	}

	qCritical("No D-Bus interface %s found!", QStringLiteral(SERV_DBUS_INTERFACE).toUtf8().constData());

	return EXIT_FAILURE;
}
