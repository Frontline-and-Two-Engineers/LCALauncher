import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: root

    signal backClicked()
    signal openSettingsRequested()

    property bool isCopiedNotification: false

    Timer {
        id: copyTimer
        interval: 2000
        onTriggered: root.isCopiedNotification = false
    }

    Component.onCompleted: {
        if (!authBridge.isLoggedIn && !authBridge.isPolling) {
            authBridge.requestCode()
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "transparent"

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
                        text: authBridge.isLoggedIn ? "Профиль игрока" : "Вход через Telegram"
                        color: "#FFFFFF"
                        font.pixelSize: 26
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Text {
                        text: authBridge.isLoggedIn 
                              ? "Лаунчер привязан к вашей учетной записи"
                              : ("Привязка лаунчера через бота " + authBridge.telegramBotUsername)
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
                    visible: authBridge.errorMessage !== "" && !authBridge.isLoggedIn

                    RowLayout {
                        id: errorLayout
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 12

                        LCAIcon {
                            source: "qrc:/qt/qml/LCALauncher/assets/icons/warning.svg"
                            size: 20
                            color: "#FF453A"
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
                                spacing: 16

                                Item {
                                    implicitWidth: settingsRow.implicitWidth
                                    implicitHeight: settingsRow.implicitHeight

                                    Row {
                                        id: settingsRow
                                        anchors.fill: parent
                                        spacing: 6

                                        LCAIcon {
                                            source: "qrc:/qt/qml/LCALauncher/assets/icons/settings.svg"
                                            size: 13
                                            color: "#00E5FF"
                                            anchors.verticalCenter: parent.verticalCenter
                                        }

                                        Text {
                                            text: "Настройки сервера"
                                            color: "#00E5FF"
                                            font.pixelSize: 12
                                            font.underline: settingsMouse.containsMouse
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                    }

                                    MouseArea {
                                        id: settingsMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: root.openSettingsRequested()
                                    }
                                }

                                Item {
                                    implicitWidth: retryRow.implicitWidth
                                    implicitHeight: retryRow.implicitHeight

                                    Row {
                                        id: retryRow
                                        anchors.fill: parent
                                        spacing: 6

                                        LCAIcon {
                                            source: "qrc:/qt/qml/LCALauncher/assets/icons/refresh.svg"
                                            size: 13
                                            color: "#00FFA3"
                                            anchors.verticalCenter: parent.verticalCenter
                                        }

                                        Text {
                                            text: "Повторить попытку"
                                            color: "#00FFA3"
                                            font.pixelSize: 12
                                            font.underline: retryMouse.containsMouse
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                    }

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

                // ================= Logged In Profile View =================
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 20
                    visible: authBridge.isLoggedIn

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 160
                        radius: 16
                        color: "#990F1322"
                        border.color: "#30FFFFFF"
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 20
                            spacing: 12

                            RowLayout {
                                spacing: 14
                                Layout.alignment: Qt.AlignHCenter

                                Rectangle {
                                    width: 44
                                    height: 44
                                    radius: 22
                                    color: "#00FFA3"

                                    LCAIcon {
                                        anchors.centerIn: parent
                                        source: "qrc:/qt/qml/LCALauncher/assets/icons/user.svg"
                                        size: 24
                                        color: "#0D1117"
                                    }
                                }

                                ColumnLayout {
                                    spacing: 4

                                    Text {
                                        text: authBridge.username
                                        color: "#FFFFFF"
                                        font.pixelSize: 20
                                        font.bold: true
                                    }

                                    RowLayout {
                                        spacing: 6

                                        Rectangle {
                                            width: 8
                                            height: 8
                                            radius: 4
                                            color: "#00FFA3"
                                        }

                                        Text {
                                            text: "Авторизован через Telegram"
                                            color: "#00FFA3"
                                            font.pixelSize: 12
                                            font.bold: true
                                        }
                                    }
                                }
                            }

                            Text {
                                text: "UUID: " + authBridge.uuid
                                color: "#7E879E"
                                font.pixelSize: 11
                                font.family: "JetBrains Mono, Menlo, Consolas, monospace"
                                Layout.alignment: Qt.AlignHCenter
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        LCAButton {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 46
                            isPrimary: true
                            accentColor: "#00E5FF"
                            textColor: "#0C0F17"
                            text: "Вернуться на главную"
                            onClicked: root.backClicked()
                        }

                        LCAButton {
                            Layout.preferredWidth: 160
                            Layout.preferredHeight: 46
                            text: "Выйти из аккаунта"
                            onClicked: {
                                authBridge.logout()
                                authBridge.requestCode()
                            }
                        }
                    }
                }

                // ================= Telegram Code Mode =================
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 20
                    visible: !authBridge.isLoggedIn

                    // Big Code Container Card
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 180
                        radius: 16
                        color: "#990F1322"
                        border.color: authBridge.isCodeExpired ? "#FF5252" : (authBridge.isPolling ? "#00E5FF" : "#30FFFFFF")
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
                                    iconSource: root.isCopiedNotification 
                                        ? "qrc:/qt/qml/LCALauncher/assets/icons/check.svg" 
                                        : "qrc:/qt/qml/LCALauncher/assets/icons/copy.svg"
                                    iconSize: 18
                                    iconColor: root.isCopiedNotification ? "#00FFA3" : "#FFFFFF"
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
                        color: "#8C0F1424"
                        border.color: "#2EFFFFFF"
                        border.width: 1

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
                            iconSource: "qrc:/qt/qml/LCALauncher/assets/icons/telegram.svg"
                            iconColor: "#0C0F17"
                            iconSize: 18
                            text: "Открыть бота в Telegram"
                            onClicked: authBridge.openTelegramBot()
                        }

                        LCAButton {
                            Layout.preferredWidth: 150
                            Layout.preferredHeight: 46
                            iconSource: authBridge.isBusy ? "" : "qrc:/qt/qml/LCALauncher/assets/icons/refresh.svg"
                            iconSize: 15
                            text: authBridge.isBusy ? "Запрос..." : "Новый код"
                            enabled: !authBridge.isBusy
                            onClicked: authBridge.requestCode()
                        }
                    }

                    // Back button row
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        Item { Layout.fillWidth: true }

                        LCAButton {
                            Layout.preferredWidth: 110
                            Layout.preferredHeight: 38
                            text: "Назад"
                            onClicked: root.backClicked()
                        }
                    }
                }
            }
        }
    }
}
