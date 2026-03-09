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

    property int sourceSlotId: -1
    property string sourceTitle: ""
    property string sourceRtspUrl: ""
    property bool sourceOnline: false
    property rect sourceCropRect: Qt.rect(0, 0, 1, 1)

    readonly property int roleSlotId: 257
    readonly property int roleTitle: 258
    readonly property int roleRtspUrl: 259
    readonly property int roleIsOnline: 260
    readonly property int roleCropRect: 266

    function modelRowBySlotId() {
        if (sourceSlotId < 0)
            return -1;
        const rows = cameraModel.rowCount();
        for (let i = 0; i < rows; ++i) {
            const sid = cameraModel.data(cameraModel.index(i, 0), roleSlotId);
            if (sid === sourceSlotId)
                return i;
        }
        return -1;
    }

    function refreshModelRow() {
        const row = modelRowBySlotId();
        if (root.modelRow !== row) {
            root.modelRow = row;
        }
    }

    property int modelRow: modelRowBySlotId()

    Connections {
        target: cameraModel
        function onRowsMoved() {
            root.refreshModelRow();
        }
        function onModelReset() {
            root.refreshModelRow();
        }
        function onRowsInserted() {
            root.refreshModelRow();
        }
        function onRowsRemoved() {
            root.refreshModelRow();
        }
    }

    property string title: modelRow >= 0 ? (cameraModel.data(cameraModel.index(modelRow, 0), roleTitle) || sourceTitle) : sourceTitle
    property string rtspUrl: modelRow >= 0 ? (cameraModel.data(cameraModel.index(modelRow, 0), roleRtspUrl) || sourceRtspUrl) : sourceRtspUrl
    property bool isOnline: modelRow >= 0 ? (cameraModel.data(cameraModel.index(modelRow, 0), roleIsOnline) || false) : sourceOnline
    property rect cropRect: modelRow >= 0 ? (cameraModel.data(cameraModel.index(modelRow, 0), roleCropRect) || sourceCropRect) : sourceCropRect

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
