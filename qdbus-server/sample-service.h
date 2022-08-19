/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#ifndef SAMPLE_SERVICE_H
#define SAMPLE_SERVICE_H

#include "dbus-address.h"

#include <QObject>

#include <QDBusAbstractAdaptor>
#include <QDBusContext>
#include <QDBusConnection>
#include <QDBusServer>

class SampleService;

class SampleServiceAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", SAMPLE_DBUS_INTERFACE)

public:
    explicit SampleServiceAdaptor(SampleService *parent);

signals:
   void StateChanged(int state);

public slots:
    bool Hello(QMap<QString, QVariant> params);

private:
    SampleService *m_sampleService;
};

class SampleService : public QObject,
                      public QDBusContext {
    Q_OBJECT

public:
    explicit SampleService(QObject *parent = nullptr);
    ~SampleService();

public slots:
    bool handleHello(QMap<QString, QVariant> params);
    void handleDisconnected();

signals:

private:
    void waitConnAuthenticated(const QDBusConnection &c);
    uint getConnUid(const QDBusConnection &c);
    uint getConnPid(const QDBusConnection &c);
    void handleNewConnection(const QDBusConnection &connection);
    bool isPeerToPeer() const;

    QDBusServer m_p2pServer;
    SampleServiceAdaptor m_adaptor;

    typedef QHash<QString, int> ConnectionList;
    ConnectionList m_connections;
};

#endif // SAMPLE_SERVICE_H
