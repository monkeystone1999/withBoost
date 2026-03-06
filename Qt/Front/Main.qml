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

    // 앱 종료 시 모든 자식 창(새창)과 함께 완전히 프로세스가 종료되도록 함
    onClosing: Qt.quit()

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
            agent.setHitTestVisible(titleBar.menuBtn);
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
        initialItem: "View/LoginPage.qml"

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

    // 사이드 네비게이션 패널
    SideNavPanel {
        id: sideNav
        z: 50
        currentPage: "Dashboard"

        onRequestPage: pageName => {
            switch (pageName) {
            case "Home":
            case "Dashboard":
            case "AdminDashboard":
                if (stackView.depth > 1) {
                    stackView.pop(null);
                } else {
                    stackView.replace(null, "View/DashboardPage.qml");
                }
                break;
            case "AI":
                stackView.replace(null, "View/AIPage.qml");
                break;
            case "Device":
                stackView.replace(null, "View/DevicePage.qml");
                break;
            case "MyPage":
                stackView.replace(null, "View/MyPage.qml");
                break;
            case "Settings":
                stackView.replace(null, "View/SettingsPage.qml");
                break;
            case "Login":
                stackView.replace(null, "View/LoginPage.qml");
                break;
            case "Signup":
                stackView.replace(null, "View/SignupPage.qml");
                break;
            }
        }
    }

    // ── 전역 알람 레이어 ──────────────────────────────────────────────────────
    Item {
        id: alarmLayer
        z: 200
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 20
        anchors.bottomMargin: 20
        width: 300
        // 높이는 자식 카드들의 총합에 맞게 자동
        height: parent.height
        clip: false

        // 알람 큐 (화면 초과 시 대기)
        property var pendingQueue: []
        // 현재 표시 중인 카드들의 총 높이 추적
        property int totalDisplayedHeight: 0
        readonly property int cardSpacing: 8
        readonly property int maxDisplayHeight: root.height - titleBar.height - 40

        // 현재 표시 카드 컨테이너 (아래서 위로 쌓임)
        Column {
            id: alarmColumn
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            spacing: alarmLayer.cardSpacing
        }

        // AlarmCard 동적 생성용 컴포넌트
        Component {
            id: alarmCardComponent
            AlarmCard {
                onDismissRequested: id => alarmLayer.removeAlarm(id)
            }
        }

        // 알람 추가 함수
        function addAlarm(title, detail, severity) {
            var id = Date.now();
            var estimatedHeight = 60 + cardSpacing;

            if (totalDisplayedHeight + estimatedHeight > maxDisplayHeight) {
                // 화면 초과 — 큐에 저장
                pendingQueue.push({
                    id: id,
                    title: title,
                    detail: detail,
                    severity: severity
                });
                return;
            }

            var card = alarmCardComponent.createObject(alarmColumn, {
                alarmId: id,
                alarmTitle: title,
                alarmDetail: detail,
                severity: severity
            });

            if (card) {
                totalDisplayedHeight += estimatedHeight;
            }
        }

        // 알람 제거 함수
        function removeAlarm(id) {
            // alarmColumn 자식 탐색 후 제거
            for (var i = 0; i < alarmColumn.children.length; i++) {
                var child = alarmColumn.children[i];
                if (child.alarmId === id) {
                    totalDisplayedHeight -= (child.height + cardSpacing);
                    if (totalDisplayedHeight < 0)
                        totalDisplayedHeight = 0;
                    child.destroy();
                    break;
                }
            }
            // 큐에서 다음 알람 꺼내기
            if (pendingQueue.length > 0) {
                var next = pendingQueue.shift();
                pendingQueue = pendingQueue;  // notify
                addAlarm(next.title, next.detail, next.severity);
            }
        }
    }

    // 백엔드 알람 시그널 연결 (alarmManager 는 App.cpp 에서 context property 로 노출)
    Connections {
        target: typeof alarmManager !== "undefined" ? alarmManager : null
        function onAlarmTriggered(title, detail, severity) {
            alarmLayer.addAlarm(title, detail, severity);
        }
    }

    // Global Navigation Handler (StackView pages)
    Connections {
        target: stackView.currentItem
        function onRequestPage(pageName) {
            console.log("Navigation requested:", pageName);
            switch (pageName) {
            case "Login":
                // Logout 시나리오나 처음 Session 시작 시나리오 둘 다 커버 가능
                // Navigation은 LoginPage를 루트 페이지로 교체합니다.
                stackView.replace(null, "View/LoginPage.qml");
                break;
            case "Dashboard":
            case "Home":
            case "AdminDashboard":
                if (stackView.depth > 1) {
                    stackView.pop(null);
                } else {
                    stackView.replace(null, "View/DashboardPage.qml");
                }
                break;
            case "AI":
                stackView.replace(null, "View/AIPage.qml");
                break;
            case "Device":
                stackView.replace(null, "View/DevicePage.qml");
                break;
            case "MyPage":
                stackView.replace(null, "View/MyPage.qml");
                break;
            case "Signup":
                stackView.replace(null, "View/SignupPage.qml");
                break;
            case "Settings":
                stackView.replace(null, "View/SettingsPage.qml");
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
        target: titleBar.menuBtn
        function onClicked() {
            sideNav.toggle();
        }
    }
}
