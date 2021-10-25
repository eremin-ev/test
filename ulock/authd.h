/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#ifndef AUTHD_H
#define AUTHD_H

#include <QObject>
#include <QDBusServer>
#include <QDBusAbstractAdaptor>

#include "authd-addr.h"

/*
 * AuthdWatcher
 */
struct AuthdWatcher : public QObject {
    Q_OBJECT

public:
    AuthdWatcher(QObject *parent);
    ~AuthdWatcher();

public slots:
    void handleDisconnected();

private:
    //const QDBusConnection *m_conn;
};

/*
 * Adaptor
 */
class Authd;
class AuthdAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", AUTHD_DBUS_INTERFACE)

public:
    explicit AuthdAdaptor(Authd *authd);

public slots:
    double multiply(double factor0, double factor2);
    double divide(double divident, double divisor);
    int quit();

    QString begin(const QString &authType);

signals:
	void newProduct(double product);
	void newQuotient(double quotient);
	void servQuit(int retcode);

private:
    Authd *m_authd;
};

/*
 * Authd
 */
class Authd : public QObject {
    Q_OBJECT

public:
	Authd(QObject *parent);
	virtual ~Authd();

    void handleNewConnection(const QDBusConnection &connection);

private:
    void connWait(const QDBusConnection &connection);
    unsigned long connPid(const QDBusConnection &connection);
    unsigned long connUid(const QDBusConnection &connection);

    QDBusServer m_qdbusServer;
    QDBusConnection *m_p2pConn;
    AuthdAdaptor m_adaptor;
};

#endif  /* AUTHD_H */
