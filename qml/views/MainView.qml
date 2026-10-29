import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: root

    Rectangle {
        anchors.fill: parent
        color: "transparent"



        // Main layout
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 40
            spacing: 24

            // Header Hero Banner
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 180

                Rectangle {
                    anchors.fill: parent
                    radius: 16
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "#A6151D2F" }
                        GradientStop { position: 1.0; color: "#800F1322" }
                    }
                    border.color: "#33FFFFFF"
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 28
                        spacing: 20

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            RowLayout {
                                spacing: 10
                                Rectangle {
                                    width: 8
                                    height: 8
                                    radius: 4
                                    color: serverStatusBridge.statusColor

                                    SequentialAnimation on opacity {
                                        running: serverStatusBridge.isOnline
                                        loops: Animation.Infinite
                                        NumberAnimation { from: 1.0; to: 0.4; duration: 900; easing.type: Easing.InOutQuad }
                                        NumberAnimation { from: 0.4; to: 1.0; duration: 900; easing.type: Easing.InOutQuad }
                                    }
                                }
                                Text {
                                    text: serverStatusBridge.statusText
                                    color: serverStatusBridge.statusColor
                                    font.pixelSize: 11
                                    font.bold: true
                                    font.letterSpacing: 1.5
                                }
                                Text {
                                    text: "•  " + settingsBridge.serverAddress + (settingsBridge.serverPort !== 25565 ? (":" + settingsBridge.serverPort) : "")
                                    color: "#6E7790"
                                    font.pixelSize: 11
                                }
                            }

                            Text {
                                text: "Light, Camera, Action!"
                                color: "#FFFFFF"
                                font.pixelSize: 32
                                font.bold: true
                            }

                            Text {
                                text: "Light, Camera, Action! 3 СЕЗОН | FROSTY WINTER\nПри поддержке dead_seniordev"
                                color: "#9EA8C2"
                                font.pixelSize: 14
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                        }

                        Rectangle {
                            Layout.preferredWidth: 150
                            Layout.preferredHeight: 74
                            radius: 12
                            color: onlineMouseArea.containsMouse ? "#1E2235" : "#161926"
                            border.color: onlineMouseArea.containsMouse ? "#00E5FF" : "#2C3147"
                            border.width: 1

                            Behavior on color { ColorAnimation { duration: 150 } }
                            Behavior on border.color { ColorAnimation { duration: 150 } }

                            MouseArea {
                                id: onlineMouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: serverStatusBridge.refresh()
                            }

                            ToolTip.visible: onlineMouseArea.containsMouse
                            ToolTip.text: "Нажмите, чтобы обновить статус сервера"
                            ToolTip.delay: 400

                            ColumnLayout {
                                anchors.centerIn: parent
                                spacing: 2

                                RowLayout {
                                    Layout.alignment: Qt.AlignHCenter
                                    spacing: 6

                                    Text {
                                        text: "Онлайн"
                                        color: "#7E879E"
                                        font.pixelSize: 11
                                    }

                                    LCAIcon {
                                        source: "qrc:/qt/qml/LCALauncher/assets/icons/refresh.svg"
                                        size: 11
                                        color: serverStatusBridge.isChecking ? "#00E5FF" : "#555E75"
                                        rotation: serverStatusBridge.isChecking ? refreshAnim.angle : 0
                                        Layout.alignment: Qt.AlignVCenter

                                        NumberAnimation on rotation {
                                            id: refreshAnim
                                            property real angle: 0
                                            running: serverStatusBridge.isChecking
                                            loops: Animation.Infinite
                                            from: 0
                                            to: 360
                                            duration: 800
                                        }
                                    }
                                }

                                Text {
                                    Layout.alignment: Qt.AlignHCenter
                                    text: serverStatusBridge.playersText
                                    color: serverStatusBridge.isOnline ? "#00E5FF" : (serverStatusBridge.isChecking ? "#FFB300" : "#7E879E")
                                    font.pixelSize: 18
                                    font.bold: true
                                }

                                RowLayout {
                                    Layout.alignment: Qt.AlignHCenter
                                    spacing: 5
                                    visible: serverStatusBridge.isOnline

                                    LCAIcon {
                                        source: "qrc:/qt/qml/LCALauncher/assets/icons/signal.svg"
                                        size: 10
                                        color: {
                                            if (serverStatusBridge.pingMs < 60) return "#00FFA3";
                                            if (serverStatusBridge.pingMs < 150) return "#FFD54F";
                                            return "#FF5252";
                                        }
                                        Layout.alignment: Qt.AlignVCenter
                                    }
                                    Text {
                                        text: serverStatusBridge.pingMs + " ms"
                                        color: {
                                            if (serverStatusBridge.pingMs < 60) return "#00FFA3";
                                            if (serverStatusBridge.pingMs < 150) return "#FFD54F";
                                            return "#FF5252";
                                        }
                                        font.pixelSize: 10
                                        font.bold: true
                                        Layout.alignment: Qt.AlignVCenter
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true }

            // Bottom Action Area
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 110
                radius: 18
                color: "#A60C101A"
                border.color: "#33FFFFFF"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 24

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        LCAProgressBar {
                            Layout.fillWidth: true
                            visible: launcherBridge.isBusy
                            value: launcherBridge.progressPercent
                            statusText: launcherBridge.statusText
                        }

                        Text {
                            visible: !launcherBridge.isBusy
                            text: launcherBridge.statusText
                            color: "#A2ABBF"
                            font.pixelSize: 14
                        }
                    }

                    LCAButton {
                        Layout.preferredWidth: 200
                        Layout.preferredHeight: 56
                        isPrimary: true
                        accentColor: launcherBridge.isGameRunning ? "#FF5252" : "#00E5FF"
                        textColor: launcherBridge.isGameRunning ? "#FFFFFF" : "#0A0D14"
                        enabled: !launcherBridge.isBusy

                        text: {
                            if (launcherBridge.isGameRunning) return "ЗАКРЫТЬ ИГРУ"
                            if (launcherBridge.isBusy) return "ЗАГРУЗКА..."
                            return "ИГРАТЬ"
                        }

                        onClicked: {
                            if (launcherBridge.isGameRunning) {
                                launcherBridge.killGame()
                            } else {
                                launcherBridge.launchGame()
                            }
                        }
                    }
                }
            }
        }
    }
}
