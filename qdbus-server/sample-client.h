/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#ifndef SAMPLE_CLIENT_H
#define SAMPLE_CLIENT_H

#include <QObject>
#include <QVariant>
#include <QDBusInterface>
#include <QDBusConnection>
#include <QDBusReply>

class QDBusPendingCallWatcher;

class SampleClient : public QObject {
    Q_OBJECT

public:
    explicit SampleClient(QObject *parent = nullptr);
    ~SampleClient();

public slots:
    void handleDisconnected();

signals:

private slots:

private:
    void setupConnection();

    QDBusConnection m_connection;
    QDBusInterface m_iface;
};

#endif  // SAMPLE_CLIENT_H
