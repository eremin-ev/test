#include <QCoreApplication>
#include <QtDBus>
#include <QDebug>

int main(int ac, char **av)
{
	QCoreApplication a(ac, av);

	if (!QDBusConnection::sessionBus().isConnected()) {
		qCritical() << "Cannot connect to the D-Bus session bus.\n";
		return EXIT_FAILURE;
	}

    QDBusInterface iface("my.test.Calc_Service",
                         "/my/test/Calc_Object",
                         "my.test.Calc_Interface",
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

	qCritical() << "No D-Bus interface found!";
	return EXIT_FAILURE;
}
