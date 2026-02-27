import QtQuick
import QtQuick.Controls
import QWindowKit
import AnoMap.front

Window {
    id: root
    width: 1280
    height: 800
    visible: true
    color: "transparent"
    // 프레임리스 설정
    flags: Qt.Window | Qt.FramelessWindowHint
    // QWindowKit 에이전트 - 이게 핵심이에요
    // 드래그/리사이즈/스냅 등 네이티브 동작을 담당해요
    Loader {}

    WindowAgent {
        id: agent
        Component.onCompleted: {
            agent.setup(root);
            // 타이틀바 영역 등록
            agent.setTitleBar(titleBar);
            // 시스템 버튼 등록: setup() 이후에 호율해야 함 (null 크래시 방지)
            agent.setSystemButton(WindowAgent.Close, titleBar.closeBtn);
            agent.setSystemButton(WindowAgent.Maximize, titleBar.maximizeBtn);
            agent.setSystemButton(WindowAgent.Minimize, titleBar.minimizeBtn);
            // Option 버튼은 드래그 영역 제외
            agent.setHitTestVisible(titleBar.optionBtn);
            agent.setHitTestVisible(titleBar.alarmBtn);
        }
    }
    Titlebar {
        id: titleBar
        window: root
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
    }

    // 콘텐츠 영역 (StackView for Page Navigation)
    StackView {
        id: stackView
        anchors.top: titleBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        initialItem: "views/LoginPage.qml"

        // Push: 새 페이지가 오른쪽에서 슬라이드인
        pushEnter: Transition {
            ParallelAnimation {
                NumberAnimation {
                    property: "x"
                    from: stackView.width * 0.06
                    to: 0
                    duration: 320
                    easing.type: Easing.OutCubic
                }
                NumberAnimation {
                    property: "opacity"
                    from: 0
                    to: 1
                    duration: 280
                    easing.type: Easing.OutCubic
                }
            }
        }
        pushExit: Transition {
            NumberAnimation {
                property: "x"
                from: 0
                to: -stackView.width * 0.04
                duration: 320
                easing.type: Easing.OutCubic
            }
        }

        // Pop: 이전 페이지가 왼쪽에서 슬라이드인
        popEnter: Transition {
            ParallelAnimation {
                NumberAnimation {
                    property: "x"
                    from: -stackView.width * 0.06
                    to: 0
                    duration: 320
                    easing.type: Easing.OutCubic
                }
                NumberAnimation {
                    property: "opacity"
                    from: 0
                    to: 1
                    duration: 280
                    easing.type: Easing.OutCubic
                }
            }
        }
        popExit: Transition {
            NumberAnimation {
                property: "x"
                from: 0
                to: stackView.width * 0.04
                duration: 320
                easing.type: Easing.OutCubic
            }
        }
    }

    // 설정 오버레이 레이어 추가
    Loader {
        id: settingsOverlay
        anchors.fill: parent
        z: 100
        source: active ? "views/SettingsPage.qml" : ""
        active: false
    }

    Connections {
        target: settingsOverlay.item
        function onRequestClose() {
            settingsOverlay.active = false;
        }
    }

    // Global Navigation Handler
    Connections {
        target: stackView.currentItem
        function onRequestPage(pageName) {
            console.log("Navigation requested:", pageName);
            switch (pageName) {
            case "Login":
                stackView.push("views/LoginPage.qml");
                break;
            case "Dashboard":
                stackView.push("views/DashboardPage.qml");
                break;
            case "Signup":
                stackView.push("views/SignupPage.qml");
                break;
            case "Settings":
                settingsOverlay.active = true; // push 대신 overlay 활성화
                break;
            case "Back":
                stackView.pop();
                break;
            default:
                console.log("Unknown page:", pageName);
            }
        }
    }

    Connections {
        target: titleBar.optionBtn
        function onClicked() {
            settingsOverlay.active = true;
        }
    }
}
