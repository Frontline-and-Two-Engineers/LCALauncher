import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "views"
import "components"

ApplicationWindow {
    id: window
    width: 1000
    height: 640
    minimumWidth: 880
    minimumHeight: 560
    visible: true
    title: "Light, Camera, Action! 3 | FROSTY WINTER"
    color: "#0F1118"
    font.family: "RazerF5"

    // Fonts are loaded globally in main.cpp via QFontDatabase::addApplicationFont;
    // QML FontLoader duplicates are removed.

    property int currentTabIndex: 0 // 0: Main, 1: Settings, 2: Console, 3: Login

    Connections {
        target: launcherBridge
        function onRequestLoginView() { window.currentTabIndex = 3 }
    }

    Connections {
        target: authBridge
        function onLoginSucceeded() { window.currentTabIndex = 0 }
    }

    Image {
        id: bgImage
        anchors.fill: parent
        source: "qrc:/qt/qml/LCALauncher/assets/background.jpg"
        fillMode: Image.PreserveAspectCrop
        smooth: true
        mipmap: true
        asynchronous: false
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: "#55070912" }
            GradientStop { position: 0.5; color: "#660A0D18" }
            GradientStop { position: 1.0; color: "#88050710" }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.color: "#1A00E5FF"
        border.width: 1
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Sidebar
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
                            source: "qrc:/qt/qml/LCALauncher/assets/logo.png"
                            Layout.fillWidth: true
                            Layout.preferredHeight: 74
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                            mipmap: true
                            asynchronous: false
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#26FFFFFF" }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Repeater {
                        model: [
                            { name: "Главная", icon: "qrc:/qt/qml/LCALauncher/assets/icons/gamepad.svg", index: 0 },
                            { name: "Настройки", icon: "qrc:/qt/qml/LCALauncher/assets/icons/settings.svg", index: 1 },
                            { name: "Консоль", icon: "qrc:/qt/qml/LCALauncher/assets/icons/console.svg", index: 2 }
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

                                LCAIcon {
                                    source: modelData.icon
                                    size: 18
                                    color: (window.currentTabIndex === modelData.index) ? "#00E5FF" : (navArea.containsMouse ? "#FFFFFF" : "#A6B2CC")
                                    Behavior on color { ColorAnimation { duration: 150 } }
                                    Layout.alignment: Qt.AlignVCenter
                                }
                                Text {
                                    text: modelData.name
                                    color: (window.currentTabIndex === modelData.index) ? "#FFFFFF" : (navArea.containsMouse ? "#E1E7F5" : "#A6B2CC")
                                    font.pixelSize: 14
                                    font.bold: window.currentTabIndex === modelData.index
                                    Behavior on color { ColorAnimation { duration: 150 } }
                                    Layout.alignment: Qt.AlignVCenter
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
                            width: 40; height: 40; radius: 8
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
                                text: authBridge.isLoggedIn ? "Аккаунт сервера" : "Нажмите для входа"
                                color: authBridge.isLoggedIn ? "#00FFA3" : "#FFB300"
                                font.pixelSize: 11
                                font.italic: true
                            }
                        }
                    }

                    MouseArea {
                        id: profileMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: window.currentTabIndex = 3
                    }
                }

                Image {
                    source: "qrc:/qt/qml/LCALauncher/assets/fate_logo.png"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 15
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: true
                    asynchronous: false
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

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

    // ================= Startup Splash Animation =================
    // Loaded via Loader so we can unload it (freeing MultiEffect / Shape)
    // once the intro completes. Using sourceComponent (not source URL) because
    // LCASplashIntro.qml lives in components/ and is exposed via `import "components"`.
    Loader {
        id: splashLoader
        anchors.fill: parent
        z: 9999
        active: true
        sourceComponent: splashComponent
        asynchronous: false

        onStatusChanged: {
            if (status === Loader.Error) {
                console.error("[Splash] Failed to load LCASplashIntro")
                splashLoader.active = false
            } else if (status === Loader.Ready && item) {
                item.introFinished.connect(function() {
                    // Defer unloading: we are inside the signal emission right now.
                    Qt.callLater(function() { splashLoader.active = false })
                })
            }
        }
    }

    Component {
        id: splashComponent
        LCASplashIntro {
            anchors.fill: parent
        }
    }
}