import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: root

    Rectangle {
        anchors.fill: parent
        color: "transparent"

        ScrollView {
            anchors.fill: parent
            anchors.margins: 40
            contentWidth: availableWidth
            clip: true

            ColumnLayout {
                width: parent.width
                spacing: 24

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Text {
                        text: "Настройки лаунчера"
                        color: "#FFFFFF"
                        font.pixelSize: 26
                        font.bold: true
                    }

                    Item { Layout.fillWidth: true }

                    Rectangle {
                        visible: settingsBridge.isDevMode
                        Layout.preferredHeight: 28
                        Layout.preferredWidth: devModeLabel.implicitWidth + 24
                        radius: 6
                        color: "#26FF9100"
                        border.color: "#FF9100"
                        border.width: 1

                        Text {
                            id: devModeLabel
                            anchors.centerIn: parent
                            text: "DEV MODE"
                            color: "#FF9100"
                            font.pixelSize: 11
                            font.bold: true
                            font.italic: true
                        }
                    }
                }

                // 1. RAM Section
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 120
                    radius: 14
                    color: "#990F1424"
                    border.color: "#30FFFFFF"
                    border.width: 1

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
                    radius: 14
                    color: "#990F1424"
                    border.color: "#30FFFFFF"
                    border.width: 1

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
                            font.italic: true
                        }
                    }
                }

                // 3. Network & Server Section
                Rectangle {
                    visible: settingsBridge.isDevMode
                    Layout.fillWidth: true
                    Layout.preferredHeight: 220
                    radius: 14
                    color: "#990F1424"
                    border.color: "#30FFFFFF"
                    border.width: 1

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
                    visible: settingsBridge.isDevMode
                    Layout.fillWidth: true
                    implicitHeight: backendColumn.implicitHeight + 40
                    radius: 14
                    color: "#990F1424"
                    border.color: "#30FFFFFF"
                    border.width: 1

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

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            Text { text: "Рабочая папка игры"; color: "#8E98B0"; font.pixelSize: 12 }
                            LCATextField {
                                Layout.fillWidth: true
                                text: settingsBridge.gameDir
                                placeholderText: "Путь к папке игры"
                                onEditingFinished: settingsBridge.gameDir = text
                            }
                        }
                    }
                }

                // 4.5. Launcher Updates
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: updateSettingsCol.implicitHeight + 48
                    radius: 14
                    color: "#B3101424"
                    border.color: "#2EFFFFFF"
                    border.width: 1

                    ColumnLayout {
                        id: updateSettingsCol
                        anchors.fill: parent
                        anchors.margins: 24
                        spacing: 14

                        RowLayout {
                            Layout.fillWidth: true

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4

                                Text {
                                    text: "Обновления лаунчера"
                                    color: "#FFFFFF"
                                    font.pixelSize: 16
                                    font.bold: true
                                }

                                Text {
                                    text: "Текущая версия: v" + updateBridge.currentVersion
                                    color: "#8E98B0"
                                    font.pixelSize: 12
                                }
                            }

                            LCAButton {
                                visible: updateBridge.updateAvailable
                                text: updateBridge.isInstalling ? "Установка..." : (updateBridge.isDownloading ? "Загрузка..." : "Обновить до v" + updateBridge.latestVersion)
                                isPrimary: true
                                textColor: "#0A0D18"
                                enabled: !updateBridge.isDownloading && !updateBridge.isInstalling
                                Layout.preferredWidth: 190
                                Layout.preferredHeight: 38
                                onClicked: updateBridge.startUpdate()
                            }

                            LCAButton {
                                text: updateBridge.isChecking ? "Проверка..." : "Проверить"
                                enabled: !updateBridge.isChecking && !updateBridge.isDownloading && !updateBridge.isInstalling
                                Layout.preferredWidth: 140
                                Layout.preferredHeight: 38
                                onClicked: updateBridge.checkForUpdates(false)
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Text {
                                    text: "Ветка обновлений"
                                    color: "#E0E6F2"
                                    font.pixelSize: 13
                                    font.bold: true
                                }

                                Text {
                                    text: updateBridge.updateChannel === "dev"
                                          ? "dev — включая тестовые пререлизы"
                                          : "main — только стабильные релизы"
                                    color: "#7E879E"
                                    font.pixelSize: 11
                                    font.italic: true
                                }
                            }

                            Rectangle {
                                Layout.preferredWidth: 156
                                Layout.preferredHeight: 36
                                radius: 8
                                color: "#80090C16"
                                border.color: "#2EFFFFFF"
                                border.width: 1

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 3
                                    spacing: 4

                                    Repeater {
                                        model: ["main", "dev"]

                                        Rectangle {
                                            Layout.fillWidth: true
                                            Layout.fillHeight: true
                                            radius: 6
                                            property bool selected: updateBridge.updateChannel === modelData
                                            color: selected ? "#00E5FF" : (chanMouse.containsMouse ? "#26FFFFFF" : "transparent")

                                            Behavior on color { ColorAnimation { duration: 140 } }

                                            Text {
                                                anchors.centerIn: parent
                                                text: modelData
                                                color: selected ? "#0A0D18" : "#A6B2CC"
                                                font.pixelSize: 12
                                                font.bold: true
                                            }

                                            MouseArea {
                                                id: chanMouse
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                cursorShape: Qt.PointingHandCursor
                                                enabled: !updateBridge.isDownloading && !updateBridge.isInstalling
                                                onClicked: updateBridge.updateChannel = modelData
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            visible: updateBridge.statusText !== "" || updateBridge.errorText !== ""
                            text: updateBridge.errorText !== "" ? updateBridge.errorText : updateBridge.statusText
                            color: updateBridge.errorText !== "" ? "#FF6B6B" : (updateBridge.updateAvailable ? "#00E5FF" : "#00FFA3")
                            font.pixelSize: 12
                            wrapMode: Text.Wrap
                        }
                    }
                }

                // 5. Reset & Defaults
                RowLayout {
                    visible: settingsBridge.isDevMode
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
