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
    flags: Qt.Window | Qt.FramelessWindowHint

    onClosing: Qt.quit()

    // ── AppController: 네비게이션 + CameraWindow 레지스트리 ───────────────────
    Connections {
        target: typeof appController !== "undefined" ? appController : null

        function onNavigateTo(page) {
            switch (page) {
            case "Login":
                stackView.replace(null, "View/LoginPage.qml"); break
            case "Dashboard":
            case "Home":
                if (stackView.depth > 1) stackView.pop(null)
                else stackView.replace(null, "View/DashboardPage.qml"); break
            case "AdminDashboard":
                stackView.replace(null, "View/AdminPage.qml"); break
            case "AI":
                stackView.replace(null, "View/AIPage.qml"); break
            case "Device":
                stackView.replace(null, "View/DevicePage.qml"); break
            case "MyPage":
                stackView.replace(null, "View/MyPage.qml"); break
            case "Signup":
                stackView.replace(null, "View/SignupPage.qml"); break
            case "Back":
                if (stackView.depth > 1) stackView.pop(); break
            }
        }
        function onOptionDialogRequested() {
            optionDialog.open()
        }
        function onLogoutRequested() {
            if (typeof loginController !== "undefined" && loginController !== null)
                loginController.logout()
        }
    }

    WindowAgent {
        id: agent
        Component.onCompleted: {
            agent.setup(root)
            agent.setTitleBar(titleBar)
            agent.setSystemButton(WindowAgent.Close, titleBar.closeBtn)
            agent.setSystemButton(WindowAgent.Maximize, titleBar.maximizeBtn)
            agent.setSystemButton(WindowAgent.Minimize, titleBar.minimizeBtn)
            agent.setHitTestVisible(titleBar.menuBtn)
            agent.setHitTestVisible(titleBar.alarmBtn)
        }
    }

    Titlebar {
        id: titleBar
        window: root
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
    }

    StackView {
        id: stackView
        anchors.top: titleBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        initialItem: "View/LoginPage.qml"

        pushEnter: Transition {
            ParallelAnimation {
                NumberAnimation { property: "x"; from: stackView.width * 0.06; to: 0; duration: 320; easing.type: Easing.OutCubic }
                NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 280; easing.type: Easing.OutCubic }
            }
        }
        pushExit: Transition {
            NumberAnimation { property: "x"; from: 0; to: -stackView.width * 0.04; duration: 320; easing.type: Easing.OutCubic }
        }
        popEnter: Transition {
            ParallelAnimation {
                NumberAnimation { property: "x"; from: -stackView.width * 0.06; to: 0; duration: 320; easing.type: Easing.OutCubic }
                NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 280; easing.type: Easing.OutCubic }
            }
        }
        popExit: Transition {
            NumberAnimation { property: "x"; from: 0; to: stackView.width * 0.04; duration: 320; easing.type: Easing.OutCubic }
        }
    }

    SideNavPanel {
        id: sideNav
        z: 50
        currentPage: typeof appController !== "undefined" ? appController.currentPage : "Dashboard"
        onRequestPage: pageName => {
            if (typeof appController !== "undefined")
                appController.navigate(pageName)
        }
    }

    // ── 현재 페이지의 requestPage 시그널 처리 ──────────────────────────────────
    Connections {
        target: stackView.currentItem
        function onRequestPage(pageName) {
            if (typeof appController !== "undefined")
                appController.navigate(pageName)
        }
        function onRequestClose() {
            if (stackView.depth > 1) stackView.pop()
        }
    }

    Connections {
        target: titleBar.menuBtn
        function onClicked() { sideNav.toggle() }
    }

    // ── 알람 레이어: AlarmController.alarms 리스트를 ListView로 표시 ──────────
    Item {
        id: alarmLayer
        z: 200
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 20
        anchors.bottomMargin: 20
        width: 300
        height: parent.height
        clip: false

        ListView {
            id: alarmListView
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            width: parent.width
            height: contentHeight
            spacing: 8
            verticalLayoutDirection: ListView.BottomToTop
            model: typeof alarmController !== "undefined" ? alarmController.alarms : []

            delegate: AlarmCard {
                alarmId: modelData.id
                alarmTitle: modelData.title
                alarmDetail: modelData.detail
                severity: modelData.severity
                onDismissRequested: id => {
                    if (typeof alarmController !== "undefined")
                        alarmController.dismiss(id)
                }
            }
        }
    }

    // ── Option Dialog ─────────────────────────────────────────────────────────
    Popup {
        id: optionDialog
        anchors.centerIn: parent
        width: 440
        height: optDialogCol.implicitHeight + 56
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        padding: 0

        background: Rectangle {
            color: Theme.bgSecondary
            radius: 14
            border.color: Theme.isDark ? "#3A3A3C" : "#D1D1D6"
            border.width: 1
        }

        Column {
            id: optDialogCol
            anchors { top: parent.top; left: parent.left; right: parent.right; margins: 0 }
            spacing: 0

            Rectangle {
                width: parent.width
                height: 52
                color: "transparent"
                radius: 14
                Text { anchors.centerIn: parent; text: "⚙️  Option"; font.pixelSize: 17; font.bold: true; color: Theme.fontColor }
                Rectangle {
                    anchors { right: parent.right; verticalCenter: parent.verticalCenter; rightMargin: 14 }
                    width: 28; height: 28; radius: 14
                    color: closeOptHov.containsMouse ? (Theme.isDark ? "#555" : "#ddd") : "transparent"
                    Text { anchors.centerIn: parent; text: "×"; font.pixelSize: 20; color: Theme.fontColor }
                    MouseArea { id: closeOptHov; anchors.fill: parent; hoverEnabled: true; onClicked: optionDialog.close() }
                }
                Rectangle { anchors { bottom: parent.bottom; left: parent.left; right: parent.right; leftMargin: 16; rightMargin: 16 }height: 1; color: Theme.isDark ? "#3A3A3C" : "#D1D1D6" }
            }

            Column {
                width: parent.width
                spacing: 0
                padding: 0

                OptRow {
                    label: "🌙  Dark Mode"
                    content: Switch {
                        checked: typeof settingsController !== "undefined" ? settingsController.darkMode : Theme.isDark
                        onToggled: {
                            if (typeof settingsController !== "undefined") {
                                settingsController.darkMode = checked
                                settingsController.save()
                            }
                            Theme.isDark = checked
                        }
                    }
                }
                OptRow {
                    label: "🔔  Alarm Sound"
                    content: Switch {
                        checked: typeof settingsController !== "undefined" ? settingsController.alarmSound : true
                        onToggled: {
                            if (typeof settingsController !== "undefined") {
                                settingsController.alarmSound = checked
                                settingsController.save()
                            }
                        }
                    }
                }
                OptRow {
                    label: "🗖  Default Grid"
                    content: Row {
                        spacing: 6
                        Repeater {
                            model: [2, 3, 4]
                            delegate: Rectangle {
                                width: 36; height: 28; radius: 6
                                color: defGridBtn.containsMouse ? Theme.hanwhaFirst : (Theme.isDark ? "#3A3A3C" : "#E5E5EA")
                                Text { anchors.centerIn: parent; text: modelData; font.pixelSize: 13; color: Theme.fontColor }
                                MouseArea {
                                    id: defGridBtn
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: {
                                        if (typeof settingsController !== "undefined") {
                                            settingsController.defaultGrid = modelData
                                            settingsController.save()
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                OptRow {
                    label: "🌐  Server"
                    content: Text {
                        text: (typeof networkBridge !== "undefined" && networkBridge.serverAddress) ? networkBridge.serverAddress : "--"
                        font.pixelSize: 13
                        color: Theme.isDark ? "#888" : "#666"
                    }
                }
                OptRow {
                    label: "📜  Log Level"
                    content: Row {
                        spacing: 6
                        Repeater {
                            model: ["INFO", "WARN", "DEBUG"]
                            delegate: Rectangle {
                                width: 52; height: 26; radius: 6
                                color: logBtn.containsMouse ? Theme.hanwhaFirst : (Theme.isDark ? "#3A3A3C" : "#E5E5EA")
                                Text { anchors.centerIn: parent; text: modelData; font.pixelSize: 11; color: Theme.fontColor }
                                MouseArea {
                                    id: logBtn
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: {
                                        if (typeof settingsController !== "undefined") {
                                            settingsController.logLevel = modelData
                                            settingsController.save()
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                OptRow {
                    label: "ℹ️  Version"
                    content: Text { text: "1.0.0"; font.pixelSize: 13; color: Theme.isDark ? "#888" : "#666" }
                }
            }
        }
    }

    component OptRow: Item {
        property string label: ""
        property alias content: contentSlot.data
        width: optDialogCol.width
        height: 52
        Text { anchors { left: parent.left; leftMargin: 24; verticalCenter: parent.verticalCenter } text: parent.label; font.pixelSize: 14; color: Theme.fontColor }
        Item { id: contentSlot; anchors { right: parent.right; rightMargin: 24; verticalCenter: parent.verticalCenter } width: childrenRect.width; height: childrenRect.height }
        Rectangle { anchors { bottom: parent.bottom; left: parent.left; right: parent.right; leftMargin: 16; rightMargin: 16 } height: 1; color: Theme.isDark ? "#2A2A2C" : "#E5E5EA" }
    }
}
