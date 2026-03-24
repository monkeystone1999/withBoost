import QtQuick
import AnoMap.Front

Rectangle {
    id: rootItem
    width: 210
    radius: 7
    color: Theme.bgSecondary
    border.color: Theme.hanwhaFirst
    border.width: 1
    height: ctxCol.implicitHeight + 12

    property string targetCameraId: ""
    property int targetSlotId: -1
    property int targetIndex: -1
    property int targetSplit: 1
    property bool targetOnline: false

    signal deviceControlRequested(string cameraId)
    signal aiControlRequested(string cameraId)
    signal viewInDeviceTabRequested(string cameraId)
    signal viewInAiTabRequested(string cameraId)
    signal splitRequested(int index, int n, int direction)
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
            label: "View in Device Tab"
            itemEnabled: typeof deviceModel !== "undefined" && deviceModel.hasDeviceByCameraId(rootItem.targetCameraId)
            onTriggered: rootItem.viewInDeviceTabRequested(rootItem.targetCameraId)
        }
        CtxItem {
            label: "View in AI Tab"
            itemEnabled: true
            onTriggered: rootItem.viewInAiTabRequested(rootItem.targetCameraId)
        }
        Rectangle {
            width: parent.width - 12
            height: 1
            color: "#33ffffff"
            anchors.horizontalCenter: parent.horizontalCenter
        }
        // -- Column Split --
        CtxSectionLabel {
            label: "Column"
        }
        CtxItem {
            label: "Col-2"
            itemEnabled: rootItem.targetSplit === 1 && rootItem.targetOnline
            onTriggered: rootItem.splitRequested(rootItem.targetIndex, 2, 0)
        }
        CtxItem {
            label: "Col-3"
            itemEnabled: rootItem.targetSplit === 1 && rootItem.targetOnline
            onTriggered: rootItem.splitRequested(rootItem.targetIndex, 3, 0)
        }
        Rectangle {
            width: parent.width - 12
            height: 1
            color: "#33ffffff"
            anchors.horizontalCenter: parent.horizontalCenter
        }
        // -- Row Split --
        CtxSectionLabel {
            label: "Row"
        }
        CtxItem {
            label: "Row-2"
            itemEnabled: rootItem.targetSplit === 1 && rootItem.targetOnline
            onTriggered: rootItem.splitRequested(rootItem.targetIndex, 2, 1)
        }
        CtxItem {
            label: "Row-3"
            itemEnabled: rootItem.targetSplit === 1 && rootItem.targetOnline
            onTriggered: rootItem.splitRequested(rootItem.targetIndex, 3, 1)
        }
        Rectangle {
            width: parent.width - 12
            height: 1
            color: "#33ffffff"
            anchors.horizontalCenter: parent.horizontalCenter
        }
        // -- Grid Split --
        CtxSectionLabel {
            label: "Grid"
        }
        CtxItem {
            label: "Grid-4 (2x2)"
            itemEnabled: rootItem.targetSplit === 1 && rootItem.targetOnline
            onTriggered: rootItem.splitRequested(rootItem.targetIndex, 4, 2)
        }
        Rectangle {
            width: parent.width - 12
            height: 1
            color: "#33ffffff"
            anchors.horizontalCenter: parent.horizontalCenter
        }
        // -- Merge --
        CtxItem {
            label: "Merge"
            itemEnabled: rootItem.targetSplit > 1
            onTriggered: rootItem.mergeRequested(rootItem.targetIndex, rootItem.targetSplit)
        }
    }

    component CtxSectionLabel: Rectangle {
        property string label: ""
        width: parent.width
        height: 20
        color: "transparent"
        Text {
            anchors {
                left: parent.left
                leftMargin: 12
                verticalCenter: parent.verticalCenter
            }
            text: parent.label
            color: "#7799bb"
            font.pixelSize: 10
            font.bold: true
            font.capitalization: Font.AllUppercase
            font.letterSpacing: 1
        }
    }

    component CtxItem: Rectangle {
        property string label: ""
        property bool itemEnabled: true
        signal triggered
        width: parent.width
        height: 32
        radius: 4
        color: itemEnabled && ma.containsMouse ? Theme.bgThird : "transparent"
        Text {
            anchors {
                left: parent.left
                leftMargin: 12
                verticalCenter: parent.verticalCenter
            }
            text: parent.label
            color: parent.itemEnabled ? Theme.fontColor : "#44556688"
            font.pixelSize: 12
        }
        MouseArea {
            id: ma
            anchors.fill: parent
            hoverEnabled: true
            enabled: parent.itemEnabled
            onClicked: {
                rootItem.visible = false;
                parent.triggered();
            }
        }
    }
}
