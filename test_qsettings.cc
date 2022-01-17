/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <QDebug>
#include <QDir>
#include <QSettings>

struct group1 {
    int settingA;
    int settingB;
};

void testRead(struct group1 *g1, const QString &path)
{
    QDir confDir(path);
    if (!confDir.exists()) {
        qDebug() << __func__ << "No such directory" << path;
        return;
    }

    for (const QString &fileName : confDir.entryList(QDir::Files, QDir::Name)) {
        qDebug() << __func__ << "setting_a" << g1->settingA << "setting_b" << g1->settingB;

        const QSettings settings(confDir.absoluteFilePath(fileName), QSettings::IniFormat);

        QVariant value = settings.value(QStringLiteral("group1/setting_a"));
        if (value.isValid()) {
             g1->settingA = value.toInt();
        }

        value = settings.value(QStringLiteral("group1/setting_b"));
        if (value.isValid()) {
            g1->settingB = value.toInt();
        }
    }
}

int main(void)
{
    const QString path = QStringLiteral("/tmp/qsettings.d");

    struct group1 g1 = {
        .settingA = 21,
        .settingB = 2001,
    };

    testRead(&g1, path);

    qDebug() << __func__ << "setting_a" << g1.settingA << "setting_b" << g1.settingB;

    return 0;
}
