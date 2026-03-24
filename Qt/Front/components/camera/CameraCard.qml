import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import AnoMap.Front

// VideoSurface looks up the worker by slotId, so card swaps (URL exchange)
// no longer cause the video to flash or reconnect.
Rectangle {
    id: rootItem

    property int slotId: -1
    property string title: ""
    property string cameraId: ""
    property bool isOnline: false
    property rect cropRect: Qt.rect(0, 0, 1, 1)  // normalised crop from model

    property bool showActionIcon: false
    property string actionIconText: ""
    property bool draggable: false
    property bool highlightOnHover: false

    signal tapped
    signal doubleTapped
    signal actionIconTapped
    signal rightTapped(real x, real y)

    width: 320
    height: 240
    color: "#1e1e1e"
    radius: 8
    border.color: isOnline ? "#4caf50" : "#f44336"
    border.width: 2
    clip: true

    TapHandler {
        onTapped: {
            rootItem.tapped();
        }
        onDoubleTapped: {
            rootItem.doubleTapped();
        }
    }
    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: eventPoint => {
            rootItem.rightTapped(eventPoint.position.x, eventPoint.position.y);
        }
    }
    Item {
        anchors.fill: parent
        clip: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // Title bar
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                color: "#2d2d2d"
                radius: rootItem.radius
                Rectangle {
                    width: parent.width
                    height: rootItem.radius
                    color: parent.color
                    anchors.bottom: parent.bottom
                }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    Text {
                        text: rootItem.title
                        color: "white"
                        font.pixelSize: 14
                        font.bold: true
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                    Item {
                        visible: rootItem.showActionIcon
                        width: 18
                        height: 18
                        Layout.alignment: Qt.AlignVCenter

                        Image {
                            id: actionImg
                            anchors.fill: parent
                            source: Icon.option
                            sourceSize.width: 18
                            sourceSize.height: 18
                            fillMode: Image.PreserveAspectFit
                            visible: false
                        }

                        ColorOverlay {
                            anchors.fill: actionImg
                            source: actionImg
                            color: rootItem.highlightOnHover ? "#88aaff" : "#bbbbcc"
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                resolutionMenu.popup();
                            }
                        }

                        Menu {
                            id: resolutionMenu
                            background: Rectangle {
                                implicitWidth: 120
                                color: Theme.bgSecondary
                                border.color: Theme.hanwhaFirst
                                radius: 4
                            }
                            Repeater {
                                model: typeof settingsController !== "undefined" ? settingsController.resolutions : ["1920x1080"]
                                MenuItem {
                                    text: modelData
                                    contentItem: Text {
                                        text: parent.text
                                        color: parent.highlighted ? Theme.hanwhaFirst : Theme.fontColor
                                        font.pixelSize: 12
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    background: Rectangle {
                                        implicitWidth: 120
                                        implicitHeight: 32
                                        color: parent.highlighted ? Theme.bgThird : "transparent"
                                        radius: 2
                                    }
                                    onTriggered: {
                                        console.log("CameraCard resolution selected:", modelData);
                                        rootItem.actionIconTapped();
                                    }
                                }
                            }
                        }
                    }
                    Rectangle {
                        width: 10
                        height: 10
                        radius: 5
                        color: rootItem.isOnline ? "#4caf50" : "#f44336"
                    }
                }
            }

            // Video area
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "black"

                Loader {
                    id: videoLoader
                    anchors.fill: parent
                    active: rootItem.isOnline && rootItem.cameraId !== ""
                    visible: status === Loader.Ready
                    sourceComponent: VideoSurface {
                        slotId: rootItem.slotId
                        cameraId: rootItem.cameraId
                        cropRect: rootItem.cropRect  // crop region for split tiles
                    }
                }

                Text {
                    anchors.centerIn: parent
                    text: rootItem.isOnline ? "Loading..." : "OFFLINE"
                    color: rootItem.isOnline ? "#8888aa" : "#ff5555"
                    font.pixelSize: 14
                    font.bold: true
                    visible: !videoLoader.visible
                }
            }
        }
    }
}
