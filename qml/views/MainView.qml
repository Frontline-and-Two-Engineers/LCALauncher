import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: root

    Rectangle {
        anchors.fill: parent
        color: "transparent"

        // Background decorative glow
        Rectangle {
            width: 450
            height: 450
            radius: 225
            color: "#1800E5FF"
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: -100
        }

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
                    radius: 14
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "#1E2233" }
                        GradientStop { position: 1.0; color: "#141724" }
                    }
                    border.color: "#2C3147"
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
                                    color: "#00FFA3"
                                }
                                Text {
                                    text: "СЕРВЕР ОНЛАЙН"
                                    color: "#00FFA3"
                                    font.pixelSize: 11
                                    font.bold: true
                                    font.letterSpacing: 1.5
                                }
                            }

                            Text {
                                text: "Minecraft 1.21.1"
                                color: "#FFFFFF"
                                font.pixelSize: 32
                                font.bold: true
                            }

                            Text {
                                text: "NeoForge Edition • Модовый сервер с кастомным клиентом"
                                color: "#9EA8C2"
                                font.pixelSize: 14
                            }
                        }

                        Rectangle {
                            Layout.preferredWidth: 140
                            Layout.preferredHeight: 60
                            radius: 10
                            color: "#161926"
                            border.color: "#2C3147"

                            Column {
                                anchors.centerIn: parent
                                spacing: 2
                                Text {
                                    text: "Онлайн"
                                    color: "#7E879E"
                                    font.pixelSize: 11
                                    horizontalAlignment: Text.AlignHCenter
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                                Text {
                                    text: "48 / 150"
                                    color: "#00E5FF"
                                    font.pixelSize: 18
                                    font.bold: true
                                    horizontalAlignment: Text.AlignHCenter
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                            }
                        }
                    }
                }
            }

            // Quick Info Cards
            RowLayout {
                Layout.fillWidth: true
                spacing: 20

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 90
                    radius: 12
                    color: "#181B27"
                    border.color: "#262B3D"

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 16

                        Text {
                            text: "☕"
                            font.pixelSize: 28
                        }
                        ColumnLayout {
                            spacing: 4
                            Text {
                                text: "Java Runtime"
                                color: "#7E879E"
                                font.pixelSize: 12
                            }
                            Text {
                                text: settingsBridge.detectedJavaInfo
                                color: "#E0E5F5"
                                font.pixelSize: 13
                                font.bold: true
                                elide: Text.ElideRight
                                Layout.maximumWidth: 260
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 90
                    radius: 12
                    color: "#181B27"
                    border.color: "#262B3D"

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 16

                        Text {
                            text: "⚡"
                            font.pixelSize: 28
                        }
                        ColumnLayout {
                            spacing: 4
                            Text {
                                text: "Выделенная память (RAM)"
                                color: "#7E879E"
                                font.pixelSize: 12
                            }
                            Text {
                                text: (settingsBridge.ramMb / 1024).toFixed(1) + " GB"
                                color: "#00FFA3"
                                font.pixelSize: 14
                                font.bold: true
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
                radius: 16
                color: "#181A26"
                border.color: "#272C3E"

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
