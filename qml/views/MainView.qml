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
                                Text {
                                    text: serverStatusBridge.statusText
                                    color: serverStatusBridge.statusColor
                                    font.pixelSize: 11
                                    font.bold: true
                                    font.italic: true
                                    font.letterSpacing: 1.5
                                }
                                Text {
                                    color: "#6E7790"
                                    font.pixelSize: 11
                                }
                            }

                            Text {
                                text: "Основной сервер"
                                color: "#FFFFFF"
                                font.pixelSize: 32
                                font.bold: true
                            }

                            Text {
                                text: "Сезон \"Frosty Winter\" - мороз ждёт Вас!"
                                color: "#9EA8C2"
                                font.pixelSize: 14
                                font.italic: true
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

            // ================= Channel Posts Section =================
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 10

                // Posts Header Row
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    LCAIcon {
                        source: "qrc:/qt/qml/LCALauncher/assets/icons/telegram.svg"
                        size: 15
                        color: "#00E5FF"
                        Layout.alignment: Qt.AlignVCenter
                    }

                    Text {
                        text: "НОВОСТИ КАНАЛА @" + channelPostsBridge.channelUsername.toUpperCase()
                        color: "#9EA8C2"
                        font.pixelSize: 11
                        font.bold: true
                        font.letterSpacing: 1.2
                        Layout.alignment: Qt.AlignVCenter
                    }

                    Item { Layout.fillWidth: true }

                    Item {
                        implicitWidth: refreshRow.implicitWidth
                        implicitHeight: refreshRow.implicitHeight
                        Layout.alignment: Qt.AlignVCenter

                        Row {
                            id: refreshRow
                            anchors.fill: parent
                            spacing: 5

                            LCAIcon {
                                source: "qrc:/qt/qml/LCALauncher/assets/icons/refresh.svg"
                                size: 12
                                color: channelPostsBridge.isLoading ? "#00E5FF" : (refreshMouse.containsMouse ? "#FFFFFF" : "#6E7790")
                                rotation: channelPostsBridge.isLoading ? newsAnim.angle : 0
                                anchors.verticalCenter: parent.verticalCenter

                                NumberAnimation on rotation {
                                    id: newsAnim
                                    property real angle: 0
                                    running: channelPostsBridge.isLoading
                                    loops: Animation.Infinite
                                    from: 0
                                    to: 360
                                    duration: 800
                                }
                            }

                            Text {
                                text: "Обновить"
                                color: refreshMouse.containsMouse ? "#FFFFFF" : "#6E7790"
                                font.pixelSize: 11
                                font.underline: refreshMouse.containsMouse
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }

                        MouseArea {
                            id: refreshMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: channelPostsBridge.refresh()
                        }
                    }

                    Rectangle {
                        width: 1
                        height: 12
                        color: "#33FFFFFF"
                    }

                    Item {
                        implicitWidth: openChannelText.implicitWidth
                        implicitHeight: openChannelText.implicitHeight
                        Layout.alignment: Qt.AlignVCenter

                        Text {
                            id: openChannelText
                            anchors.fill: parent
                            text: "Открыть канал"
                            color: openChannelMouse.containsMouse ? "#00E5FF" : "#6E7790"
                            font.pixelSize: 11
                            font.underline: openChannelMouse.containsMouse
                        }

                        MouseArea {
                            id: openChannelMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: Qt.openUrlExternally("https://t.me/" + channelPostsBridge.channelUsername)
                        }
                    }
                }

                // 3 Posts Horizontal Row (left to right, newest on the left)
                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 16
                    visible: channelPostsBridge.posts.length > 0

                    Repeater {
                        model: channelPostsBridge.posts

                        Rectangle {
                            id: postCard
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.minimumHeight: 140
                            Layout.preferredHeight: 175
                            radius: 14
                            color: cardMouse.containsMouse ? "#B3141829" : "#8C0F1424"
                            border.color: cardMouse.containsMouse ? "#00E5FF" : "#2EFFFFFF"
                            border.width: 1
                            clip: true

                            Behavior on color { ColorAnimation { duration: 150 } }
                            Behavior on border.color { ColorAnimation { duration: 150 } }

                            readonly property real cardAvailableWidth: Math.max(100, postCard.width - 24)

                            // Dynamic adaptive font size: 11px on compact window, up to 13px on fullscreen
                            readonly property real adaptiveFontSize: {
                                var progress = (postCard.width - 210) / 280.0;
                                var clamped = Math.max(0.0, Math.min(1.0, progress));
                                return Math.round(11.0 + clamped * 2.0);
                            }

                            // Dynamic adaptive image height: expands in fullscreen to match 16:9 / natural aspect ratio, minimizing crop
                            readonly property real adaptiveImageHeight: {
                                if (!modelData.image_url) return 0;

                                var naturalRatio = 9.0 / 16.0; // default 16:9 ratio
                                if (postImg.status === Image.Ready && postImg.implicitWidth > 0 && postImg.implicitHeight > 0) {
                                    naturalRatio = postImg.implicitHeight / postImg.implicitWidth;
                                }

                                var idealHeight = cardAvailableWidth * naturalRatio;
                                var maxAllowedByCard = Math.max(82, postCard.height * 0.48);
                                var minAllowed = Math.min(82, maxAllowedByCard);

                                var target = Math.max(minAllowed, Math.min(idealHeight, maxAllowedByCard));
                                return Math.min(target, 320);
                            }

                            readonly property real topItemHeight: (modelData.image_url !== "") 
                                ? adaptiveImageHeight 
                                : (dateRowNoImg.visible ? dateRowNoImg.implicitHeight : 0)

                            readonly property real footerHeight: Math.max(18, Math.round(adaptiveFontSize * 1.5))

                            readonly property real availTextHeight: Math.max(
                                30, 
                                (postCard.height - 24) - topItemHeight - footerHeight - (cardCol.spacing * 2)
                            )

                            FontMetrics {
                                id: textFontMetrics
                                font: postText.font
                            }

                            readonly property int maxLines: {
                                var lineH = (textFontMetrics.lineSpacing > 0 ? textFontMetrics.lineSpacing : 14.0) * postText.lineHeight;
                                return Math.max(2, Math.floor(availTextHeight / lineH));
                            }

                            ColumnLayout {
                                id: cardCol
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 8

                                // Post Image (if present)
                                Rectangle {
                                    id: imgContainer
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: postCard.adaptiveImageHeight
                                    radius: 10
                                    clip: true
                                    color: "#121520"
                                    visible: modelData.image_url !== ""

                                    Image {
                                        id: postImg
                                        anchors.fill: parent
                                        source: modelData.image_url || ""
                                        fillMode: Image.PreserveAspectCrop
                                        asynchronous: true
                                        smooth: true
                                        mipmap: true
                                    }

                                    // Gradient vignette at bottom of image
                                    Rectangle {
                                        anchors.bottom: parent.bottom
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        height: Math.max(26, Math.round(imgContainer.height * 0.25))
                                        gradient: Gradient {
                                            GradientStop { position: 0.0; color: "#00000000" }
                                            GradientStop { position: 1.0; color: "#B30A0D15" }
                                        }
                                    }

                                    // Date badge over image
                                    Rectangle {
                                        anchors.top: parent.top
                                        anchors.left: parent.left
                                        anchors.margins: 6
                                        height: Math.max(18, Math.round(postCard.adaptiveFontSize * 1.5))
                                        radius: height / 2
                                        color: "#D90B0E17"
                                        border.color: "#33FFFFFF"
                                        border.width: 1

                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.leftMargin: 8
                                            anchors.rightMargin: 8
                                            Text {
                                                text: modelData.formatted_date
                                                color: "#A2ADC7"
                                                font.pixelSize: Math.max(9, Math.round(postCard.adaptiveFontSize * 0.85))
                                                font.bold: true
                                                font.italic: true
                                            }
                                        }
                                    }
                                }

                                // Date & Channel badge if no image
                                RowLayout {
                                    id: dateRowNoImg
                                    Layout.fillWidth: true
                                    spacing: 6
                                    visible: modelData.image_url === ""

                                    Rectangle {
                                        width: Math.max(22, Math.round(postCard.adaptiveFontSize * 1.8))
                                        height: width
                                        radius: 6
                                        color: "#2600E5FF"
                                        border.color: "#4000E5FF"
                                        border.width: 1

                                        LCAIcon {
                                            anchors.centerIn: parent
                                            source: "qrc:/qt/qml/LCALauncher/assets/icons/telegram.svg"
                                            size: Math.max(12, Math.round(postCard.adaptiveFontSize))
                                            color: "#00E5FF"
                                        }
                                    }

                                    Text {
                                        text: modelData.formatted_date
                                        color: "#8E99B3"
                                        font.pixelSize: postCard.adaptiveFontSize
                                        font.bold: true
                                        Layout.alignment: Qt.AlignVCenter
                                    }

                                    Item { Layout.fillWidth: true }
                                }

                                // Post Text snippet
                                Text {
                                    id: postText
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    text: modelData.text !== "" ? modelData.text : "Публикация в канале"
                                    color: "#D5DDEC"
                                    font.pixelSize: postCard.adaptiveFontSize
                                    wrapMode: Text.Wrap
                                    elide: Text.ElideRight
                                    maximumLineCount: postCard.maxLines
                                    lineHeight: 1.25
                                }

                                // Card Footer: link & view count
                                RowLayout {
                                    id: cardFooter
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: postCard.footerHeight
                                    spacing: 4

                                    Text {
                                        text: "Читать в Telegram"
                                        color: cardMouse.containsMouse ? "#00FFA3" : "#00E5FF"
                                        font.pixelSize: Math.max(10, Math.round(postCard.adaptiveFontSize * 0.9))
                                        font.bold: true
                                        font.italic: true
                                        Behavior on color { ColorAnimation { duration: 150 } }
                                    }

                                    Item { Layout.fillWidth: true }

                                    RowLayout {
                                        spacing: 4
                                        visible: modelData.views !== "" && modelData.views !== "0"

                                        LCAIcon {
                                            source: "qrc:/qt/qml/LCALauncher/assets/icons/eye.svg"
                                            size: Math.max(11, Math.round(postCard.adaptiveFontSize * 0.95))
                                            color: "#6B758E"
                                            Layout.alignment: Qt.AlignVCenter
                                        }

                                        Text {
                                            text: modelData.views
                                            color: "#6B758E"
                                            font.pixelSize: Math.max(10, Math.round(postCard.adaptiveFontSize * 0.9))
                                            Layout.alignment: Qt.AlignVCenter
                                        }
                                    }
                                }
                            }

                            MouseArea {
                                id: cardMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: Qt.openUrlExternally(modelData.link)
                            }
                        }
                    }
                }

                // Loading / Empty State Placeholder
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 140
                    radius: 14
                    color: "#8C0F1424"
                    border.color: "#2EFFFFFF"
                    border.width: 1
                    visible: channelPostsBridge.posts.length === 0

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 8

                        LCAIcon {
                            source: "qrc:/qt/qml/LCALauncher/assets/icons/refresh.svg"
                            size: 24
                            color: "#00E5FF"
                            Layout.alignment: Qt.AlignHCenter
                            rotation: loadAnim.angle

                            NumberAnimation on rotation {
                                id: loadAnim
                                property real angle: 0
                                running: channelPostsBridge.isLoading
                                loops: Animation.Infinite
                                from: 0
                                to: 360
                                duration: 800
                            }
                        }

                        Text {
                            text: channelPostsBridge.isLoading 
                                ? "Загрузка публикаций канала..." 
                                : (channelPostsBridge.errorMessage !== "" ? "Не удалось загрузить новости." : "Публикаций пока нет")
                            color: channelPostsBridge.errorMessage !== "" ? "#FF8A80" : "#8E99B3"
                            font.pixelSize: 12
                            font.italic: true
                            Layout.alignment: Qt.AlignHCenter
                        }

                        LCAButton {
                            text: "Повторить попытку"
                            Layout.preferredWidth: 150
                            Layout.preferredHeight: 32
                            visible: !channelPostsBridge.isLoading && channelPostsBridge.errorMessage !== ""
                            Layout.alignment: Qt.AlignHCenter
                            onClicked: channelPostsBridge.refresh()
                        }
                    }
                }
            }

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
                            font.italic: true
                        }
                    }

                    LCAButton {
                        Layout.preferredWidth: 200
                        Layout.preferredHeight: 56
                        isPrimary: true
                        anchors.right: parent.right
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
