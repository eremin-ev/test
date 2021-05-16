#include <QtCore>
#include <QtDBus>

#include "Listener.h"
#include "addr.h"

int main(int ac, char **av)
{
	QCoreApplication a(ac, av);

    QDBusConnection c = QDBusConnection::sessionBus();

	if (!c.isConnected()) {
		qCritical() << "Cannot connect to the D-Bus session bus!";
		return EXIT_FAILURE;
	}

    bool r;
    Listener l;
    const char *slotNewProduct = SLOT(reportNewProduct(double));
    const char *slotNewQuotient = SLOT(reportNewQuotient(double));

    r = c.connect(QStringLiteral(SERV_DBUS_SERVICE),
                  QStringLiteral(SERV_DBUS_OBJECT_PATH),
                  QStringLiteral(SERV_DBUS_INTERFACE),
                  QStringLiteral("newProduct"),
                  QStringLiteral("d"),
                  &l, slotNewProduct);

    if (r) {
        qDebug() << "Ok connected" << slotNewProduct;
    } else {
        qDebug("Cannot connect reportNewProduct");
    }

    r = c.connect(QStringLiteral(SERV_DBUS_SERVICE),
                  QStringLiteral(SERV_DBUS_OBJECT_PATH),
                  QStringLiteral(SERV_DBUS_INTERFACE),
                  QStringLiteral("newQuotient"),
                  QStringLiteral("d"),
                  &l, slotNewQuotient);

    if (r) {
        qDebug() << "Ok connected" << slotNewQuotient;
    } else {
        qDebug("Cannot connect %s", slotNewQuotient);
    }

	return a.exec();
}
