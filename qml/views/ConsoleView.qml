import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: root

    Timer {
        id: copiedTimer
        interval: 2000
    }

    Rectangle {
        anchors.fill: parent
        color: "transparent"

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
                    text: copiedTimer.running ? "Скопировано!" : "Скопировать всё"
                    iconSource: copiedTimer.running 
                        ? "qrc:/qt/qml/LCALauncher/assets/icons/check.svg" 
                        : "qrc:/qt/qml/LCALauncher/assets/icons/copy.svg"
                    iconColor: copiedTimer.running ? "#00FFA3" : "#FFFFFF"
                    iconSize: 14
                    Layout.preferredWidth: 165
                    Layout.preferredHeight: 36
                    onClicked: {
                        launcherBridge.copyLogsToClipboard()
                        copiedTimer.restart()
                    }
                }

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
                radius: 14
                color: "#B30A0D15"
                border.color: "#2EFFFFFF"
                border.width: 1
                clip: true

                ListView {
                    id: logListView
                    anchors.fill: parent
                    anchors.margins: 16
                    model: launcherBridge.consoleLogs
                    spacing: 4

                    delegate: TextEdit {
                        width: logListView.width
                        text: modelData
                        readOnly: true
                        selectByMouse: true
                        activeFocusOnPress: false
                        selectionColor: "#4000E5FF"
                        selectedTextColor: "#FFFFFF"
                        font.family: "JetBrains Mono, Menlo, Consolas, monospace"
                        font.pixelSize: 12
                        wrapMode: TextEdit.WrapAnywhere
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
