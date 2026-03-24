import QtQuick
import AnoMap.Front

Rectangle {
    id: rootItem
    width: 280
    height: 120
    color: "#aa000000"
    border.color: "#88ffffff"
    border.width: 1
    radius: 6

    property bool isAdmin: false
    property bool hasServer: typeof serverStatusModel !== "undefined" && serverStatusModel.hasServer
    property real cpu: visible && hasServer ? serverStatusModel.serverCpu : 0
    property real mem: visible && hasServer ? serverStatusModel.serverMemory : 0
    property real temp: visible && hasServer ? serverStatusModel.serverTemp : 0

    visible: isAdmin

    signal listPendingRequested
    signal approveTargetRequested

    Column {
        anchors.centerIn: parent
        spacing: 8
        Text {
            text: "Admin Dashboard (Server Status)"
            color: "white"
            font.bold: true
        }
        Text {
            text: rootItem.hasServer ? qsTr("CPU: %1% | Mem: %2% | Temp: %3¡ÆC").arg(rootItem.cpu.toFixed(1)).arg(rootItem.mem.toFixed(1)).arg(rootItem.temp.toFixed(1)) : "No Server Info"
            color: "#aaffaa"
            font.pixelSize: 12
        }
        Row {
            spacing: 10
            BasicButton {
                text: "Pending List"
                height: 30
                onClicked: rootItem.listPendingRequested()
            }
            BasicButton {
                text: "Approve"
                height: 30
                onClicked: rootItem.approveTargetRequested()
            }
        }
    }
}
