import QtQuick
import AnoMap.Front
import QtQuick.Controls

Item {
    id: rootItem
    height: 48
    required property var window
    property string windowTitle: ""
    property alias minimizeButtonItem: minimizeButton // system buttons
    property alias maximizeButtonItem: maximizeButton
    property alias closeButtonItem: closeButton

    Text {
        anchors.centerIn: parent
        text: rootItem.windowTitle
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
            onClicked: rootItem.window.showMinimized()
        }
        SystemButton {
            id: maximizeButton
            iconSource: Icon.maximize
            onClicked: rootItem.window.visibility === Window.Maximized ? rootItem.window.showNormal() : rootItem.window.showMaximized()
        }
        SystemButton {
            id: closeButton
            iconSource: Icon.close
            isClose: true
            onClicked: rootItem.window.close()
        }
    }
}
