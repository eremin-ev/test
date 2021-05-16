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

signals:
	void newProduct(double product);
	void newQuotient(double quotient);

private:
    Serv *m_serv;
};

class Serv : public QObject {
    Q_OBJECT

public:
	Serv(QObject *parent);
	virtual ~Serv();

    void handleNewConnection(const QDBusConnection &connection);

private:
    QDBusServer m_qdbusServer;
    ServAdaptor *m_adaptor;
    QDBusConnection *m_p2pConn;
};

#endif  /* SERV_H */
