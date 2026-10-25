import QtQuick
import QtQuick.Controls

Button {
    id: root

    property color accentColor: "#00E5FF"
    property color textColor: "#FFFFFF"
    property bool isPrimary: false

    implicitHeight: 44
    implicitWidth: 140

    contentItem: Text {
        text: root.text
        font.pixelSize: 14
        font.bold: true
        font.family: "Segoe UI, Inter, Roboto, sans-serif"
        color: root.textColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
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
                return root.down ? "#222533" :
                       root.hovered ? "#2A2E40" : "#1F222E"
            }
        }

        border.color: root.isPrimary ? "transparent" : (root.hovered ? root.accentColor : "#31364A")
        border.width: 1

        Behavior on color { ColorAnimation { duration: 150 } }
        Behavior on border.color { ColorAnimation { duration: 150 } }
    }
}
