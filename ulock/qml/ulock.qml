/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0
import org.test.ulock 1.0

ApplicationWindow {
    id: page

    initialPage: Qt.resolvedUrl("qrc:///qml/ULockMain.qml")

    Connections {
        target: ulockId

        onInputRequested: {
            console.log('onInputRequested: msg', msg, 'type', type)
            pageStack.push(Qt.resolvedUrl("qrc:///qml/ULockPassword.qml"))
        }
    }

    ULock {
        id: ulockId
    }
}
