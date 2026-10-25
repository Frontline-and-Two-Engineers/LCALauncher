import QtQuick
import QtQuick.Controls

Item {
    id: root

    property real value: 0.0 // 0.0 to 1.0
    property string statusText: ""
    property string subText: ""

    implicitHeight: 36
    implicitWidth: 300

    Column {
        anchors.fill: parent
        spacing: 6

        Row {
            width: parent.width
            Text {
                text: root.statusText
                color: "#E0E0E0"
                font.pixelSize: 12
                font.bold: true
                elide: Text.ElideRight
                width: parent.width - percentText.width - 10
            }
            Text {
                id: percentText
                text: Math.round(root.value * 100) + "%"
                color: "#00E5FF"
                font.pixelSize: 12
                font.bold: true
            }
        }

        Rectangle {
            id: track
            width: parent.width
            height: 6
            radius: 3
            color: "#1E2230"
            clip: true

            Rectangle {
                id: fill
                height: parent.height
                width: Math.max(0, Math.min(parent.width, parent.width * root.value))
                radius: 3
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: "#00E5FF" }
                    GradientStop { position: 1.0; color: "#00FFA3" }
                }

                Behavior on width {
                    NumberAnimation { duration: 120; easing.type: Easing.OutQuad }
                }
            }
        }
    }
}
