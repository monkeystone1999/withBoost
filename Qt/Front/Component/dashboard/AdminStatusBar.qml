import QtQuick
import AnoMap.front

Rectangle {
    id: root
    width: 280
    height: 120
    color: "#aa000000"
    border.color: "#88ffffff"
    border.width: 1
    radius: 6

    // ── 상태 ──────────────────────────────────────
    property bool isAdmin: false
    property bool hasServer: typeof serverStatusModel !== "undefined" && serverStatusModel.hasServer
    property real cpu: hasServer ? serverStatusModel.serverCpu : 0
    property real mem: hasServer ? serverStatusModel.serverMemory : 0
    property real temp: hasServer ? serverStatusModel.serverTemp : 0

    visible: isAdmin

    signal listPendingRequested
    signal approveTargetRequested

    Column {
        anchors.centerIn: parent
        spacing: 8
        Text {
            text: qsTr("Admin Dashboard (Server Status)")
            color: "white"
            font.bold: true
        }
        Text {
            text: root.hasServer ? qsTr("CPU: %1% | Mem: %2% | Temp: %3°C").arg(root.cpu.toFixed(1)).arg(root.mem.toFixed(1)).arg(root.temp.toFixed(1)) : qsTr("Server Status: Not Available")
            color: "#aaffaa"
            font.pixelSize: 12
        }
        Row {
            spacing: 10
            BasicButton {
                text: "List Pending"
                height: 30
                onClicked: root.listPendingRequested()
            }
            BasicButton {
                text: "Approve Target"
                height: 30
                onClicked: root.approveTargetRequested()
            }
        }
    }
}
