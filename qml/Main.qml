import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "views"

ApplicationWindow {
    id: window
    width: 1000
    height: 640
    minimumWidth: 880
    minimumHeight: 560
    visible: true
    title: "Live, Camera, Action! SEASON 3 | FROSTY WINTER"
    color: "#0F1118"

    property int currentTabIndex: 0 // 0: Main, 1: Settings, 2: Console, 3: Login

    Connections {
        target: launcherBridge
        function onRequestLoginView() {
            window.currentTabIndex = 3
        }
    }

    Connections {
        target: authBridge
        function onLoginSucceeded() {
            window.currentTabIndex = 0
        }
    }

    // ================= Background Image =================
    Image {
        id: bgImage
        anchors.fill: parent
        source: "qrc:/qt/qml/LCALauncher/assets/background.jpg"
        fillMode: Image.PreserveAspectCrop
        smooth: true
        mipmap: true
        asynchronous: false
    }

    // Atmospheric Frosted Overlay (Darkening Vignette for high contrast)
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: "#55070912" }
            GradientStop { position: 0.5; color: "#660A0D18" }
            GradientStop { position: 1.0; color: "#88050710" }
        }
    }

    // Frosty Cyan Ambient Rim
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.color: "#1A00E5FF"
        border.width: 1
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ================= Sidebar (Liquid Glass) =================
        Rectangle {
            Layout.preferredWidth: 230
            Layout.fillHeight: true
            color: "#A60D101C"
            border.color: "#2EFFFFFF"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 20

                // Launcher Title Logo
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 105

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: window.currentTabIndex = 0
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 2

                        Image {
                            id: logoImg
                            source: "qrc:/qt/qml/LCALauncher/assets/logo.png"
                            Layout.fillWidth: true
                            Layout.preferredHeight: 88
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                            mipmap: true
                            asynchronous: false
                        }

                        Text {
                            text: "Frontline&TwoEngineers"
                            color: "#00E5FF"
                            font.pixelSize: 10
                            font.bold: true
                            font.letterSpacing: 0.8
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#26FFFFFF"
                }

                // Nav Buttons
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Repeater {
                        model: [
                            { name: "Главная", icon: "🎮", index: 0 },
                            { name: "Настройки", icon: "⚙️", index: 1 },
                            { name: "Консоль", icon: "📜", index: 2 }
                        ]

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 46
                            radius: 10
                            color: (window.currentTabIndex === modelData.index)
                                   ? "#4000E5FF"
                                   : (navArea.containsMouse ? "#26FFFFFF" : "#1AFFFFFF")
                            border.color: (window.currentTabIndex === modelData.index)
                                          ? "#00E5FF"
                                          : (navArea.containsMouse ? "#4DFFFFFF" : "#1AFFFFFF")
                            border.width: 1

                            Behavior on color { ColorAnimation { duration: 150 } }
                            Behavior on border.color { ColorAnimation { duration: 150 } }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 16
                                spacing: 12

                                Text {
                                    text: modelData.icon
                                    font.pixelSize: 16
                                }
                                Text {
                                    text: modelData.name
                                    color: (window.currentTabIndex === modelData.index) ? "#FFFFFF" : "#A6B2CC"
                                    font.pixelSize: 14
                                    font.bold: window.currentTabIndex === modelData.index
                                }
                            }

                            MouseArea {
                                id: navArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: window.currentTabIndex = modelData.index
                            }
                        }
                    }
                }

                Item { Layout.fillHeight: true }

                // User Profile Card at Bottom of Sidebar
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 64
                    radius: 12
                    color: profileMouse.containsMouse ? "#B3141829" : "#8C0F1322"
                    border.color: profileMouse.containsMouse ? "#00E5FF" : "#30FFFFFF"
                    border.width: 1

                    Behavior on color { ColorAnimation { duration: 150 } }
                    Behavior on border.color { ColorAnimation { duration: 150 } }

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 12

                        Rectangle {
                            width: 40
                            height: 40
                            radius: 8
                            color: "#3D2C344E"
                            border.color: "#40FFFFFF"
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: authBridge.isLoggedIn ? authBridge.username.slice(0, 1).toUpperCase() : "?"
                                color: "#00E5FF"
                                font.pixelSize: 18
                                font.bold: true
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: authBridge.isLoggedIn ? authBridge.username : "Не авторизован"
                                color: "#FFFFFF"
                                font.pixelSize: 13
                                font.bold: true
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                            Text {
                                text: authBridge.isLoggedIn ? (authBridge.userType === "custom" ? "Аккаунт сервера" : "Оффлайн") : "Нажмите для входа"
                                color: authBridge.isLoggedIn ? "#00FFA3" : "#FFB300"
                                font.pixelSize: 11
                            }
                        }
                    }

                    MouseArea {
                        id: profileMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: window.currentTabIndex = 3 // Go to login view
                    }
                }
            }
        }

        // ================= Content Area =================
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            StackLayout {
                anchors.fill: parent
                currentIndex: window.currentTabIndex

                MainView {}
                SettingsView {}
                ConsoleView {}
                LoginView {
                    onBackClicked: window.currentTabIndex = 0
                    onOpenSettingsRequested: window.currentTabIndex = 1
                }
            }
        }
    }
}
