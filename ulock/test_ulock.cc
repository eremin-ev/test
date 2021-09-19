/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <QObject>
#include <QtQuick/QQuickView>
#include <QGuiApplication>

#include "ulock.h"
#include "auth-type.h"
#include "sighandler.h"

int main(int argc, char **argv)
{
    QGuiApplication a(argc, argv);

    SigHandler h(&a, &a);

    qmlRegisterType<ULock>("org.test.ulock", 1, 0, "ULock");
    qmlRegisterType<AuthTypeModel>("org.test.ulock", 1, 0, "AuthTypeModel");

    QQuickView view;
    view.setSource(QUrl("qrc:/qml/ulock.qml"));
    view.showFullScreen();

    h.setupUnixSignalHandlers();

    QObject::connect(&h, &SigHandler::unixHupArrived, &a, &QCoreApplication::quit, Qt::QueuedConnection);
    QObject::connect(&h, &SigHandler::unixTermArrived, &a, &QCoreApplication::quit, Qt::QueuedConnection);
    QObject::connect(&h, &SigHandler::unixIntArrived, &a, &QCoreApplication::quit, Qt::QueuedConnection);

    return a.exec();
}
