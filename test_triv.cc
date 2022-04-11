/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 *
 * Build this way to get a warning:
 *
 *  $ make CXXFLAGS='-DUSE_NON_POD' test_triv
 *
 *  $ make test_triv 
 *  g++ -Wall -Wextra -Werror -O0 -g -std=c++17 -fPIC -I. \
 *		-I/usr/include/qt5/QtCore -I/usr/include/qt5  \
 *  	test_triv.cc  -lm -lQt5Core -o test_triv
 *
 *  In file included from /usr/include/qt5/QtCore/qdebug.h:44,
 *                   from /usr/include/qt5/QtCore/QDebug:1,
 *                   from test_triv.cc:16:
 * /usr/include/qt5/QtCore/qvector.h: In instantiation of ‘QVector<T>::iterator
 *			QVector<T>::erase(QVector<T>::iterator, QVector<T>::iterator)
 *			[with T = Info; QVector<T>::iterator = Info*]’:
 * /usr/include/qt5/QtCore/qvector.h:448:3: required from ‘void QVector<T>::remove(int) [with T = Info]’
 * test_triv.cc:58:22:   required from here
 * /usr/include/qt5/QtCore/qvector.h:757:20: error:
 *			‘void* memmove(void*, const void*, size_t)’ writing to an object of
 *			type ‘struct Info’ with no trivial copy-assignment; use copy-assignment or
 *			copy-initialization instead [-Werror=class-memaccess]
 *            memmove(abegin, aend, (d->size - itemsToErase - itemsUntouched) * sizeof(T));
 *            ~~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * test_triv.cc:23:8: note: ‘struct Info’ declared here
 * struct Info {
 *        ^~~~
 * cc1plus: all warnings being treated as errors
 * make: *** [<builtin>: test_triv] Error 1
 *
 * See also:
 *
 * [1] Invalid declaration of ProString as a Q_MOVABLE_TYPE?
 *     https://bugreports.qt.io/browse/QTCREATORBUG-20976
 *
 */

#include <QDebug>
#include <QVector>

#ifdef USE_NON_POD
# include <QString>
#endif

struct Info {
#ifdef USE_NON_POD
    QString a;
    QString b;
#else
    int a;
    int b;
#endif
};

Q_DECLARE_TYPEINFO(Info, Q_PRIMITIVE_TYPE);

void test1()
{
    QVector<Info> info;

    qDebug() << __func__ << "info.count()" << info.count();

#ifdef USE_NON_POD
    info.append(Info{QStringLiteral("/org/test/serviceA"), QStringLiteral("foo")});
    info.append(Info{QStringLiteral("/org/test/serviceB"), QStringLiteral("bar")});
    info.append(Info{QStringLiteral("/org/test/serviceC"), QStringLiteral("baz")});
#else
    info.append(Info{1, 2});
    info.append(Info{3, 4});
    info.append(Info{5, 6});
#endif

    qDebug() << __func__ << "info.count()" << info.count();

    for (const auto &i : info) {
        qDebug() << __func__ << "info" << i.a << i.b;
    }

    if (!info.isEmpty()) {
        info.remove(0);
    }

    qDebug() << __func__ << "info.count()" << info.count();

    for (const auto &i : info) {
        qDebug() << __func__ << "info" << i.a << i.b;
    }

    while (info.count()) {
        info.removeLast();
    }

    info.clear();
    info.squeeze();
}

int main()
{
    test1();

    return 0;
}
