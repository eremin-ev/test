#include <QtCore>
#include <QtDBus>

#include "Listen.h"

int main(int ac, char **av)
{
	QCoreApplication a(ac, av);

	if (!QDBusConnection::sessionBus().isConnected()) {
		qCritical() << "Cannot connect to the D-Bus session bus!";
		return EXIT_FAILURE;
	}

	Listen l;

	QDBusConnection::sessionBus().connect(QString(),
					      QString(),
					      "my.test.Calc_Interface",
					      "newProduct",
					      "d",
					      &l,
					      SLOT(reportNewProduct(double)));

	QDBusConnection::sessionBus().connect(QString(),
					      QString(),
					      "my.test.Calc_Interface",
					      "newQuotient",
					      "d",
					      &l,
					      SLOT(reportNewQuotient(double)));

	return a.exec();
}
