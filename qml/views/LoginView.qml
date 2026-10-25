import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: root

    signal backClicked()

    Rectangle {
        anchors.fill: parent
        color: "#12141D"

        ColumnLayout {
            anchors.centerIn: parent
            width: 380
            spacing: 20

            Text {
                text: "Вход в аккаунт"
                color: "#FFFFFF"
                font.pixelSize: 26
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }

            Text {
                text: "Авторизуйтесь для доступа к модовому серверу"
                color: "#808A9D"
                font.pixelSize: 13
                Layout.alignment: Qt.AlignHCenter
            }

            // Error display
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                radius: 8
                color: "#33FF5252"
                border.color: "#FF5252"
                visible: authBridge.errorMessage !== ""

                Text {
                    anchors.centerIn: parent
                    text: authBridge.errorMessage
                    color: "#FF8A80"
                    font.pixelSize: 12
                    font.bold: true
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: "Логин / Никнейм"
                    color: "#A2ABBF"
                    font.pixelSize: 12
                }
                LCATextField {
                    id: usernameField
                    Layout.fillWidth: true
                    placeholderText: "Ваш никнейм"
                    text: authBridge.username
                }
            }

            ColumnLayout {
                id: passContainer
                Layout.fillWidth: true
                spacing: 8
                visible: !offlineCheck.checked

                Text {
                    text: "Пароль"
                    color: "#A2ABBF"
                    font.pixelSize: 12
                }
                LCATextField {
                    id: passwordField
                    Layout.fillWidth: true
                    placeholderText: "Пароль от аккаунта"
                    echoMode: TextInput.Password
                }
            }

            CheckBox {
                id: offlineCheck
                text: "Оффлайн режим (только никнейм)"
                checked: false
                palette.windowText: "#9CA3AF"
            }

            Item { Layout.preferredHeight: 8 }

            LCAButton {
                Layout.fillWidth: true
                Layout.preferredHeight: 48
                isPrimary: true
                accentColor: "#00E5FF"
                textColor: "#0C0F17"
                enabled: !authBridge.isBusy
                text: authBridge.isBusy ? "Проверка..." : "Войти"

                onClicked: {
                    if (offlineCheck.checked) {
                        authBridge.loginOffline(usernameField.text)
                    } else {
                        authBridge.loginWithCredentials(usernameField.text, passwordField.text)
                    }
                }
            }

            LCAButton {
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                text: "Назад"
                visible: authBridge.isLoggedIn
                onClicked: root.backClicked()
            }
        }
    }
}
