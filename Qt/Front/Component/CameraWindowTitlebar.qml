import QtQuick
import AnoMap.front
import QtQuick.Controls

Item {
    id: root
    height: 48
    required property var window
    // QWindowKit 이 setSystemButton 에 쓸 수 있도록 외부에 노출
    property alias minimizeBtn: minimizeButton
    property alias maximizeBtn: maximizeButton
    property alias closeBtn: closeButton
    Row {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        SystemButton {
            id: minimizeButton
            iconSource: Icon.minimize
            onClicked: root.window.showMinimized()
        }
        SystemButton {
            id: maximizeButton
            iconSource: Icon.maximize
            onClicked: root.window.visibility === Window.Maximized ? root.window.showNormal() : root.window.showMaximized()
        }
        SystemButton {
            id: closeButton
            iconSource: Icon.close
            isClose: true
            onClicked: root.window.close()
        }
    }
}
