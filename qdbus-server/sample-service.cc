/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include "sample-service.h"

#include <QDebug>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QFile>
#include <QThread>

#include <dbus/dbus.h>

#include <pwd.h>

SampleServiceAdaptor::SampleServiceAdaptor(SampleService *parent)
    : QDBusAbstractAdaptor(parent)
    , m_sampleService(parent)
{
}

bool SampleServiceAdaptor::Hello(QMap<QString, QVariant> params)
{
    return m_sampleService->handleHello(params);
}

SampleService::SampleService(QObject *parent)
    : QObject(parent)
    , m_p2pServer(QStringLiteral(SAMPLE_DBUS_P2P_ADDRESS), this)
    , m_adaptor(this)
{
    qDebug() << __PRETTY_FUNCTION__
             << "isAnonymousAuthenticationAllowed"
             << m_p2pServer.isAnonymousAuthenticationAllowed();

    m_p2pServer.setAnonymousAuthenticationAllowed(true);

    if (!connect(&m_p2pServer, &QDBusServer::newConnection,
                 this, &SampleService::handleNewConnection)) {
        qDebug() << "connection of qDBusServer::newConnection signal failed";
    }
}

SampleService::~SampleService()
{
    qDebug() << __PRETTY_FUNCTION__ << this;
}

bool SampleService::isPeerToPeer() const
{
    return connection().baseService().isEmpty();
}

bool SampleService::handleHello(QMap<QString, QVariant> params)
{
    if (!isPeerToPeer())
        return false;

    uid_t uid = getConnUid(connection());
    uint pid = getConnPid(connection());

    struct passwd *pwd = getpwuid(uid);
    if (!pwd) {
        qWarning() << __func__ << "No such user" << uid;
        return false;
    }

    qDebug() << __PRETTY_FUNCTION__
             << "uid" << uid
             << "pid" << pid
             << "message-iface" << message().interface()
             << "connection-iface" << connection().interface()
             << "user" << pwd->pw_name;

    return true;
}

void SampleService::waitConnAuthenticated(const QDBusConnection &c)
{
    auto i = static_cast<DBusConnection *>(c.internalPointer());
    while (dbus_connection_get_is_connected(i) &&
           !dbus_connection_get_is_authenticated(i)) {
        qDebug() << __PRETTY_FUNCTION__;
        QThread::usleep(100);
    }
}

uint SampleService::getConnPid(const QDBusConnection &c)
{
    auto i = static_cast<DBusConnection *>(c.internalPointer());
    unsigned long pid = 0;
    if (dbus_connection_get_unix_process_id(i, &pid)) {
        return static_cast<uint>(pid);
    }

    return 0;
}

uint SampleService::getConnUid(const QDBusConnection &c)
{
    auto i = static_cast<DBusConnection *>(c.internalPointer());
    unsigned long uid = -1;
    if (dbus_connection_get_unix_user(i, &uid)) {
        return static_cast<uint>(uid);
    }

    return -1;
}

void SampleService::handleDisconnected()
{
    qDebug() << __PRETTY_FUNCTION__ << this;

    auto l = m_connections.find(connection().name());
    if (l == m_connections.end()) {
        qDebug() << __func__ << "FIXME cannot find connection in the list"
                 << connection().name();
    } else {
        m_connections.erase(l);
    }
}

void SampleService::handleNewConnection(const QDBusConnection &c)
{
    QDBusConnection connection(c);

    waitConnAuthenticated(connection);

    bool r = connection.registerObject(QStringLiteral(SAMPLE_DBUS_OBJECT_PATH), this);
    if (!r) {
        qDebug() << __PRETTY_FUNCTION__ << "Cannot registerObject SampleService" << this;
    }

    if (!connection.connect(QString(),
                            QStringLiteral("/org/freedesktop/DBus/Local"),
                            QStringLiteral("org.freedesktop.DBus.Local"),
                            QStringLiteral("Disconnected"),
                            this,
                            SLOT(handleDisconnected()))) {
        qDebug("Failed to connect to disconnect signal");
    }

    uid_t uid = getConnUid(connection);
    pid_t pid = getConnPid(connection);

    qDebug() << __PRETTY_FUNCTION__ << connection.name()
             << "uid" << uid
             << "pid" << pid;

    m_connections.insert(connection.name(), pid);
}

#include "sample-service.moc"
