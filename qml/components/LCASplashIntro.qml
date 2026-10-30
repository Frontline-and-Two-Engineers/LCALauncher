import QtQuick
import QtQuick.Effects
import QtQuick.Shapes

Item {
    id: root
    anchors.fill: parent
    z: 9999

    signal introFinished()

    property bool isPlaying: true
    readonly property real diagAngleRad: Math.atan2(height, width)
    readonly property real diagAngleDeg: diagAngleRad * 180.0 / Math.PI
    readonly property real diagLength: Math.sqrt(width * width + height * height) + 300
    readonly property real unitUx: Math.cos(diagAngleRad)
    readonly property real unitUy: Math.sin(diagAngleRad)

    // Animation progress properties
    property real fullLogoOpacity: 0.0
    property real fullLogoScale: 0.90
    property real cutLineProgress: 0.0
    property real cutFlashOpacity: 0.0
    property real slideParallel: 0.0
    property real slideOpen: 0.0
    property bool splitActive: false

    // Calculated offsets for Piece 1 (Top-Right) — уходит в правый верхний угол
    readonly property real p1OffsetX: (slideOpen - slideParallel) * unitUx
    readonly property real p1OffsetY: -(slideOpen + slideParallel) * unitUy

    // Calculated offsets for Piece 2 (Bottom-Left) — уходит в левый нижний угол
    readonly property real p2OffsetX: -(slideOpen - slideParallel) * unitUx
    readonly property real p2OffsetY: (slideOpen + slideParallel) * unitUy

    // Consume all mouse input while splash is active so user cannot click menu early
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.AllButtons
        onClicked: {
            // Optional: allow click to speed up or skip
            if (introSeq.running) {
                introSeq.complete()
            }
        }
    }

    // =========================================================================
    // 1. PHASE 1: Whole Screen & Center Logo (Single unified view before cut)
    // =========================================================================
    Item {
        id: wholeIntroContainer
        anchors.fill: parent
        visible: !root.splitActive

        // Dark accent background
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                orientation: Gradient.Vertical
                GradientStop { position: 0.0; color: "#060810" }
                GradientStop { position: 0.5; color: "#080B15" }
                GradientStop { position: 1.0; color: "#05070C" }
            }
        }

        // Center Logo
        Item {
            id: centerLogoBox
            anchors.centerIn: parent
            width: Math.min(460, root.width * 0.48) / 1.3
            height: width * (384.0 / 650.0) / 1.3
            opacity: root.fullLogoOpacity
            scale: root.fullLogoScale

            Image {
                anchors.fill: parent
                source: "qrc:/qt/qml/LCALauncher/assets/logo.png"
                fillMode: Image.PreserveAspectFit
                smooth: true
                mipmap: true
            }
        }
    }

    // =========================================================================
    // 2. PHASE 3 & 4: Two Split Halves (Active after diagonal cut)
    // =========================================================================

    // --- Mask 1: Upper-Right Half Triangle ---
    Item {
        id: mask1Item
        width: root.width
        height: root.height
        visible: false
        layer.enabled: true

        Shape {
            anchors.fill: parent
            ShapePath {
                fillColor: "#FFFFFF"
                strokeColor: "transparent"
                startX: -150
                startY: -150
                PathLine { x: root.width + 150; y: -150 }
                PathLine { x: root.width + 150; y: root.height + 150 }
                PathLine { x: -150; y: -150 }
            }
        }
    }

    // --- Mask 2: Lower-Left Half Triangle ---
    Item {
        id: mask2Item
        width: root.width
        height: root.height
        visible: false
        layer.enabled: true

        Shape {
            anchors.fill: parent
            ShapePath {
                fillColor: "#FFFFFF"
                strokeColor: "transparent"
                startX: -150
                startY: -150
                PathLine { x: root.width + 150; y: root.height + 150 }
                PathLine { x: -150; y: root.height + 150 }
                PathLine { x: -150; y: -150 }
            }
        }
    }

    // --- Component template for one half-screen content ---
    Component {
        id: halfContentComponent
        Item {
            width: root.width
            height: root.height

            Rectangle {
                anchors.fill: parent
                gradient: Gradient {
                    orientation: Gradient.Vertical
                    GradientStop { position: 0.0; color: "#060810" }
                    GradientStop { position: 0.5; color: "#080B15" }
                    GradientStop { position: 1.0; color: "#05070C" }
                }
            }

            Item {
                anchors.centerIn: parent
                width: Math.min(460, root.width * 0.48) / 1.3
                height: width * (384.0 / 650.0) / 1.3

                Image {
                    anchors.fill: parent
                    source: "qrc:/qt/qml/LCALauncher/assets/logo.png"
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: true
                }
            }
        }
    }

    // --- Content instance 1 ---
    Loader {
        id: contentLoader1
        active: root.splitActive
        visible: false
        sourceComponent: halfContentComponent
    }

    // --- Content instance 2 ---
    Loader {
        id: contentLoader2
        active: root.splitActive
        visible: false
        sourceComponent: halfContentComponent
    }

    // --- Piece 1 Container (Moves with offsets p1OffsetX, p1OffsetY) ---
    Item {
        id: piece1Container
        x: root.p1OffsetX
        y: root.p1OffsetY
        width: root.width
        height: root.height
        visible: root.splitActive && contentLoader1.item !== null

        MultiEffect {
            anchors.fill: parent
            source: contentLoader1.item
            maskEnabled: true
            maskSource: mask1Item
        }
    }

    // --- Piece 2 Container (Moves with offsets p2OffsetX, p2OffsetY) ---
    Item {
        id: piece2Container
        x: root.p2OffsetX
        y: root.p2OffsetY
        width: root.width
        height: root.height
        visible: root.splitActive && contentLoader2.item !== null

        MultiEffect {
            anchors.fill: parent
            source: contentLoader2.item
            maskEnabled: true
            maskSource: mask2Item
        }
    }

    // =========================================================================
    // 3. PHASE 2: Cutting Beam Animation & Flash
    // =========================================================================

    // Laser slash beam line
    Item {
        id: cutBeamPivot
        x: root.width / 2
        y: root.height / 2
        rotation: root.diagAngleDeg
        visible: root.cutLineProgress > 0.0 && root.cutLineProgress < 1.0

        Rectangle {
            x: -root.diagLength / 2
            y: -1
            width: root.diagLength * root.cutLineProgress
            height: 2
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "#FFFFFF" }
                GradientStop { position: 0.7; color: "#FFFFFF" }
                GradientStop { position: 1.0; color: "#FFFFFF" }
            }
        }

        // Glow head of the laser blade
        Rectangle {
            x: -root.diagLength / 2 + root.diagLength * root.cutLineProgress - 6
            y: -4
            width: 12
            height: 8
            radius: 4
            color: "#FFFFFF"
            opacity: 0.9
        }
    }

    // Cut flash bloom along the diagonal (white only)
    Item {
        id: cutFlashPivot
        x: root.width / 2
        y: root.height / 2
        rotation: root.diagAngleDeg
        visible: root.cutFlashOpacity > 0.01

        Rectangle {
            anchors.centerIn: parent
            width: root.diagLength
            height: 4
            radius: 2
            color: "#FFFFFF"
            opacity: root.cutFlashOpacity * 0.4
        }

        Rectangle {
            anchors.centerIn: parent
            width: root.diagLength
            height: 1
            color: "#FFFFFF"
            opacity: root.cutFlashOpacity
        }
    }

    // =========================================================================
    // 4. ANIMATION TIMELINE CHOREOGRAPHY
    // =========================================================================
    SequentialAnimation {
        id: introSeq
        running: true

        // Initial delay before start
        PauseAnimation { duration: 150 }

        // 1. Logo smoothly fades in
        ParallelAnimation {
            NumberAnimation {
                target: root
                property: "fullLogoOpacity"
                from: 0.0
                to: 1.0
                duration: 300
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                target: root
                property: "fullLogoScale"
                from: 0.90
                to: 1.0
                duration: 700
                easing.type: Easing.OutCubic
            }
        }

        // Pause to display logo clearly in the center
        PauseAnimation { duration: 350 }

        // 2. Diagonal slice laser cuts across
        ParallelAnimation {
            NumberAnimation {
                target: root
                property: "cutLineProgress"
                from: 0.0
                to: 1.0
                duration: 180
                easing.type: Easing.InOutQuad
            }
        }

        // Instant cut trigger: activate split pieces & flash
        ScriptAction {
            script: {
                root.splitActive = true
            }
        }

        // Flash pulse + small initial separation (no shift along the seam)
        ParallelAnimation {
            NumberAnimation {
                target: root
                property: "cutFlashOpacity"
                from: 1.0
                to: 0.0
                duration: 140
                easing.type: Easing.OutQuad
            }
            NumberAnimation {
                target: root
                property: "slideOpen"
                from: 0.0
                to: 12.0
                duration: 220
                easing.type: Easing.OutQuad
            }
        }

        // 3. Main slide: halves part wide toward opposite corners.
        //    slideParallel остаётся 0 — никакого сдвига вдоль шва,
        //    opacity halves не меняется — половинки просто улетают за экран.
        ParallelAnimation {
            NumberAnimation {
                target: root
                property: "slideOpen"
                from: 12.0
                to: 2800.0
                duration: 550
                easing.type: Easing.InCubic
            }
        }

        // Finish
        ScriptAction {
            script: {
                root.isPlaying = false
                root.visible = false
                root.enabled = false
                root.introFinished()
            }
        }
    }
}