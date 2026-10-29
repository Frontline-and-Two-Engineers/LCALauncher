import QtQuick
import QtQuick.Effects

Item {
    id: root

    property string source: ""
    property color color: "#FFFFFF"
    property real size: 16
    property bool autoColor: true

    implicitWidth: size
    implicitHeight: size
    width: size
    height: size

    Image {
        id: iconImg
        anchors.fill: parent
        source: root.source
        sourceSize.width: Math.max(1, Math.round(root.width * Screen.devicePixelRatio))
        sourceSize.height: Math.max(1, Math.round(root.height * Screen.devicePixelRatio))
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: true
        visible: !root.autoColor
    }

    MultiEffect {
        anchors.fill: iconImg
        source: iconImg
        colorization: 1.0
        colorizationColor: root.color
        visible: root.autoColor && iconImg.status === Image.Ready
    }
}
