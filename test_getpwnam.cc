/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <QString>
#include <QDebug>
#include <QDir>

namespace {

const auto f_frsize = 4096;
const auto GB = 1000000000U;

}

void test1()
{
    struct passwd pwd, *pwd_r;
    const int buflen = 512;
    char buf[buflen];

    int r = getpwuid_r(getuid(), &pwd, buf, buflen, &pwd_r);
    if (!pwd_r) {
        printf("cannot getpwuid_r: %d %s\n", r, strerror(errno));
        exit(EXIT_FAILURE);
    }

    printf("r %d, dir '%s'\n", r, pwd.pw_dir);

    QDir d(pwd.pw_dir);
    QString s{d.dirName()};

    qDebug() << d;
    qDebug() << s;
}

void test2()
{
    qDebug() << QDir::homePath();
    qDebug() << QDir::home();
    qDebug() << QDir::home().dirName();
}

QString test3()
{
    QFileInfo path{QDir::homePath()};

    qDebug() << path.absolutePath();

    return QString("%1/.env/%2/file.conf").arg(path.absolutePath()).arg(getuid());
}

void test_overflow(uint q)
{
    quint64 val1 = static_cast<quint64>(q) * GB / f_frsize;
    quint64 val2 = q * GB / f_frsize;

    qDebug() << val1 << val1 * f_frsize;
    qDebug() << val2 << val2 * f_frsize;
}

int main()
{
    test1();
    test2();

    qDebug() << test3();

    test_overflow(7);

    return EXIT_SUCCESS;
}
