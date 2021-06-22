/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>

#include <QtDBus>

#include "UDisks2.h"

UDisks2::UDisks2(QObject *parent)
    : QObject(parent)
{
    //connect(this, &UDisks2::initDone, this, &UDisks2::startTestSetLabel);
    connect(this, &UDisks2::labelSet, parent, &QCoreApplication::quit, Qt::QueuedConnection);

    const QDBusConnection &c = QDBusConnection::systemBus();

    if (!c.isConnected()) {
        qCritical() << "Cannot connect to the D-Bus system bus";
        exit(EXIT_FAILURE);
    }

    QDBusInterface iface(QStringLiteral(UDISKS2_SERVICE),
                         QStringLiteral("/org/freedesktop/UDisks2/Manager"),
                         QStringLiteral(UDISKS2_MANAGER_INTERFACE),
                         c);

    if (!iface.isValid()) {
        qCritical() << "Cannot connect to " UDISKS2_MANAGER_INTERFACE;
        exit(EXIT_FAILURE);
    }

    const QMap<QString,QVariant> vm = {
        { QStringLiteral("FirstUnused"), QVariant(QStringLiteral("FirstValue")) },
        { QStringLiteral("SecondUnused"), QVariant(QStringLiteral("SecondValue")) },
    };

    QVariant arg = QVariant::fromValue(vm);
    //qDebug() << arg;

    // Synchronous call
    const QString &method = QStringLiteral("GetBlockDevices");
    QDBusMessage reply = iface.call(method, arg);

    if (reply.type() == QDBusMessage::ErrorMessage) {
        qDebug() << QStringLiteral("Failed to send") << method << qPrintable(reply.errorMessage());
        exit(EXIT_FAILURE);
    } else if (reply.type() != QDBusMessage::ReplyMessage) {
        qDebug("Bad reply type %d", reply.type());
        exit(EXIT_FAILURE);
    }

    //qDebug() << reply;

    // Expect array of object paths
    const QDBusArgument &ao = reply.arguments().at(0).value<QDBusArgument>();
    if (ao.currentType() != QDBusArgument::ArrayType) {
        fprintf(stderr, "ERROR: Unexpected argument type of EnumerateDevices call\n");
    }

    ao.beginArray();
    while (!ao.atEnd()) {
        const QString &path = qdbus_cast<QDBusObjectPath>(ao).path();
        //qDebug() << QStringLiteral("Path") << path;
        getProperties(path);
    }
    ao.endArray();
}

UDisks2::~UDisks2() = default;

void UDisks2::dump() const
{
    qDebug() << __PRETTY_FUNCTION__;
    for (auto i = m_blockDevices.constBegin(); i != m_blockDevices.constEnd(); ++i) {
        const BlockDevice &d = i.value();
        qDebug() << i.key() << d.objectPath << d.fsType;
    }
}

const UDisks2::BlockDevice UDisks2::find(const QString &devicePath) const
{
    const BlockDevice &d = m_blockDevices.value(devicePath);
    if (!d.objectPath.isNull()) {
        return d;
    }

    return {};
}

void UDisks2::mount(const QString &method,
                    const QString &blockDevice,
                    const QString &objectPath,
                    const QString &fsType)
{
    QVariantMap options;
    options.insert(QStringLiteral("fstype"), fsType);

    QVariantList args;
    args << options;

    if (blockDevice.isEmpty()) {
        qDebug() << "Cannot call" << method << "without device name";
        return;
    }

    QDBusInterface iface(UDISKS2_SERVICE,
                         objectPath,
                         UDISKS2_FILESYSTEM_INTERFACE,
                         QDBusConnection::systemBus());

    qDebug() << method << blockDevice << objectPath << args;

    QDBusMessage reply = iface.callWithArgumentList(QDBus::Block, method, args);

    if (reply.type() == QDBusMessage::ErrorMessage) {
        qDebug() << QStringLiteral("Failed to send") << method << qPrintable(reply.errorMessage());
        //exit(EXIT_FAILURE);
    } else if (reply.type() != QDBusMessage::ReplyMessage) {
        qDebug("Bad reply type %d", reply.type());
        //exit(EXIT_FAILURE);
    }

    qDebug() << reply;
}

