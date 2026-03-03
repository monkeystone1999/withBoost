import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QWindowKit
import AnoMap.front
import AnoMap.back

Window {
    id: root
    flags: Qt.Window | Qt.FramelessWindowHint
    color: "transparent"
    visible: true

    // 카드에서 전달받는 속성
    property string title:    ""
    property string rtspUrl:  ""
    property bool   isOnline: false
    property int    sourceCardIndex: -1  // 어느 카드에서 분리됐는지

    width:  640
    height: 480

    HoverHandler { id: hoverHandler }

    // QWindowKit 프레임리스 설정
    WindowAgent {
        id: agent
        Component.onCompleted: {
            agent.setup(root)
            agent.setTitleBar(camTitlebar)
            agent.setSystemButton(WindowAgent.Close,    camTitlebar.closeBtn)
            agent.setSystemButton(WindowAgent.Maximize, camTitlebar.maximizeBtn)
            agent.setSystemButton(WindowAgent.Minimize, camTitlebar.minimizeBtn)
        }
    }

    // ── 타이틀바 (hover 시 나타남) ─────────────────────────────────────────
    CameraWindowTitlebar {
        id: camTitlebar
        z: 100
        y: hoverHandler.hovered ? 0 : -height
        window: root
        windowTitle: root.title
        anchors.top:   parent.top
        anchors.left:  parent.left
        anchors.right: parent.right
        Behavior on y {
            NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
        }
    }

    // ── 배경 + 비디오 ─────────────────────────────────────────────────────
    Rectangle {
        anchors.fill: parent
        color: "#1e1e1e"
        radius: 8
        clip: true

        // 온라인 상태 테두리
        border.color: root.isOnline ? "#4caf50" : "#f44336"
        border.width: 2

        // 비디오 스트림
        Video {
            id: videoItem
            anchors.fill: parent
            rtspUrl: root.rtspUrl

            Component.onCompleted: {
                if (root.isOnline && root.rtspUrl !== "") {
                    videoItem.startStream()
                }
            }
        }

        // 오프라인 표시
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            visible: !root.isOnline

            Text {
                anchors.centerIn: parent
                text: "OFFLINE"
                color: "#f44336"
                font.pixelSize: 28
                font.bold: true
            }
        }

        // 좌하단 카메라 이름 레이블 (타이틀바 숨겨진 상태에서도 식별 가능)
        Rectangle {
            anchors.left:   parent.left
            anchors.bottom: parent.bottom
            anchors.margins: 8
            width: titleLabel.width + 16
            height: 28
            radius: 6
            color: "#88000000"
            visible: !hoverHandler.hovered

            Text {
                id: titleLabel
                anchors.centerIn: parent
                text: root.title
                color: "white"
                font.pixelSize: 13
                font.bold: true
            }
        }
    }
}
