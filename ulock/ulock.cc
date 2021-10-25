/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <QDebug>
#include <QString>

#include "ulock.h"
#include "authd-addr.h"

static QDBusInterface &getAuthdIface()
{
    const QDBusConnection c =
#if (AUTHD_DBUS_TYPE == AUTHD_DBUS_TYPE_SYSTEM)
        QDBusConnection::systemBus();
#elif  (AUTHD_DBUS_TYPE == AUTHD_DBUS_TYPE_SESSION)
        QDBusConnection::sessionBus();
#elif (AUTHD_DBUS_TYPE == AUTHD_DBUS_TYPE_P2P)
        QDBusConnection::connectToPeer(QStringLiteral(AUTHD_DBUS_P2P_ADDRESS),
                                       QStringLiteral("authd_p2p_name"));
#else
    #error "Wrong AUTHD_DBUS_TYPE: " #AUTHD_DBUS_TYPE
#endif

    if (!c.isConnected()) {
        // FIXME: handle this gracefully
        qCritical() << "Cannot connect to the D-Bus";
        exit(EXIT_FAILURE);
    }

    static
    QDBusInterface iface(QStringLiteral(AUTHD_DBUS_SERVICE),
                         QStringLiteral(AUTHD_DBUS_OBJECT_PATH),
                         QStringLiteral(AUTHD_DBUS_INTERFACE),
                         c);

    return iface;
}

ULock::ULock(QObject *parent)
    : QObject(parent)
    , m_iface(getAuthdIface())
    , m_login(QStringLiteral(""))
    , m_passwd(QStringLiteral(""))
    , m_state(AUTH_FAILED)
{
    qDebug() << __PRETTY_FUNCTION__ << this << "iface.isValid" << m_iface.isValid();
}

ULock::~ULock()
{
    qDebug() << __PRETTY_FUNCTION__ << this;
}

const QString ULock::login()
{
    return m_login;
}

const QString ULock::passwd()
{
    return m_passwd;
}

void ULock::setLogin(const QString &login)
{
    if (m_login != login) {
        m_login = login;
        emit loginChanged();
        qDebug() << __PRETTY_FUNCTION__ << "ULock login:" << m_login;
    }
}

void ULock::setPasswd(const QString &passwd)
{
    if (m_passwd != passwd) {
        m_passwd = passwd;
        emit passwdChanged();
        qDebug() << __PRETTY_FUNCTION__ << "ULock passwd:" << m_passwd;
    }
}

void ULock::authenticate(const QString &authType)
{
    //emit inputRequested(QStringLiteral("Password:"), ReqType::Password);

    qDebug() << __PRETTY_FUNCTION__ << "iface.isValid" << m_iface.isValid();

    QDBusMessage reply = m_iface.call("begin", authType);

    //QString mathod
    //qDebug() << __PRETTY_FUNCTION__ << "authType" << authType << "method:" << method;

    qDebug() << __PRETTY_FUNCTION__ << "authType" << authType << "method reply:" << reply;

#if 0
    if (m_login.isEmpty() || m_passwd.isEmpty()) {
        qDebug() << __PRETTY_FUNCTION__ << "ULock: empty login or passwd";
        return;
    }

    if (m_state == AUTH_SUCCESS) {
        qDebug() << __PRETTY_FUNCTION__ << "ULock: already authenticated";
        return;
    }
#endif

    //m_state = doAuth(m_login, m_passwd)
    //            ? AUTH_SUCCESS
    //            : AUTH_FAILED;
}

bool ULock::doAuth(const QString &login, const QString &password)
{
    return login == QStringLiteral("1") && password == QStringLiteral("2");
}
