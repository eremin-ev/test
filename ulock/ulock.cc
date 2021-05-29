/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <QDebug>

#include "ulock.h"

ULock::ULock(QObject *parent)
    : QObject(parent)
    , m_login(QStringLiteral(""))
    , m_passwd(QStringLiteral(""))
    , m_state(AUTH_FAILED)
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

void ULock::authenticate(ULockType ulockType)
{
    qDebug() << __PRETTY_FUNCTION__ << ulockType;
    emit inputRequested(QStringLiteral("Password:"), ReqType::Password);

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

    m_state = doAuth(m_login, m_passwd)
                ? AUTH_SUCCESS
                : AUTH_FAILED;
}

bool ULock::doAuth(const QString &login, const QString &password)
{
    return login == QStringLiteral("1") && password == QStringLiteral("2");
}
