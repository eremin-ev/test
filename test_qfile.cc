/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <QDebug>
#include <QDir>
#include <QFile>

static QString outDir(const QString &user)
{
    //QString dir(QStandardPaths::displayName(QStandardPaths::ConfigLocation));
    return QString("tmp/%1/config").arg(user);
}

QFileDevice::FileError testWrite(const QString &user, const QString &data)
{
    QDir dir(outDir(user));
    dir.mkpath(QStringLiteral("."));

    qDebug() << __func__ << "Mkdir" << dir.path();

    QString path = QString("%1/qfile.conf").arg(dir.path());
    qDebug() << __func__ << "Writing to" << "path" << path;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Cannot open" << path << ":" << file.errorString();
        return file.error();
    }

    QFileDevice::FileError retval = QFileDevice::NoError;

    if (file.write(data.toUtf8()) == -1) {
        qDebug() << "Cannot write" << path << ":" << file.errorString();
        retval = file.error();
    }

    file.close();

    return retval;
}

QFileDevice::FileError testRead(QByteArray &contents, const QString &user)
{
    QDir dir(outDir(user));
    QString path = QString("%1/qfile.conf").arg(dir.path());
    qDebug() << __func__ << "Reading" << path;

    QFile file(path);
    if (!file.exists()) {
        qDebug() << "File not found" << path;
        return file.error();
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open" << path << ":" << file.errorString();
        return file.error();
    }

    contents = file.read(4096);

    QFileDevice::FileError retval = QFileDevice::NoError;
    if (contents.size() != file.size()) {
        qDebug() << "Cannot read" << path << ":" << file.errorString();
        retval = file.error();
    }

    file.close();

    return retval;
}

int main()
{
    QString user(QStringLiteral("user1"));
    QString data(QStringLiteral("data1 for user1"));

    testWrite(user, data);

    QByteArray contents;
    testRead(contents, user);

    if (data == contents) {
        qDebug() << "write/read data Ok";
    }

    return 0;
}
