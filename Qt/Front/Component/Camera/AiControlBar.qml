import QtQuick
import QtQuick.Controls
import AnoMap.Front

Rectangle {
    id: rootItem

    property string targetCameraId: ""

    // Placeholder AI properties
    property bool intrusionDetectionOn: false
    property bool loiteringDetectionOn: false
    property bool lineCrossingOn: false

    signal sendAiCmd(string cameraId, string feature, bool state)

    color: "transparent"

    Column {
        anchors.fill: parent
        spacing: 12

        Text {
            text: "AI Features Control"
            color: Theme.fontColor
            font.pixelSize: 14
            font.bold: true
        }

        Rectangle {
            width: parent.width
            height: 1
            color: Theme.isDark ? "#33ffffff" : "#33000000"
        }

        Repeater {
            model: [
                {
                    name: "Intrusion Detection",
                    key: "intrusion",
                    active: rootItem.intrusionDetectionOn
                },
                {
                    name: "Loitering Detection",
                    key: "loitering",
                    active: rootItem.loiteringDetectionOn
                },
                {
                    name: "Line Crossing",
                    key: "linecross",
                    active: rootItem.lineCrossingOn
                }
            ]

            // Reusing a similar toggle button style to DeviceControlBar
            Rectangle {
                width: parent.width
                height: 36
                radius: 6
                color: modelData.active ? "#886688ff" : (mouseArea.containsMouse ? "#333355" : "#1a1a2e")
                border.color: modelData.active ? "#6688ff" : "#334466"
                border.width: 1

                Text {
                    id: lblText
                    anchors {
                        left: parent.left
                        leftMargin: 12
                        verticalCenter: parent.verticalCenter
                    }
                    text: modelData.name + (modelData.active ? " [ON]" : " [OFF]")
                    color: "white"
                    font.pixelSize: 12
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        let newState = !modelData.active;
                        if (modelData.key === "intrusion")
                            rootItem.intrusionDetectionOn = newState;
                        else if (modelData.key === "loitering")
                            rootItem.loiteringDetectionOn = newState;
                        else if (modelData.key === "linecross")
                            rootItem.lineCrossingOn = newState;

                        rootItem.sendAiCmd(rootItem.targetCameraId, modelData.key, newState);
                    }
                }
            }
        }
    }
}
