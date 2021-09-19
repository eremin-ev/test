/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: passwordPageId

    Column {
        id: content
        width: parent.width

        PageHeader {
            title: "Password Page"
        }

        Item { width: 1; height: Theme.paddingLarge }

        TextField {
            id: loginFieldId
            label: "Login bottom label"
            placeholderText: "Please enter your login"
            onActiveFocusChanged: {
                    if (activeFocus)
                            selectAll()
            }
            EnterKey.iconSource: "image://theme/icon-m-enter-next"
            EnterKey.onClicked: {
                ulockId.login = text
                passwordField.focus = true
            }
        }

        Item { width: 1; height: Theme.paddingLarge }

        PasswordField {
            id: passwordField
            text: ulockId.passwd
            label: "Password bottom label"
            placeholderText: "Please enter your password"
            onActiveFocusChanged: {
                    if (activeFocus)
                            selectAll()
            }
            EnterKey.iconSource: "image://theme/icon-m-enter-accept"
            EnterKey.onClicked: {
                ulockId.passwd = text
                focus = false
            }
        }
    }
}
