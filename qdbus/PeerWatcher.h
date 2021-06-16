#ifndef PEER_WATCHER_H
#define PEER_WATCHER_H

#include <QObject>
#include <QDebug>

class QDBusConnection;
struct PeerWatcher : public QObject {
    Q_OBJECT

public:
    PeerWatcher(QObject *parent)
        : QObject(parent)
    {
        qDebug() << __PRETTY_FUNCTION__ << this;
    }

    ~PeerWatcher()
    {
        qDebug() << __PRETTY_FUNCTION__ << this;
    }

public slots:
    void handleDisconnected()
    {
        deleteLater();
        qDebug() << __PRETTY_FUNCTION__ << this;
    }

private:
    //const QDBusConnection *m_conn;
};

#endif  /* PEER_WATCHER_H */
