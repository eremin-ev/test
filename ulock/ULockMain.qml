/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0
import org.test.ulock 1.0

Page {
    id: mainPageId

    Column {
        id: content
        width: parent.width

        PageHeader {
            title: "Main Page"
        }

        Item { width: 1; height: Theme.paddingLarge }

        SilicaListView {
            width: parent.width
            height: contentHeight
            spacing: 10

            model: authType

            delegate: BackgroundItem {
                width: parent.width
                contentHeight: Theme.itemSizeSmall

                onClicked: {
                    console.log("clicked ", name, "type", type)
                    ulockId.authenticate(type)
                }

                Label {
                    text: name
                    anchors.centerIn: parent
                    color: highlighted ? Theme.highlightColor : Theme.primaryColor
                }
            }
        }
    }

    AuthType {
        id: authType
    }
}
