import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QWindowKit
import AnoMap.Front

Window {
    id: rootItem
    flags: Qt.Window | Qt.FramelessWindowHint
    color: "transparent"
    visible: true

    Component.onCompleted: {
        if (typeof appController !== "undefined")
            appController.registerCameraWindow(rootItem);
    }
    Component.onDestruction: {
        if (typeof appController !== "undefined")
            appController.unregisterCameraWindow(rootItem);
    }

    property int sourceSlotId: -1
    property string sourceTitle: ""
    property string sourceCameraId: ""
    property bool sourceOnline: false
    property rect sourceCropRect: Qt.rect(0, 0, 1, 1)  // crop for split tiles

    property int updateTrigger: 0

    Connections {
        target: cameraModel
        function onRowsMoved() {
            rootItem.updateTrigger++;
        }
        function onModelReset() {
            rootItem.updateTrigger++;
        }
        function onRowsInserted() {
            rootItem.updateTrigger++;
        }
        function onRowsRemoved() {
            rootItem.updateTrigger++;
        }
        function onDataChanged(topLeft, bottomRight) {
            var myRow = cameraModel.rowForSlot(rootItem.sourceSlotId);
            if (myRow >= 0 && myRow >= topLeft.row && myRow <= bottomRight.row)
                rootItem.updateTrigger++;
        }
    }

    property string title: {
        var dummy = updateTrigger;
        if (rootItem.sourceSlotId < 0 || !cameraModel.hasSlot(rootItem.sourceSlotId))
            return sourceTitle;
        var t = cameraModel.titleForSlot(rootItem.sourceSlotId);
        return t ? t : sourceTitle;
    }
    property string cameraId: {
        var dummy = updateTrigger;
        if (rootItem.sourceSlotId < 0 || !cameraModel.hasSlot(rootItem.sourceSlotId))
            return sourceCameraId;
        var u = cameraModel.cameraIdForSlot(rootItem.sourceSlotId);
        return u ? u : sourceCameraId;
    }
    property bool isOnline: {
        var dummy = updateTrigger;
        if (rootItem.sourceSlotId < 0 || !cameraModel.hasSlot(rootItem.sourceSlotId))
            return sourceOnline;
        return cameraModel.isOnlineForSlot(rootItem.sourceSlotId);
    }
    property rect cropRect: {
        var dummy = updateTrigger;
        if (rootItem.sourceSlotId < 0 || !cameraModel.hasSlot(rootItem.sourceSlotId))
            return sourceCropRect;
        return cameraModel.cropRectForSlot(rootItem.sourceSlotId);
    }

    width: 640
    height: 480

    HoverHandler {
        id: hoverHandler
    }

    WindowAgent {
        id: agent
        Component.onCompleted: {
            agent.setup(rootItem);
            agent.setTitleBar(camTitleBar);
            agent.setSystemButton(WindowAgent.Close, camTitleBar.closeButtonItem);
            agent.setSystemButton(WindowAgent.Maximize, camTitleBar.maximizeButtonItem);
            agent.setSystemButton(WindowAgent.Minimize, camTitleBar.minimizeButtonItem);
        }
    }

    CameraWindowTitleBar {
        id: camTitleBar
        z: 100
        y: hoverHandler.hovered ? 0 : -height
        window: rootItem
        windowTitle: rootItem.title
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
        border.color: rootItem.isOnline ? "#4caf50" : "#f44336"
        border.width: 2

        Loader {
            anchors.fill: parent
            active: rootItem.isOnline && rootItem.cameraId !== ""
            sourceComponent: VideoSurface {
                slotId: rootItem.sourceSlotId
                cameraId: rootItem.cameraId
                cropRect: rootItem.cropRect
            }
        }

        Rectangle {
            anchors.fill: parent
            color: "transparent"
            visible: !rootItem.isOnline

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
                text: rootItem.title
                color: "white"
                font.pixelSize: 13
                font.bold: true
            }
        }
    }
}
