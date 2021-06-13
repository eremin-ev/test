#ifndef SERV_H
#define SERV_H

#include <QObject>
#include <QDBusServer>
#include <QDBusAbstractAdaptor>

#include "addr.h"

class Serv;
class ServAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", SERV_DBUS_INTERFACE)

public:
    explicit ServAdaptor(Serv *serv);

public slots:
    double multiply(double factor0, double factor2);
    double divide(double divident, double divisor);
    int quit();

signals:
	void newProduct(double product);
	void newQuotient(double quotient);
	void servQuit(int retcode);

private:
    Serv *m_serv;
};

class Serv : public QObject {
    Q_OBJECT

public:
	Serv(QObject *parent);
	virtual ~Serv();

#if (SERV_DBUS_TYPE == SERV_DBUS_TYPE_P2P)
    void handleNewConnection(const QDBusConnection &connection);
#endif

private:
#if (SERV_DBUS_TYPE == SERV_DBUS_TYPE_P2P)
    QDBusServer m_qdbusServer;
    QDBusConnection *m_p2pConn;
#endif
    ServAdaptor m_adaptor;
};

#endif  /* SERV_H */
