/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#ifndef ULOCK_H
#define ULOCK_H

#include <QObject>

class ULock : public QObject {
	Q_OBJECT

	Q_PROPERTY(QString login READ login WRITE setLogin NOTIFY loginChanged)
	Q_PROPERTY(QString passwd READ passwd WRITE setPasswd NOTIFY passwdChanged)

	enum State {
		AUTH_FAILED =	0,
		AUTH_SUCCESS =	1,
	};

public:
    enum ULockType {
        ULockLogin = 0,
        ULockAuthenticate,
    };
    Q_ENUM(ULockType)

    enum ReqType {
        Password = 0,
    };
    Q_ENUM(ReqType)

	explicit ULock(QObject *parent = nullptr);

	const QString login();
	const QString passwd();
	void setLogin(const QString &login);
	void setPasswd(const QString &passwd);

signals:
    void inputRequested(const QString &msg, ReqType type = ReqType::Password);
	void loginChanged();
	void passwdChanged();

public slots:
    void authenticate(ULockType ulockType);

private:
    bool doAuth(const QString &login, const QString &password);
	QString m_login;
	QString m_passwd;
	int m_state;
};

#endif  // ULOCK_H
