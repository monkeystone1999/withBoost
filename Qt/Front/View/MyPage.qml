import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.front

Item {
    id: root

    // Page Name Property
    property string pageName: "MyPage"
    signal requestPage(string pageName)
    signal requestClose

    Rectangle {
        anchors.fill: parent
        color: Theme.bgPrimary

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 20

            // 로고 또는 아이콘 영역 (옵션)
            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                width: 80
                height: 80
                radius: 40
                color: Theme.bgSecondary

                Text {
                    anchors.centerIn: parent
                    text: loginController.username.length > 0 ? loginController.username.substring(0, 1).toUpperCase() : "?"
                    color: Theme.bgPrimary
                    font.family: Typography.fontFamilyPrimary
                    font.pixelSize: 36
                    font.weight: Font.Bold
                }
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("환영합니다, %1 님").arg(loginController.username)
                color: Theme.fontColor
                font.family: Typography.fontFamilyPrimary
                font.pixelSize: 28
                font.weight: Font.Bold
            }

            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                width: parent.width * 0.8
                height: 1
                color: Theme.bgSecondary
                opacity: 0.3
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 15

                Text {
                    text: qsTr("권한 그룹:")
                    color: Theme.fontColor
                    font.family: Typography.fontFamilyPrimary
                    font.pixelSize: 18
                }

                Rectangle {
                    color: loginController.state === "admin" ? Theme.hanwhaFirst : Theme.bgSecondary
                    radius: 4
                    width: groupText.implicitWidth + 16
                    Layout.preferredHeight: 30

                    Text {
                        id: groupText
                        anchors.centerIn: parent
                        text: loginController.state.toUpperCase()
                        color: Theme.bgPrimary
                        font.family: Typography.fontFamilyPrimary
                        font.pixelSize: 14
                        font.weight: Font.Bold
                    }
                }
            }
        }
    }
}
