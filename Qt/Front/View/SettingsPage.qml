import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.front

Item {
    id: root
    signal requestClose

    // 딤 배경
    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: 0.5
        MouseArea {
            anchors.fill: parent
            // 뒤 클릭 차단 및 다른 UI 조작 방지
        }
    }

    // 설정 패널
    Rectangle {
        width: 400
        height: 500
        anchors.centerIn: parent
        color: Theme.bgSecondary
        radius: 12

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 30

            Text {
                text: "Settings"
                color: Theme.fontColor
                font.pixelSize: 28
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
                Layout.bottomMargin: 10
            }

            RowLayout {
                spacing: 20
                Layout.alignment: Qt.AlignHCenter
                Text {
                    text: "Dark Mode"
                    color: Theme.fontColor
                    font.pixelSize: 18
                }
                Switch {
                    checked: Theme.isDark
                    onToggled: Theme.isDark = !Theme.isDark
                }
            }

            Item {
                Layout.preferredHeight: 30
                Layout.preferredWidth: 1
            }

            BasicButton {
                text: "Close"
                Layout.alignment: Qt.AlignHCenter
                onClicked: root.requestClose()
            }
        }
    }
}
