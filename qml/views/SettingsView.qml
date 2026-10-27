import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: root

    Rectangle {
        anchors.fill: parent
        color: "#12141D"

        ScrollView {
            anchors.fill: parent
            anchors.margins: 40
            contentWidth: availableWidth
            clip: true

            ColumnLayout {
                width: parent.width
                spacing: 24

                Text {
                    text: "Настройки лаунчера"
                    color: "#FFFFFF"
                    font.pixelSize: 26
                    font.bold: true
                }

                // 1. RAM Section
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 120
                    radius: 12
                    color: "#181B27"
                    border.color: "#272C3E"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 12

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: "Выделение оперативной памяти (Xmx)"
                                color: "#FFFFFF"
                                font.pixelSize: 15
                                font.bold: true
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: (ramSlider.value / 1024).toFixed(1) + " GB (" + Math.round(ramSlider.value) + " MB)"
                                color: "#00E5FF"
                                font.pixelSize: 15
                                font.bold: true
                            }
                        }

                        Slider {
                            id: ramSlider
                            Layout.fillWidth: true
                            from: 2048
                            to: 16384
                            stepSize: 512
                            value: settingsBridge.ramMb
                            onMoved: settingsBridge.ramMb = Math.round(value)
                        }
                    }
                }

                // 2. Java Runtime Section
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 130
                    radius: 12
                    color: "#181B27"
                    border.color: "#272C3E"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 12

                        Text {
                            text: "Java Runtime (Требуется Java 21+)"
                            color: "#FFFFFF"
                            font.pixelSize: 15
                            font.bold: true
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            LCATextField {
                                id: javaField
                                Layout.fillWidth: true
                                text: settingsBridge.javaPath
                                placeholderText: "Путь к исполняемому файлу java"
                                onEditingFinished: settingsBridge.javaPath = text
                            }

                            LCAButton {
                                Layout.preferredWidth: 150
                                Layout.preferredHeight: 44
                                text: "Автопоиск"
                                enabled: !settingsBridge.isDownloadingJava
                                onClicked: settingsBridge.autoDetectJava()
                            }

                            LCAButton {
                                Layout.preferredWidth: 160
                                Layout.preferredHeight: 44
                                isPrimary: true
                                accentColor: "#00E5FF"
                                textColor: "#0D1117"
                                text: settingsBridge.isDownloadingJava ? 
                                      ("Загрузка " + Math.round(settingsBridge.javaDownloadProgress * 100) + "%") : 
                                      "Скачать Java 21"
                                enabled: !settingsBridge.isDownloadingJava
                                onClicked: settingsBridge.downloadJava21()
                            }
                        }

                        Text {
                            text: settingsBridge.detectedJavaInfo
                            color: settingsBridge.isDownloadingJava ? "#00E5FF" : "#7E879E"
                            font.pixelSize: 12
                        }
                    }
                }

                // 3. Network & Server Section
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 200
                    radius: 12
                    color: "#181B27"
                    border.color: "#272C3E"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 12

                        Text {
                            text: "Подключение к серверу"
                            color: "#FFFFFF"
                            font.pixelSize: 15
                            font.bold: true
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                Text { text: "Адрес сервера"; color: "#8E98B0"; font.pixelSize: 12 }
                                LCATextField {
                                    Layout.fillWidth: true
                                    text: settingsBridge.serverAddress
                                    onEditingFinished: settingsBridge.serverAddress = text
                                }
                            }

                            ColumnLayout {
                                Layout.preferredWidth: 100
                                spacing: 4
                                Text { text: "Порт"; color: "#8E98B0"; font.pixelSize: 12 }
                                LCATextField {
                                    Layout.fillWidth: true
                                    text: settingsBridge.serverPort.toString()
                                    onEditingFinished: settingsBridge.serverPort = parseInt(text) || 25565
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            Rectangle {
                                width: 8
                                height: 8
                                radius: 4
                                color: serverStatusBridge.statusColor
                            }

                            Text {
                                text: serverStatusBridge.statusText + " (" + serverStatusBridge.playersText + ")" + (serverStatusBridge.isOnline ? (" • " + serverStatusBridge.pingMs + " ms") : "")
                                color: serverStatusBridge.statusColor
                                font.pixelSize: 12
                                font.bold: true
                            }

                            Item { Layout.fillWidth: true }

                            LCAButton {
                                text: serverStatusBridge.isChecking ? "Проверка..." : "Проверить"
                                Layout.preferredWidth: 110
                                Layout.preferredHeight: 32
                                enabled: !serverStatusBridge.isChecking
                                onClicked: serverStatusBridge.refresh()
                            }
                        }

                        CheckBox {
                            text: "Автоматически подключаться к серверу при старте (--quickPlayMultiplayer)"
                            checked: settingsBridge.autoConnect
                            palette.windowText: "#B0B8C9"
                            onToggled: settingsBridge.autoConnect = checked
                        }
                    }
                }

                // 4. Backend & Telegram Bot Section
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: backendColumn.implicitHeight + 40
                    radius: 12
                    color: "#181B27"
                    border.color: "#272C3E"

                    ColumnLayout {
                        id: backendColumn
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 12

                        Text {
                            text: "Сервер авторизации и сборки"
                            color: "#FFFFFF"
                            font.pixelSize: 15
                            font.bold: true
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                Text { text: "URL API бэкенда"; color: "#8E98B0"; font.pixelSize: 12 }
                                LCATextField {
                                    Layout.fillWidth: true
                                    text: settingsBridge.authUrl
                                    placeholderText: "http://127.0.0.1:8080"
                                    onEditingFinished: settingsBridge.authUrl = text
                                }
                            }

                            ColumnLayout {
                                Layout.preferredWidth: 180
                                spacing: 4
                                Text { text: "Имя бота в Telegram"; color: "#8E98B0"; font.pixelSize: 12 }
                                LCATextField {
                                    Layout.fillWidth: true
                                    text: settingsBridge.telegramBotUsername
                                    placeholderText: "@LCAMinecraftBot"
                                    onEditingFinished: settingsBridge.telegramBotUsername = text
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            Text { text: "URL манифеста сборки (manifest.json)"; color: "#8E98B0"; font.pixelSize: 12 }
                            LCATextField {
                                Layout.fillWidth: true
                                text: settingsBridge.modpackUrl
                                placeholderText: "http://127.0.0.1:8080/api/modpack/manifest.json"
                                onEditingFinished: settingsBridge.modpackUrl = text
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            Text { text: "URL базового клиента 1.21.1 NeoForge (client-base.zip)"; color: "#8E98B0"; font.pixelSize: 12 }
                            LCATextField {
                                Layout.fillWidth: true
                                text: settingsBridge.clientBaseUrl
                                placeholderText: "http://127.0.0.1:8080/api/modpack/files/client-base.zip"
                                onEditingFinished: settingsBridge.clientBaseUrl = text
                            }
                        }
                    }
                }

                // 5. Reset & Defaults
                RowLayout {
                    Layout.fillWidth: true
                    Item { Layout.fillWidth: true }
                    LCAButton {
                        text: "Сбросить к стандартным"
                        Layout.preferredWidth: 200
                        Layout.preferredHeight: 40
                        onClicked: settingsBridge.resetToDefaults()
                    }
                }
            }
        }
    }
}
