#include <dbus/dbus.h>

#include <QDebug>
#include <QThread>
#include <QDBusConnection>
#include <QCoreApplication>

#include "Serv.h"
#include "addr.h"

#if (SERV_DBUS_TYPE == SERV_DBUS_TYPE_P2P)
#include "PeerWatcher.h"
#endif  /* SERV_DBUS_TYPE == SERV_DBUS_TYPE_P2P */

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

int ServAdaptor::quit()
{
    int retcode = 0;
    qDebug() << __PRETTY_FUNCTION__;
    emit servQuit(retcode);
    QCoreApplication::instance()->quit();
    return retcode;
}

Serv::Serv(QObject *parent)
    : QObject(parent)
#if (SERV_DBUS_TYPE == SERV_DBUS_TYPE_P2P)
    , m_qdbusServer(QStringLiteral(SERV_DBUS_P2P_ADDRESS), this)
    , m_p2pConn(nullptr)
#endif
    , m_adaptor(this)
{
    qDebug() << __PRETTY_FUNCTION__;

#if (SERV_DBUS_TYPE == SERV_DBUS_TYPE_SYSTEM)
    QDBusConnection c = QDBusConnection::systemBus();
#elif (SERV_DBUS_TYPE == SERV_DBUS_TYPE_SESSION)
    QDBusConnection c = QDBusConnection::sessionBus();
#endif

#if (SERV_DBUS_TYPE == SERV_DBUS_TYPE_P2P)
    if (!connect(&m_qdbusServer, &QDBusServer::newConnection,
                 this, &Serv::handleNewConnection)) {
        qDebug() << "connection of qDBusServer::newConnection signal failed";
    }
#elif (SERV_DBUS_TYPE == SERV_DBUS_TYPE_SYSTEM) || \
      (SERV_DBUS_TYPE == SERV_DBUS_TYPE_SESSION)
    if (!c.isConnected()) {
        qCritical() << QStringLiteral("Cannot connect to the D-Bus");
        exit(EXIT_FAILURE);
    }

    bool r;
    r = c.registerService(QStringLiteral(SERV_DBUS_SERVICE));
    if (!r) {
        qDebug() << __PRETTY_FUNCTION__ << QStringLiteral("Cannot registerService");
    }
    r = c.registerObject(QStringLiteral(SERV_DBUS_OBJECT_PATH), this);
    if (!r) {
        qDebug() << __PRETTY_FUNCTION__ << QStringLiteral("Cannot registerObject");
    }
#endif
}

Serv::~Serv()
{
    qDebug() << __PRETTY_FUNCTION__;
#if (SERV_DBUS_TYPE == SERV_DBUS_TYPE_P2P)
    delete m_p2pConn;
#endif
}

// Wait until connection is authenticated: it is done concurrently to the
// emitting of the newConnection signal
void Serv::connWait(const QDBusConnection &c)
{
    auto i = static_cast<DBusConnection *>(c.internalPointer());
    while (dbus_connection_get_is_connected(i) &&
           !dbus_connection_get_is_authenticated(i)) {
        qDebug() << __PRETTY_FUNCTION__;
        QThread::usleep(100);
    }
}

unsigned long Serv::connPid(const QDBusConnection &c)
{
    auto i = static_cast<DBusConnection *>(c.internalPointer());
    unsigned long pid = 0;
    if (dbus_connection_get_unix_process_id(i, &pid)) {
        return pid;
    }

    return 0;
}

unsigned long Serv::connUid(const QDBusConnection &c)
{
    auto i = static_cast<DBusConnection *>(c.internalPointer());
    unsigned long uid = -1;
    if (dbus_connection_get_unix_user(i, &uid)) {
        return uid;
    }

    return -1;
}

#if (SERV_DBUS_TYPE == SERV_DBUS_TYPE_P2P)
void Serv::handleNewConnection(const QDBusConnection &connection)
{
    QDBusConnection c(connection);

    connWait(c);

    PeerWatcher *w = new PeerWatcher(this);
    qDebug() << __PRETTY_FUNCTION__
             << c.name()
             << &c
             << w
             << "pid" << connPid(c)
             << "uid" << connUid(c);

    bool r = c.registerObject(QStringLiteral(SERV_DBUS_OBJECT_PATH), this);
    if (!r) {
        qDebug() << __PRETTY_FUNCTION__ << "Cannot registerObject";
    }

    if (!c.connect(QString(),
                   QStringLiteral("/org/freedesktop/DBus/Local"),
                   QStringLiteral("org.freedesktop.DBus.Local"),
                   QStringLiteral("Disconnected"),
                   w,
                   SLOT(handleDisconnected()))) {
        qDebug("Failed to connect to disconnect signal");
    }
}
#endif  /* SERV_DBUS_TYPE == SERV_DBUS_TYPE_P2P */
