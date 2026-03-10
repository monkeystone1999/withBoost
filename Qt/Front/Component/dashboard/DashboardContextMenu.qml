import QtQuick
import AnoMap.front

Rectangle {
    id: root
    width: 190
    radius: 7
    color: "#1e1e2e"
    border.color: "#55aaaaff"
    border.width: 1
    height: ctxCol.implicitHeight + 12

    property string targetUrl: ""
    property int targetSlotId: -1
    property int targetIndex: -1
    property int targetSplit: 1
    property bool targetOnline: false

    signal deviceControlRequested(string url)
    signal aiControlRequested(string url)
    signal viewInDeviceTabRequested(string url)
    signal viewInAiTabRequested(string url)
    signal splitRequested(int index, int n)
    signal mergeRequested(int index, int currentSplit)

    Column {
        id: ctxCol
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            margins: 6
        }
        spacing: 2

        CtxItem {
            label: "Device 제어"
            itemEnabled: typeof deviceModel !== "undefined" && deviceModel.hasDevice(root.targetUrl)
            onTriggered: root.deviceControlRequested(root.targetUrl)
        }
        CtxItem {
            label: "AI 제어"
            itemEnabled: true
            onTriggered: root.aiControlRequested(root.targetUrl)
        }
        Rectangle {
            width: parent.width - 12
            height: 1
            color: "#33ffffff"
            anchors.horizontalCenter: parent.horizontalCenter
        }
        CtxItem {
            label: "Device 탭에서 보기"
            itemEnabled: typeof deviceModel !== "undefined" && deviceModel.hasDevice(root.targetUrl)
            onTriggered: root.viewInDeviceTabRequested(root.targetUrl)
        }
        CtxItem {
            label: "AI 탭에서 보기"
            itemEnabled: true
            onTriggered: root.viewInAiTabRequested(root.targetUrl)
        }
        Rectangle {
            width: parent.width - 12
            height: 1
            color: "#33ffffff"
            anchors.horizontalCenter: parent.horizontalCenter
        }
        CtxItem {
            label: "Split 2"
            itemEnabled: root.targetSplit === 1 && root.targetOnline
            onTriggered: root.splitRequested(root.targetIndex, 2)
        }
        CtxItem {
            label: "Split 3"
            itemEnabled: root.targetSplit === 1 && root.targetOnline
            onTriggered: root.splitRequested(root.targetIndex, 3)
        }
        CtxItem {
            label: "Split 4"
            itemEnabled: root.targetSplit === 1 && root.targetOnline
            onTriggered: root.splitRequested(root.targetIndex, 4)
        }
        CtxItem {
            label: "Merge"
            itemEnabled: root.targetSplit > 1
            onTriggered: root.mergeRequested(root.targetIndex, root.targetSplit)
        }
    }

    component CtxItem: Rectangle {
        property string label: ""
        property bool itemEnabled: true
        signal triggered
        width: parent.width
        height: 32
        radius: 4
        color: itemEnabled && ma.containsMouse ? "#336688aa" : "transparent"
        Text {
            anchors {
                left: parent.left
                leftMargin: 12
                verticalCenter: parent.verticalCenter
            }
            text: parent.label
            color: parent.itemEnabled ? "white" : "#44556688"
            font.pixelSize: 12
        }
        MouseArea {
            id: ma
            anchors.fill: parent
            hoverEnabled: true
            enabled: parent.itemEnabled
            onClicked: {
                root.visible = false;
                parent.triggered();
            }
        }
    }
}
