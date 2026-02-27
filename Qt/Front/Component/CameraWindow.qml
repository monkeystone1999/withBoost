import QtQuick
import QtQuick.Controls
import QWindowKit
import AnoMap.front

Window {
    id: root
    flags: Qt.Window | Qt.FramelessWindowHint
    color: "transparent"
    visible: true
    StackView {
        id: stackView
        anchors.fill: parent
    }
    HoverHandler {
        id: hoverHandler
    }

    // 프레임리스 설정
    WindowAgent {
        id: agent
        Component.onCompleted: {
            agent.setup(root);
            // 타이틀바 영역 등록 (이 아이템 위에서 드래그하면 창이 움직여요)
            agent.setTitleBar(cameraTitlebar);
            agent.setSystemButton(WindowAgent.Close, cameraTitlebar.closeBtn);
            agent.setSystemButton(WindowAgent.Maximize, cameraTitlebar.maximizeBtn);
            agent.setSystemButton(WindowAgent.Minimize, cameraTitlebar.minimizeBtn);
        }
    }
    CameraWindowTitlebar {
        id: cameraTitlebar
        z: 100
        y: hoverHandler.hovered ? 0 : -height
        window: root
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        Behavior on y {
            NumberAnimation {
                duration: 200
                easing.type: Easing.OutQuad
            }
        }
    }
}
