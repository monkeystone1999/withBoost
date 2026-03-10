import QtQuick
import AnoMap.front

Row {
    id: root
    spacing: 10

    // ── 입력/출력 ─────────────────────────────────
    property int currentValue: 4
    property var options: [2, 3, 4, 5, 6]
    signal valueChanged(int newValue)

    Text {
        text: "Grid Columns :"
        color: "white"
        font.pixelSize: 14
        anchors.verticalCenter: parent.verticalCenter
    }

    Repeater {
        model: root.options
        delegate: Rectangle {
            width: 30
            height: 24
            radius: 4
            color: root.currentValue === modelData ? Theme.hanwhaFirst : "#333333"
            border.color: "#555555"
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: modelData
                color: "white"
                font.pixelSize: 13
                font.bold: true
            }

            MouseArea {
                anchors.fill: parent
                onClicked: root.valueChanged(modelData)
            }
        }
    }
}
