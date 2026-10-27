import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: root

    signal backClicked()
    signal openSettingsRequested()

    property bool isOfflineMode: false
    property bool isCopiedNotification: false

    Timer {
        id: copyTimer
        interval: 2000
        onTriggered: root.isCopiedNotification = false
    }

    Component.onCompleted: {
        if (!authBridge.isLoggedIn && !authBridge.isPolling && !root.isOfflineMode) {
            authBridge.requestCode()
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#12141D"

        // Background decorative glow
        Rectangle {
            width: 500
            height: 500
            radius: 250
            color: "#1200E5FF"
            anchors.centerIn: parent
        }

        ScrollView {
            anchors.fill: parent
            contentWidth: availableWidth
            clip: true

            ColumnLayout {
                anchors.centerIn: parent
                width: Math.min(460, parent.width - 40)
                spacing: 20

                // Header
                ColumnLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 6

                    Text {
                        text: root.isOfflineMode ? "Оффлайн вход" : "Вход через Telegram"
                        color: "#FFFFFF"
                        font.pixelSize: 26
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Text {
                        text: root.isOfflineMode 
                              ? "Локальный запуск без подключения к серверу"
                              : "Привязка лаунчера через бота " + authBridge.telegramBotUsername
                        color: "#8B95AB"
                        font.pixelSize: 13
                        Layout.alignment: Qt.AlignHCenter
                    }
                }

                // Error Message Box
                Rectangle {
                    id: errorBox
                    Layout.fillWidth: true
                    implicitHeight: errorLayout.implicitHeight + 24
                    radius: 10
                    color: "#2B1417"
                    border.color: "#FF453A"
                    border.width: 1
                    visible: authBridge.errorMessage !== ""

                    RowLayout {
                        id: errorLayout
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 12

                        Text {
                            text: "⚠️"
                            font.pixelSize: 20
                            Layout.alignment: Qt.AlignTop
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Text {
                                text: authBridge.errorMessage
                                color: "#FFB4AB"
                                font.pixelSize: 12
                                Layout.fillWidth: true
                                wrapMode: Text.Wrap
                                textFormat: Text.PlainText
                                lineHeight: 1.25
                            }

                            RowLayout {
                                spacing: 14

                                Text {
                                    text: "⚙️ Настройки сервера"
                                    color: "#00E5FF"
                                    font.pixelSize: 12
                                    font.underline: settingsMouse.containsMouse

                                    MouseArea {
                                        id: settingsMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: root.openSettingsRequested()
                                    }
                                }

                                Text {
                                    text: "🔄 Повторить попытку"
                                    color: "#00FFA3"
                                    font.pixelSize: 12
                                    font.underline: retryMouse.containsMouse

                                    MouseArea {
                                        id: retryMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: authBridge.requestCode()
                                    }
                                }
                            }
                        }
                    }
                }

                // ================= Telegram Code Mode =================
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 20
                    visible: !root.isOfflineMode

                    // Big Code Container Card
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 180
                        radius: 16
                        color: "#181B27"
                        border.color: authBridge.isCodeExpired ? "#FF5252" : (authBridge.isPolling ? "#00E5FF" : "#2A3044")
                        border.width: authBridge.isPolling ? 2 : 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 20
                            spacing: 12

                            Text {
                                text: authBridge.isCodeExpired ? "СРОК КОДА ИСТЁК" : "ВАШ КОД АВТОРИЗАЦИИ"
                                color: authBridge.isCodeExpired ? "#FF8A80" : "#7E879E"
                                font.pixelSize: 11
                                font.bold: true
                                font.letterSpacing: 1.5
                                Layout.alignment: Qt.AlignHCenter
                            }

                            // Glowing 6-digit Code Display
                            RowLayout {
                                Layout.alignment: Qt.AlignHCenter
                                spacing: 14

                                Text {
                                    text: authBridge.authCode !== "" ? authBridge.formattedAuthCode : "------"
                                    color: authBridge.isCodeExpired ? "#6B7280" : "#00FFA3"
                                    font.pixelSize: 38
                                    font.bold: true
                                    font.family: "JetBrains Mono, Menlo, Consolas, monospace"
                                    font.letterSpacing: 4
                                }

                                LCAButton {
                                    Layout.preferredWidth: 40
                                    Layout.preferredHeight: 40
                                    text: root.isCopiedNotification ? "✓" : "📋"
                                    enabled: authBridge.authCode !== "" && !authBridge.isCodeExpired
                                    onClicked: {
                                        authBridge.copyCodeToClipboard()
                                        root.isCopiedNotification = true
                                        copyTimer.restart()
                                    }
                                }
                            }

                            // Status / Timer pill
                            RowLayout {
                                Layout.alignment: Qt.AlignHCenter
                                spacing: 8

                                Rectangle {
                                    width: 8
                                    height: 8
                                    radius: 4
                                    color: authBridge.isCodeExpired ? "#FF5252" : "#00E5FF"
                                    
                                    SequentialAnimation on opacity {
                                        running: authBridge.isPolling
                                        loops: Animation.Infinite
                                        NumberAnimation { from: 0.3; to: 1.0; duration: 800; easing.type: Easing.InOutQuad }
                                        NumberAnimation { from: 1.0; to: 0.3; duration: 800; easing.type: Easing.InOutQuad }
                                    }
                                }

                                Text {
                                    text: {
                                        if (root.isCopiedNotification) return "Код скопирован в буфер обмена!"
                                        if (authBridge.isCodeExpired) return "Код больше недействителен. Запросите новый."
                                        if (authBridge.isPolling) return "Ожидаем подтверждения • Истекает через: " + authBridge.formattedTime
                                        return "Нажмите «Получить код» для входа"
                                    }
                                    color: root.isCopiedNotification ? "#00FFA3" : (authBridge.isCodeExpired ? "#FF8A80" : "#9AA4BF")
                                    font.pixelSize: 12
                                }
                            }
                        }
                    }

                    // Steps Guide
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 90
                        radius: 12
                        color: "#161925"
                        border.color: "#24293A"

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 14
                            spacing: 6

                            Text {
                                text: "1. Откройте бота: " + authBridge.telegramBotUsername
                                color: "#BAC3D6"
                                font.pixelSize: 12
                            }
                            Text {
                                text: "2. Нажмите /start и зарегистрируйте игровой ник (если первый раз)"
                                color: "#BAC3D6"
                                font.pixelSize: 12
                            }
                            Text {
                                text: "3. Отправьте боту 6-значный код выше"
                                color: "#00E5FF"
                                font.pixelSize: 12
                                font.bold: true
                            }
                        }
                    }

                    // Action Buttons
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        LCAButton {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 46
                            isPrimary: true
                            accentColor: "#00E5FF"
                            textColor: "#0C0F17"
                            text: "✈️ Открыть бота в Telegram"
                            onClicked: authBridge.openTelegramBot()
                        }

                        LCAButton {
                            Layout.preferredWidth: 150
                            Layout.preferredHeight: 46
                            text: authBridge.isBusy ? "Запрос..." : "🔄 Новый код"
                            enabled: !authBridge.isBusy
                            onClicked: authBridge.requestCode()
                        }
                    }
                }

                // ================= Offline Mode =================
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 16
                    visible: root.isOfflineMode

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Text {
                            text: "Игровой никнейм (без пароля)"
                            color: "#A2ABBF"
                            font.pixelSize: 12
                        }
                        LCATextField {
                            id: offlineUsernameField
                            Layout.fillWidth: true
                            placeholderText: "Например: Player"
                            text: authBridge.username
                        }
                    }

                    LCAButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 46
                        isPrimary: true
                        accentColor: "#00FFA3"
                        textColor: "#0C0F17"
                        text: "Войти в оффлайн режиме"
                        onClicked: authBridge.loginOffline(offlineUsernameField.text)
                    }
                }

                // Mode switch & Back button
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    LCAButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38
                        text: root.isOfflineMode ? "← Войти через Telegram" : "Режим без интернета (Оффлайн)"
                        onClicked: {
                            root.isOfflineMode = !root.isOfflineMode
                            if (!root.isOfflineMode && !authBridge.isPolling) {
                                authBridge.requestCode()
                            }
                        }
                    }

                    LCAButton {
                        Layout.preferredWidth: 110
                        Layout.preferredHeight: 38
                        text: "Назад"
                        visible: authBridge.isLoggedIn
                        onClicked: root.backClicked()
                    }
                }
            }
        }
    }
}
