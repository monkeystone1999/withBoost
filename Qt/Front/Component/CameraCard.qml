import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.front

// ── CameraCard ────────────────────────────────────────────────────────────────
// §3 VIDEO_STREAMING_SPEC: now carries slotId (permanent virtual identity).
// VideoSurface looks up the worker by slotId, so card swaps (URL exchange)
// no longer cause the video to flash or reconnect.
// ─────────────────────────────────────────────────────────────────────────────
Rectangle {
    id: root

    property int slotId: -1      // §3: permanent grid-position identity
    property string title: ""
    property string rtspUrl: ""
    property bool isOnline: false
    property rect cropRect: Qt.rect(0, 0, 1, 1)   // §4: UV crop (full frame default)

    property bool ctrlVisible: false
    signal controlRequested

    width: 320
    height: 240
    color: "#1e1e1e"
    radius: 8
    border.color: isOnline ? "#4caf50" : "#f44336"
    border.width: 2
    clip: true

    TapHandler {
        onTapped: {
            if (!deviceModel.hasDevice(root.rtspUrl))
                return;
            root.controlRequested();
        }
    }

<<<<<<< Updated upstream
    // 카드 내부는 clip true 로 별도 관리
=======
    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: eventPoint => {
            root.rightClicked(eventPoint.position.x, eventPoint.position.y);
        }
    }

>>>>>>> Stashed changes
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
                radius: root.radius
                Rectangle {
                    width: parent.width
                    height: root.radius
                    color: parent.color
                    anchors.bottom: parent.bottom
                }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    Text {
                        text: root.title
                        color: "white"
                        font.pixelSize: 14
                        font.bold: true
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
<<<<<<< Updated upstream
                    // 디바이스 제어 가능 표시
=======
>>>>>>> Stashed changes
                    Text {
                        visible: deviceModel.hasDevice(root.rtspUrl)
                        text: "⚙"
<<<<<<< Updated upstream
                        color: root.ctrlVisible ? "#88aaff" : "#555577"
                        font.pixelSize: 13
=======
                        color: root.ctrlVisible ? "#88aaff" : "#bbbbcc"
                        font.pixelSize: 18
                        MouseArea {
                            anchors.fill: parent
                            anchors.margins: -5
                            onClicked: root.rightClicked(width / 2, height / 2)
                        }
>>>>>>> Stashed changes
                    }
                    Rectangle {
                        width: 10
                        height: 10
                        radius: 5
                        color: root.isOnline ? "#4caf50" : "#f44336"
                    }
                }
            }

<<<<<<< Updated upstream
            // 비디오
            Loader {
                Layout.fillWidth: true
                Layout.fillHeight: true
                active: root.isOnline && root.rtspUrl !== ""
                visible: status === Loader.Ready
                sourceComponent: VideoSurface {
                    rtspUrl: root.rtspUrl
=======
            // Video area
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "black"

                Loader {
                    id: videoLoader
                    anchors.fill: parent
                    active: root.isOnline && root.rtspUrl !== ""
                    visible: status === Loader.Ready
                    sourceComponent: VideoSurface {
                        slotId: root.slotId    // §3: primary key
                        rtspUrl: root.rtspUrl   // legacy fallback
                        cropRect: root.cropRect  // §4: GPU tile crop
                    }
                }

                Text {
                    anchors.centerIn: parent
                    text: root.isOnline ? "Loading..." : "OFFLINE"
                    color: root.isOnline ? "#8888aa" : "#ff5555"
                    font.pixelSize: 14
                    font.bold: true
                    visible: !videoLoader.visible
>>>>>>> Stashed changes
                }
            }
        }
    }
}
