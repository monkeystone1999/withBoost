import QtQuick
import AnoMap.front
import QtQuick.Controls

Item {
    id: root
    height: 48
    required property var window
    property string windowTitle: ""
    // QWindowKit 이 setSystemButton 에 쓸 수 있도록 외부에 노출
    property alias minimizeBtn: minimizeButton
    property alias maximizeBtn: maximizeButton
    property alias closeBtn: closeButton

    // 타이틀 텍스트
    Text {
        anchors.centerIn: parent
        text: root.windowTitle
        color: Theme.fontColor
        font.family: Typography.fontFamilySecondary
        font.pixelSize: Typography.teriaryFont
        font.bold: true
        elide: Text.ElideRight
        width: parent.width * 0.6
        horizontalAlignment: Text.AlignHCenter
    }
    Row {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        SystemButton {
            id: minimizeButton
            iconSource: Icon.minimize
            bgColor: "transparent"
            onClicked: root.window.showMinimized()
        }
        SystemButton {
            id: maximizeButton
            iconSource: Icon.maximize
            bgColor: "transparent"
            onClicked: root.window.visibility === Window.Maximized ? root.window.showNormal() : root.window.showMaximized()
        }
        SystemButton {
            id: closeButton
            iconSource: Icon.close
            bgColor: "transparent"
            isClose: true
            onClicked: root.window.close()
        }
    }
}
