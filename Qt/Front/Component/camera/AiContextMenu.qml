import QtQuick
import AnoMap.front

Rectangle {
    id: root
    width: 210
    radius: 7
    color: "#1e1e2e"
    border.color: "#55aaaaff"
    border.width: 1
    height: col.implicitHeight + 12

    property string targetUrl: ""

    signal aiControlRequested(string url)
    signal viewInDeviceTabRequested(string url)

    Column {
        id: col
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            margins: 6
        }
        spacing: 2

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
