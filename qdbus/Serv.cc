#include <QDebug>
#include <QDBusConnection>

#include "Serv.h"
#include "addr.h"

ServAdaptor::ServAdaptor(Serv *serv)
    : QDBusAbstractAdaptor(serv)
    , m_serv(serv)
{
    qDebug() << __PRETTY_FUNCTION__;
}

double ServAdaptor::multiply(double factor0, double factor1)
{
	qDebug() << __PRETTY_FUNCTION__ << factor0 << factor1;
	double product = factor0 * factor1;
	emit newProduct(product);
	return product;
}

double ServAdaptor::divide(double dividend, double divisor)
{
	qDebug() << __PRETTY_FUNCTION__ << dividend << divisor;
	double quotient = dividend / divisor;
	emit newQuotient(quotient);
	return quotient;
}

Serv::Serv(QObject *parent)
    : QObject(parent)
    , m_qdbusServer(QStringLiteral(SERV_DBUS_P2P_ADDRESS), this)
    , m_adaptor(new ServAdaptor(this))
{
    qDebug() << __PRETTY_FUNCTION__;

    if (!connect(&m_qdbusServer, &QDBusServer::newConnection,
                 this, &Serv::handleNewConnection)) {
        qDebug() << "connection of qDBusServer::newConnection signal failed";
    }

    /*QDBusConnection dbus = QDBusConnection::sessionBus();
    bool r;
    r = dbus.registerService(QStringLiteral(SERV_DBUS_SERVICE));
    if (!r) {
        qDebug() << __PRETTY_FUNCTION__ << "Cannot registerService";
    }
    r = dbus.registerObject(QStringLiteral(SERV_DBUS_OBJECT_PATH), this);
    if (!r) {
        qDebug() << __PRETTY_FUNCTION__ << "Cannot registerObject";
    }*/
}

Serv::~Serv()
{
    qDebug() << __PRETTY_FUNCTION__;
    delete m_adaptor;
    delete m_p2pConn;
}

void Serv::handleNewConnection(const QDBusConnection &connection)
{
    qDebug() << __PRETTY_FUNCTION__ << connection.name();

    m_p2pConn = new QDBusConnection(connection);

    bool r = m_p2pConn->registerObject(QStringLiteral(SERV_DBUS_OBJECT_PATH), this);
    if (!r) {
        qDebug() << __PRETTY_FUNCTION__ << "Cannot registerObject";
    }
}
