/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#ifndef UDISKS2_H
#define UDISKS2_H

#include <QObject>
#include <QList>

#include <QDebug>

#define UDISKS2_SERVICE                 "org.freedesktop.UDisks2"
#define UDISKS2_FILESYSTEM_INTERFACE    UDISKS2_SERVICE".Filesystem"
#define UDISKS2_MANAGER_INTERFACE       UDISKS2_SERVICE".Manager"
#define UDISKS2_BLOCK_INTERFACE         UDISKS2_SERVICE".Block"

#define DBUS_OBJECT_PROPERTIES_INTERFACE "org.freedesktop.DBus.Properties"
#define DBUS_GET_ALL                     "GetAll"

#define UDISKS2_BLOCK_SETLABEL          "SetLabel"
#define UDISKS2_FILESYSTEM_MOUNT        "Mount"
#define UDISKS2_FILESYSTEM_UNMOUNT      "Unmount"

class UDisks2 : public QObject
{
    Q_OBJECT
public:
    struct BlockDevice {
        QString objectPath;
        QString fsType;
    };

    explicit UDisks2(QObject *parent = nullptr);
    ~UDisks2();

    void dump() const;
    const BlockDevice find(const QString &devicePath) const;
    void mount(const QString &devicePath,
               const QString &method,
               const QString &deviceObjectPath,
               const QString &fsType);
    //void umount(const QString &devicePath);
    void setLabel(const QString &devicePath,
                  const QString &deviceObjectPath,
                  const QString &label);
    void getProperties(const QString &path);

    void startTestSetLabel(const QString &blockDev, const QString &label);

    QString m_user;
    QString m_group;

signals:
    void initDone();
    void labelSet();
    void unmounted();

private:
    QMap<QString, BlockDevice> m_blockDevices;
};


#endif  /* UDISKS2_H */
