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
            text: qsTr("관리자 (서버 상태)")  // Changed from "Admin Dashboard (Server Status)"
            color: "white"
            font.bold: true
        }
        Text {
            text: root.hasServer ? qsTr("CPU: %1% | Mem: %2% | Temp: %3°C").arg(root.cpu.toFixed(1)).arg(root.mem.toFixed(1)).arg(root.temp.toFixed(1)) : qsTr("서버 상태: 사용 불가")  // Changed to Korean
            color: "#aaffaa"
            font.pixelSize: 12
        }
        Row {
            spacing: 10
            BasicButton {
                text: "대기 목록"  // Changed from "List Pending"
                height: 30
                onClicked: root.listPendingRequested()
            }
            BasicButton {
                text: "승인"  // Changed from "Approve Target"
                height: 30
                onClicked: root.approveTargetRequested()
            }
        }
    }
}