void UDisks2::setLabel(const QString &blockDevice,
                       const QString &objectPath,
                       const QString &label)
{
    const QString method = QStringLiteral(UDISKS2_BLOCK_SETLABEL);
    if (blockDevice.isEmpty()) {
        qDebug() << "Cannot" << method << "without device name";
        return;
    }

    QDBusInterface iface(QStringLiteral(UDISKS2_SERVICE),
                         objectPath,
                         QStringLiteral(UDISKS2_FILESYSTEM_INTERFACE),
                         QDBusConnection::systemBus());

    QVariantList args;
    args << label;

    QVariantMap options;
    args << options;

    QDBusMessage reply = iface.callWithArgumentList(QDBus::Block, method, args);

    if (reply.type() == QDBusMessage::ErrorMessage) {
        qDebug() << QStringLiteral("Failed to send") << method << qPrintable(reply.errorMessage());
        exit(EXIT_FAILURE);
    } else if (reply.type() != QDBusMessage::ReplyMessage) {
        qDebug("Bad reply type %d", reply.type());
        exit(EXIT_FAILURE);
    }

    qDebug() << reply;
    emit labelSet();
}

template <typename Arg>
Arg demarshall(const QVariant &arg)
{
    if (arg.userType() == qMetaTypeId<QDBusArgument>()) {
        Arg out;
        arg.value<QDBusArgument>() >> out;
        return out;
    } else {
        return arg.value<Arg>();
    }
}

template <>
QDBusArgument demarshall<QDBusArgument>(const QVariant &arg)
{
    return arg.value<QDBusArgument>();
}

template <>
QVariant demarshall<QVariant>(const QVariant &arg)
{
    return arg.value<QDBusVariant>().variant();
}

void UDisks2::getProperties(const QString &path)
{
    const QDBusConnection &c = QDBusConnection::systemBus();
    QDBusInterface iface(QStringLiteral(UDISKS2_SERVICE),
                         path,
                         QStringLiteral(DBUS_OBJECT_PROPERTIES_INTERFACE),
                         c);

    const QString &ifaceStr = QStringLiteral(UDISKS2_BLOCK_INTERFACE);
    QDBusPendingCall pendingCall = iface.asyncCall(QStringLiteral(DBUS_GET_ALL), ifaceStr);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall);
    connect(watcher, &QDBusPendingCallWatcher::finished,
            [this, path, ifaceStr](QDBusPendingCallWatcher *watcher) {
        if (watcher->isValid() && watcher->isFinished()) {
            QDBusPendingReply<> reply = *watcher;
            QDBusMessage message = reply.reply();
            const QVariantMap &m = demarshall<QVariantMap>(message.arguments().at(0));
            const QVariant &deviceVar = m.value(QStringLiteral("Device"));
            const QVariant &typeVar = m.value(QStringLiteral("IdType"));
            if (deviceVar.isValid()) {
                const QString &deviceStr = deviceVar.toString();
                const QString &typeStr = typeVar.toString();
                if (!deviceStr.isNull()) {
                    //qDebug() << __PRETTY_FUNCTION__ << deviceStr << path;
                    BlockDevice d {
                        path,
                        typeStr,
                    };
                    m_blockDevices.insert(deviceStr, d);

                    //if (m_blockDevices are ready)
                    //  emit initDone();
                }
            } else {
                qDebug() << "No 'Device' for" << path;
            }
        } else {
            qDebug() << "Get properties failed" << path << "interface:" << ifaceStr << watcher->isValid() << watcher->isFinished() << watcher->isError();
            QDBusPendingReply<> reply = *watcher;
            QDBusMessage message = reply.reply();
            QDBusError error = watcher->error();
            qDebug() << "Error reading" << message.interface() << "properties:" << error.name() << error.message();
        }

        watcher->deleteLater();
    });
}

static int setUid(const char *user, const char *group)
{
    struct passwd *u = getpwnam(user);
    struct group *g = getgrnam(group);

    if (!u || !g) {
        qDebug() << "Cannot obtain user or group info";
        exit(EXIT_FAILURE);
    }

    if (-1 == setregid(g->gr_gid, g->gr_gid) && getgid() != g->gr_gid) {
        qDebug("Cannot setgid: %s", strerror(errno));
        return 0;
    }

    if (-1 == setreuid(u->pw_uid, u->pw_uid) && getuid() != u->pw_uid) {
        qDebug("Cannot setuid: %s", strerror(errno));
        return 0;
    }

    return 1;
}

void UDisks2::startTestSetLabel(const QString &blockDev, const QString &label)
{
#if 0
    dump();
#else
    BlockDevice d = find(blockDev);
    if (d.objectPath.isNull()) {
        qDebug() << "Cannot find dev" << blockDev;
        return;
    }

    qDebug() << "Ok, try to set label on" << blockDev << d.objectPath;
    if (setUid(m_user.toUtf8().constData(), m_group.toUtf8().constData())) {
        mount(QStringLiteral(UDISKS2_FILESYSTEM_UNMOUNT), blockDev, d.objectPath, d.fsType);
        setLabel(blockDev, d.objectPath, label);
        mount(QStringLiteral(UDISKS2_FILESYSTEM_MOUNT), blockDev, d.objectPath, d.fsType);
    }
#endif
}
