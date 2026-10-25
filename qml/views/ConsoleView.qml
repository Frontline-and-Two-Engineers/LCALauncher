import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: root

    Rectangle {
        anchors.fill: parent
        color: "#12141D"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 30
            spacing: 16

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Text {
                    text: "Консоль логов игры"
                    color: "#FFFFFF"
                    font.pixelSize: 22
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                LCAButton {
                    text: "Очистить"
                    Layout.preferredWidth: 110
                    Layout.preferredHeight: 36
                    onClicked: launcherBridge.clearLogs()
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 10
                color: "#0D0E15"
                border.color: "#212638"
                clip: true

                ListView {
                    id: logListView
                    anchors.fill: parent
                    anchors.margins: 16
                    model: launcherBridge.consoleLogs
                    spacing: 4

                    delegate: Text {
                        width: logListView.width
                        text: modelData
                        font.family: "JetBrains Mono, Menlo, Consolas, monospace"
                        font.pixelSize: 12
                        wrapMode: Text.WrapAnywhere
                        color: {
                            if (modelData.indexOf("[ERR]") !== -1 || modelData.indexOf("Exception") !== -1 || modelData.indexOf("ERROR") !== -1)
                                return "#FF5252"
                            if (modelData.indexOf("[WARN]") !== -1)
                                return "#FFD740"
                            if (modelData.indexOf("[Launcher]") !== -1)
                                return "#00E5FF"
                            return "#C8D0E0"
                        }
                    }

                    onCountChanged: {
                        logListView.positionViewAtEnd()
                    }
                }
            }
        }
    }
}
