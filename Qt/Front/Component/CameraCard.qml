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

    width: 320
    height: 240
    color: "#1e1e1e"
    radius: 8
    border.color: isOnline ? "#4caf50" : "#f44336"
    border.width: 2
    clip: true    // 내부 요소가 벗어나지 않도록 (이제 controlBar는 외부에 있음)

    // 클릭 = 컨트롤러 토글 (드래그와 구분: TapHandler 는 이동 없을 때만 발동)
    TapHandler {
        onTapped: {
            console.log("[CameraCard] tapped, hasDevice:", deviceModel.hasDevice(root.rtspUrl), "rtspUrl:", root.rtspUrl);
            if (!deviceModel.hasDevice(root.rtspUrl))
                return;
            root.controlRequested();
        }
    }

    // 카드 내부는 clip true 로 별도 관리
    Item {
        anchors.fill: parent
        clip: true

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
                    // 디바이스 제어 가능 표시
                    Text {
                        visible: deviceModel.hasDevice(root.rtspUrl)
                        text: "⚙"
                        color: root.ctrlVisible ? "#88aaff" : "#555577"
                        font.pixelSize: 13
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
            Loader {
                Layout.fillWidth: true
                Layout.fillHeight: true
                active: root.isOnline && root.rtspUrl !== ""
                visible: status === Loader.Ready
                sourceComponent: VideoSurface {
                    rtspUrl: root.rtspUrl
                }
            }
        }
    }
}
