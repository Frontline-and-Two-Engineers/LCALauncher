import QtQuick
import QtQuick.Controls

Button {
    id: root

    property color accentColor: "#00E5FF"
    property color textColor: "#FFFFFF"
    property bool isPrimary: false
    property string iconSource: ""
    property color iconColor: root.textColor
    property real iconSize: 16

    font.family: "RazerF5"
    font.pixelSize: 14
    font.bold: true

    implicitHeight: 44
    implicitWidth: 140

    contentItem: Item {
        implicitWidth: contentRow.implicitWidth
        implicitHeight: contentRow.implicitHeight

        Row {
            id: contentRow
            anchors.centerIn: parent
            spacing: 8

            LCAIcon {
                visible: root.iconSource !== ""
                source: root.iconSource
                size: root.iconSize
                color: root.iconColor
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                visible: root.text !== ""
                text: root.text
                font.family: root.font.family
                font.pixelSize: root.font.pixelSize
                font.bold: root.font.bold
                font.italic: root.font.italic
                color: root.textColor
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    background: Rectangle {
        id: bg
        radius: 8
        color: {
            if (!root.enabled) return "#252836"
            if (root.isPrimary) {
                return root.down ? Qt.darker(root.accentColor, 1.3) :
                       root.hovered ? Qt.lighter(root.accentColor, 1.15) : root.accentColor
            } else {
                return root.down ? "#991D2436" :
                       root.hovered ? "#CC263148" : "#80161C2A"
            }
        }

        border.color: root.isPrimary ? "transparent" : (root.hovered ? root.accentColor : "#33FFFFFF")
        border.width: 1

        Behavior on color { ColorAnimation { duration: 150 } }
        Behavior on border.color { ColorAnimation { duration: 150 } }
    }
}
