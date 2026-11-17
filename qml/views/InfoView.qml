import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: root

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 40
        spacing: 20

        // Header
        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    text: "Информация о проекте"
                    color: "#FFFFFF"
                    font.pixelSize: 28
                    font.bold: true
                }

                Text {
                    text: docsBridge.currentSubtitle !== "" ? docsBridge.currentSubtitle : "Правила, гайды и ответы на частые вопросы"
                    color: "#8E98B0"
                    font.pixelSize: 13
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            Text {
                visible: docsBridge.statusMessage !== ""
                text: docsBridge.isLoading ? "Загрузка..." : docsBridge.statusMessage
                color: docsBridge.isLoading ? "#00E5FF" : "#7E879E"
                font.pixelSize: 12
                font.italic: true
                Layout.alignment: Qt.AlignVCenter
            }

            LCAButton {
                text: "Обновить"
                iconSource: "qrc:/qt/qml/LCALauncher/assets/icons/refresh.svg"
                iconSize: 14
                enabled: !docsBridge.isLoading
                Layout.preferredWidth: 130
                Layout.preferredHeight: 38
                onClicked: docsBridge.refresh()
            }
        }

        // Section Tabs
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            LCAButton {
                visible: docsBridge.canGoBack
                text: "Назад"
                iconSource: "qrc:/qt/qml/LCALauncher/assets/icons/arrow-left.svg"
                iconSize: 14
                Layout.preferredWidth: 105
                Layout.preferredHeight: 38
                onClicked: {
                    docsBridge.goBack()
                    docFlickable.contentY = 0
                }
            }

            Repeater {
                model: docsBridge.sections

                Rectangle {
                    property bool isActive: docsBridge.currentIndex === index
                    Layout.preferredHeight: 38
                    Layout.preferredWidth: Math.max(120, tabText.implicitWidth + 32)
                    radius: 10
                    color: isActive ? "#4000E5FF" : (tabMouse.containsMouse ? "#26FFFFFF" : "#800F1424")
                    border.color: isActive ? "#00E5FF" : (tabMouse.containsMouse ? "#4DFFFFFF" : "#26FFFFFF")
                    border.width: 1

                    Behavior on color { ColorAnimation { duration: 150 } }
                    Behavior on border.color { ColorAnimation { duration: 150 } }

                    Text {
                        id: tabText
                        anchors.centerIn: parent
                        text: modelData.title
                        color: isActive ? "#FFFFFF" : (tabMouse.containsMouse ? "#E1E7F5" : "#9EA8C2")
                        font.pixelSize: 13
                        font.bold: isActive
                    }

                    MouseArea {
                        id: tabMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            docsBridge.selectSection(index)
                            docFlickable.contentY = 0
                        }
                    }
                }
            }

            Item { Layout.fillWidth: true }
        }

        // Markdown Content Card
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 16
            color: "#B30F1424"
            border.color: "#2EFFFFFF"
            border.width: 1
            clip: true

            Flickable {
                id: docFlickable
                anchors.fill: parent
                anchors.margins: 24
                contentWidth: width - 16
                contentHeight: docText.implicitHeight + 16
                clip: true
                boundsBehavior: Flickable.StopAtBounds

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }

                Text {
                    id: docText
                    width: docFlickable.width - 16
                    text: docsBridge.currentMarkdown
                    textFormat: Text.MarkdownText
                    wrapMode: Text.Wrap
                    color: "#DCE3F2"
                    linkColor: "#00E5FF"
                    font.family: "RazerF5"
                    font.pixelSize: 14
                    lineHeight: 1.35

                    onLinkActivated: function(link) {
                        if (docsBridge.openLink(link)) {
                            docFlickable.contentY = 0
                        } else {
                            Qt.openUrlExternally(link)
                        }
                    }

                    HoverHandler {
                        cursorShape: docText.hoveredLink !== "" ? Qt.PointingHandCursor : Qt.ArrowCursor
                    }
                }
            }
        }
    }
}
