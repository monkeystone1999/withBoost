import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QWindowKit
import AnoMap.front

Window {
    id: root
    flags: Qt.Window | Qt.FramelessWindowHint
    color: "transparent"
    visible: true

    Component.onCompleted: {
        // Main 윗도우의 openCameraWindows 목록에 등록
        if (typeof root.Window !== "undefined" && root.transientParent !== null && typeof root.transientParent.registerCameraWindow === "function") {
            root.transientParent.registerCameraWindow(root);
        }
    }
    Component.onDestruction: {
        if (typeof root.transientParent !== "undefined" && root.transientParent !== null && typeof root.transientParent.unregisterCameraWindow === "function") {
            root.transientParent.unregisterCameraWindow(root);
        }
    }

    property int sourceSlotId: -1
    property string sourceTitle: ""
    property string sourceRtspUrl: ""
    property bool sourceOnline: false
    property rect sourceCropRect: Qt.rect(0, 0, 1, 1)

    property int updateTrigger: 0

    Connections {
        target: cameraModel
        function onRowsMoved() {
            root.updateTrigger++;
        }
        function onModelReset() {
            root.updateTrigger++;
        }
        function onRowsInserted() {
            root.updateTrigger++;
        }
        function onRowsRemoved() {
            root.updateTrigger++;
        }
        function onDataChanged() {
            root.updateTrigger++;
        }
    }

    property string title: {
        var dummy = updateTrigger;
        if (root.sourceSlotId < 0 || !cameraModel.hasSlot(root.sourceSlotId))
            return sourceTitle;
        var t = cameraModel.titleForSlot(root.sourceSlotId);
        return t ? t : sourceTitle;
    }
    property string rtspUrl: {
        var dummy = updateTrigger;
        if (root.sourceSlotId < 0 || !cameraModel.hasSlot(root.sourceSlotId))
            return sourceRtspUrl;
        var u = cameraModel.rtspUrlForSlot(root.sourceSlotId);
        return u ? u : sourceRtspUrl;
    }
    property bool isOnline: {
        var dummy = updateTrigger;
        if (root.sourceSlotId < 0 || !cameraModel.hasSlot(root.sourceSlotId))
            return sourceOnline;
        return cameraModel.isOnlineForSlot(root.sourceSlotId);
    }
    property rect cropRect: {
        var dummy = updateTrigger;
        if (root.sourceSlotId < 0 || !cameraModel.hasSlot(root.sourceSlotId))
            return sourceCropRect;
        return cameraModel.cropRectForSlot(root.sourceSlotId);
    }

    width: 640
    height: 480

    HoverHandler {
        id: hoverHandler
    }

    WindowAgent {
        id: agent
        Component.onCompleted: {
            agent.setup(root);
            agent.setTitleBar(camTitlebar);
            agent.setSystemButton(WindowAgent.Close, camTitlebar.closeBtn);
            agent.setSystemButton(WindowAgent.Maximize, camTitlebar.maximizeBtn);
            agent.setSystemButton(WindowAgent.Minimize, camTitlebar.minimizeBtn);
        }
    }

    CameraWindowTitlebar {
        id: camTitlebar
        z: 100
        y: hoverHandler.hovered ? 0 : -height
        window: root
        windowTitle: root.title
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        Behavior on y {
            NumberAnimation {
                duration: 200
                easing.type: Easing.OutQuad
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#1e1e1e"
        radius: 8
        clip: true
        border.color: root.isOnline ? "#4caf50" : "#f44336"
        border.width: 2

        Loader {
            anchors.fill: parent
            active: root.isOnline && root.rtspUrl !== ""
            sourceComponent: VideoSurface {
                slotId: root.sourceSlotId
                rtspUrl: root.rtspUrl
                cropRect: root.cropRect
            }
        }

        Rectangle {
            anchors.fill: parent
            color: "transparent"
            visible: !root.isOnline

            Text {
                anchors.centerIn: parent
                text: "OFFLINE"
                color: "#f44336"
                font.pixelSize: 28
                font.bold: true
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.margins: 8
            width: titleLabel.width + 16
            height: 28
            radius: 6
            color: "#88000000"
            visible: !hoverHandler.hovered

            Text {
                id: titleLabel
                anchors.centerIn: parent
                text: root.title
                color: "white"
                font.pixelSize: 13
                font.bold: true
            }
        }
    }
}
