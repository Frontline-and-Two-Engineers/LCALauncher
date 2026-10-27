import QtQuick
import QtQuick.Controls

TextField {
    id: root

    property string label: ""

    color: "#FFFFFF"
    placeholderTextColor: "#60677D"
    font.pixelSize: 14
    font.family: "Segoe UI, Inter, Roboto, sans-serif"
    implicitHeight: 44
    selectByMouse: true

    background: Rectangle {
        radius: 8
        color: root.activeFocus ? "#B3161C2C" : "#800F1320"
        border.color: root.activeFocus ? "#00E5FF" : "#33FFFFFF"
        border.width: 1

        Behavior on border.color { ColorAnimation { duration: 150 } }
        Behavior on color { ColorAnimation { duration: 150 } }
    }
}
