/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <dbus/dbus.h>

#include <QDebug>
#include <QThread>
#include <QDBusConnection>
#include <QCoreApplication>

#include "authd-addr.h"
#include "authd.h"

/*
 * Adaptor
 */

AuthdAdaptor::AuthdAdaptor(Authd *authd)
    : QDBusAbstractAdaptor(authd)
    , m_authd(authd)
{
    qDebug() << __PRETTY_FUNCTION__;
}

double AuthdAdaptor::multiply(double factor0, double factor1)
{
    qDebug() << __PRETTY_FUNCTION__ << factor0 << factor1;
    double product = factor0 * factor1;
    emit newProduct(product);
    return product;
}

double AuthdAdaptor::divide(double dividend, double divisor)
{
    qDebug() << __PRETTY_FUNCTION__ << dividend << divisor;
    double quotient = dividend / divisor;
    emit newQuotient(quotient);
    return quotient;
}

QString AuthdAdaptor::begin(const QString &authType)
{
    qDebug() << __PRETTY_FUNCTION__ << "requested authType:" << authType;
    return QString("Password");
}

int AuthdAdaptor::quit()
{
    int retcode = 0;
    qDebug() << __PRETTY_FUNCTION__;
    emit servQuit(retcode);
    QCoreApplication::instance()->quit();
    return retcode;
}

/*
 * Disconnected watcher
 */
AuthdWatcher::AuthdWatcher(QObject *parent)
    : QObject(parent)
{
    qDebug() << __PRETTY_FUNCTION__ << this;
}

AuthdWatcher::~AuthdWatcher()
{
    qDebug() << __PRETTY_FUNCTION__ << this;
}

void AuthdWatcher::handleDisconnected()
{
    deleteLater();
    qDebug() << __PRETTY_FUNCTION__ << this;
}

/*
 * Authd
 */

Authd::Authd(QObject *parent)
    : QObject(parent)
    , m_qdbusServer(QStringLiteral(AUTHD_DBUS_P2P_ADDRESS), this)
    , m_p2pConn(nullptr)
    , m_adaptor(this)
{
    qDebug() << __PRETTY_FUNCTION__ << AUTHD_DBUS_P2P_ADDRESS;

    if (!connect(&m_qdbusServer, &QDBusServer::newConnection,
                 this, &Authd::handleNewConnection)) {
        qDebug() << "connection of qDBusServer::newConnection signal failed";
    }
}

Authd::~Authd()
{
    qDebug() << __PRETTY_FUNCTION__ << m_p2pConn;
    delete m_p2pConn;
}

// Wait until connection is authenticated: it is done concurrently to the
// emitting of the newConnection signal
void Authd::connWait(const QDBusConnection &c)
{
    auto i = static_cast<DBusConnection *>(c.internalPointer());
    while (dbus_connection_get_is_connected(i) &&
           !dbus_connection_get_is_authenticated(i)) {
        qDebug() << __PRETTY_FUNCTION__;
        QThread::usleep(100);
    }
}

unsigned long Authd::connPid(const QDBusConnection &c)
{
    auto i = static_cast<DBusConnection *>(c.internalPointer());
    unsigned long pid = 0;
    if (dbus_connection_get_unix_process_id(i, &pid)) {
        return pid;
    }

    return 0;
}

unsigned long Authd::connUid(const QDBusConnection &c)
{
    auto i = static_cast<DBusConnection *>(c.internalPointer());
    unsigned long uid = -1;
    if (dbus_connection_get_unix_user(i, &uid)) {
        return uid;
    }

    return -1;
}

void Authd::handleNewConnection(const QDBusConnection &connection)
{
    QDBusConnection c(connection);

    connWait(c);

    AuthdWatcher *w = new AuthdWatcher(this);
    qDebug() << __PRETTY_FUNCTION__
             << c.name()
             << &c
             << w
             << "pid" << connPid(c)
             << "uid" << connUid(c);

    bool r = c.registerObject(QStringLiteral(AUTHD_DBUS_OBJECT_PATH), this);
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
