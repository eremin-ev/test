/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include "sample-client.h"
#include "dbus-address.h"

#include <QDebug>
#include <QString>
#include <QDBusConnection>

#include <unistd.h>

SampleClient::SampleClient(QObject *parent)
    : QObject(parent)
    , m_connection(QDBusConnection::connectToPeer(QStringLiteral(SAMPLE_DBUS_P2P_ADDRESS),
                                                  QStringLiteral("sample_p2p_service")))
    , m_iface(QStringLiteral(SAMPLE_DBUS_SERVICE),
              QStringLiteral(SAMPLE_DBUS_OBJECT_PATH),
              QStringLiteral(SAMPLE_DBUS_INTERFACE),
              m_connection)
{
    setupConnection();

    qDebug() << __func__ << this
             << "pid" << getpid()
             << "connection.isConnected" << m_connection.isConnected()
             << "iface.isValid" << m_iface.isValid();

    QDBusReply<bool> reply = m_iface.call(QStringLiteral("Hello"),
                                          QVariantMap{{QStringLiteral("First"),
                                                       QVariant::fromValue(QStringLiteral("123"))}});
    if (!reply.isValid()) {
        qDebug() << __func__ << "reply isError:" << reply.error().message();
    } else {
        qDebug() << __func__ << reply;
    }
}

SampleClient::~SampleClient()
{
    qDebug() << __PRETTY_FUNCTION__ << this;
}

void SampleClient::handleDisconnected()
{
    qDebug() << __func__ << "Disconnected from" << m_connection.name();
}

void SampleClient::setupConnection()
{
    if (!m_connection.isConnected()) {
        qCritical() << "Cannot connect to the D-Bus"
                    << qPrintable(m_connection.name())
                    << qPrintable(m_connection.lastError().message());
        exit(EXIT_FAILURE);
    }

    qDebug() << "Connected to"
             << qPrintable(m_connection.name());

    if (!m_connection.connect(QString(),
                              QStringLiteral("/org/freedesktop/DBus/Local"),
                              QStringLiteral("org.freedesktop.DBus.Local"),
                              QStringLiteral("Disconnected"),
                              this,
                              SLOT(handleDisconnected()))) {
        qWarning("Failed to connect to 'Disconnected' signal");
    }
}
