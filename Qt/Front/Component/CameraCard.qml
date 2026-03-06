import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.front

// ── CameraCard ────────────────────────────────────────────────────────────────
// - Click → DeviceControlBar floating 토글 (deviceModel 에 해당 rtspUrl 있을 때만)
// - Drag  → DashboardPage 의 DragHandler 가 처리 (카드 자체는 passive)
// ─────────────────────────────────────────────────────────────────────────────
Rectangle {
    id: root

    property string title: ""
    property string rtspUrl: ""
    property bool isOnline: false

    // 컨트롤러 바 표시 상태
    property bool ctrlVisible: false
    signal controlRequested
    signal rightClicked(real sceneX, real sceneY)

    width: 320
    height: 240
    color: "#1e1e1e"
    radius: 8
    border.color: isOnline ? "#4caf50" : "#f44336"
    border.width: 2
    clip: true    // 내부 요소가 벗어나지 않도록 (이제 controlBar는 외부에 있음)

    // 클릭 = 컨트롤러 토글 (드래그와 구분: TapHandler 는 이동 없을 때만 발동)
    TapHandler {
        acceptedButtons: Qt.LeftButton
        onTapped: {
            console.log("[CameraCard] tapped, hasDevice:", deviceModel.hasDevice(root.rtspUrl), "rtspUrl:", root.rtspUrl);
            if (!deviceModel.hasDevice(root.rtspUrl))
                return;
            root.controlRequested();
        }
    }

    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: eventPoint => {
            root.rightClicked(eventPoint.position.x, eventPoint.position.y);
        }
    }

    // 카드 내부는 clip true 로 별도 관리하지 않음 (루트에서 clip)
    Item {
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // 타이틀바
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                color: "#2d2d2d"
                radius: root.radius
                // 하단 모서리 채우기
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
                    // 설정(톱니바퀴) 아이콘
                    Text {
                        text: "⚙"
                        color: root.ctrlVisible ? "#88aaff" : "#bbbbcc"
                        font.pixelSize: 18

                        MouseArea {
                            anchors.fill: parent
                            anchors.margins: -5
                            onClicked: root.rightClicked(width / 2, height / 2)
                        }
                    }
                    Rectangle {
                        width: 10
                        height: 10
                        radius: 5
                        color: root.isOnline ? "#4caf50" : "#f44336"
                    }
                }
            }

            // 비디오
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
                        rtspUrl: root.rtspUrl
                    }
                }

                Text {
                    anchors.centerIn: parent
                    text: root.isOnline ? "Loading..." : "OFFLINE"
                    color: root.isOnline ? "#8888aa" : "#ff5555"
                    font.pixelSize: 14
                    font.bold: true
                    visible: !videoLoader.visible
                }
            }
        }
    }
}
