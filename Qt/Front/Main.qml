import QtQuick
import QtQuick.Controls
import QWindowKit
import AnoMap.Front

Window {
    id: rootItem
    width: 1280
    height: 800
    visible: true
    color: "transparent"
    flags: Qt.Window | Qt.FramelessWindowHint

    onClosing: close => {
        if (typeof appController !== "undefined")
            appController.shutdown();
        Qt.quit();
    }

    Component.onCompleted: {
        if (typeof settingsController !== "undefined")
            Theme.isDark = settingsController.darkMode;
    }

    Connections {
        target: typeof appController !== "undefined" ? appController : null // app controller bridge

        function onNavigateTo(page) {
            switch (page) {
            case "Login":
                stackView.replace(null, "pages/LoginPage.qml");
                break;
            case "Signup":
                stackView.replace(null, "pages/SignupPage.qml");
                break;
            case "Back":
                if (stackView.depth > 1)
                    stackView.pop();
                else{
                    stackView.replace(null, "pages/LoginPage.qml");
                }
                break;
            case "Dashboard":
                if (stackView.depth > 1)
                    stackView.pop(null);
                else
                    stackView.replace(null, "pages/DashboardPage.qml");
                break;
            case "AI":
                stackView.replace(null, "pages/AiPage.qml");
                break;
            case "Device":
                stackView.replace(null, "pages/DevicePage.qml");
                break;
            case "MyPage":
                stackView.replace(null, "pages/MyPage.qml");
                break;
            case "AdminDashboard":
                stackView.replace(null, "pages/AdminPage.qml");
                break;
            }
        }
        function onOptionDialogRequested() {
            optionDialog.open();
        }
        function onLogoutRequested() {
            if (typeof loginController !== "undefined" && loginController !== null)
                loginController.logout();
        }
    }

    WindowAgent { // window chrome integration
        id: agent
        Component.onCompleted: {
            agent.setup(rootItem);
            agent.setTitleBar(titleBar);
            agent.setSystemButton(WindowAgent.Close, titleBar.closeButtonItem);
            agent.setSystemButton(WindowAgent.Maximize, titleBar.maximizeButtonItem);
            agent.setSystemButton(WindowAgent.Minimize, titleBar.minimizeButtonItem);
            agent.setHitTestVisible(titleBar.menuButtonItem);
            agent.setHitTestVisible(titleBar.alarmButtonItem);
        }
    }

    TitleBar {
        id: titleBar
        window: rootItem
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
        initialItem: "pages/LoginPage.qml"

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

    SideNavPanel {
        id: sideNav
        z: 50
        currentPage: typeof appController !== "undefined" ? appController.currentPage : "Dashboard"
        onRequestPage: pageName => {
            if (typeof appController !== "undefined")
                appController.navigate(pageName);
        }
    }

    Connections {
        target: stackView.currentItem // page request bridge
        function onRequestPage(pageName) {
            if (typeof appController !== "undefined")
                appController.navigate(pageName);
        }
        function onRequestClose() {
            if (stackView.depth > 1)
                stackView.pop();
        }
    }

    Connections {
        target: titleBar.menuButtonItem // nav toggle hook
        function onClicked() {
            sideNav.toggle();
        }
    }

    Item {
        id: alarmLayer // alarm stack
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
                        alarmController.dismiss(id);
                }
            }
        }
    }

    Popup { // options dialog
        id: optionDialog
        anchors.centerIn: parent
        width: 440
        height: optionDialogColumn.implicitHeight + 56
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
            id: optionDialogColumn
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
                margins: 0
            }
            spacing: 0

            Rectangle {
                width: parent.width
                height: 52
                color: "transparent"
                radius: 14
                Text {
                    anchors.centerIn: parent
                    text: "Settings"
                    font.pixelSize: 17
                    font.bold: true
                    color: Theme.fontColor
                }
                Rectangle {
                    anchors {
                        right: parent.right
                        verticalCenter: parent.verticalCenter
                        rightMargin: 14
                    }
                    width: 28
                    height: 28
                    radius: 14
                    color: closeOptionHoverArea.containsMouse ? (Theme.isDark ? "#555" : "#ddd") : "transparent"
                    Text {
                        anchors.centerIn: parent
                        text: "?"
                        font.pixelSize: 20
                        color: Theme.fontColor
                    }
                    MouseArea {
                        id: closeOptionHoverArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: optionDialog.close()
                    }
                }
                Rectangle {
                    anchors {
                        bottom: parent.bottom
                        left: parent.left
                        right: parent.right
                        leftMargin: 16
                        rightMargin: 16
                    }
                    height: 1
                    color: Theme.isDark ? "#3A3A3C" : "#D1D1D6"
                }
            }

            Column {
                width: parent.width
                spacing: 0
                padding: 0

                OptRow {
                    label: "Dark Mode"
                    content: Switch {
                        checked: typeof settingsController !== "undefined" ? settingsController.darkMode : Theme.isDark
                        onToggled: {
                            if (typeof settingsController !== "undefined") {
                                settingsController.darkMode = checked;
                                settingsController.save();
                            }
                            Theme.isDark = checked;
                        }
                    }
                }
                OptRow {
                    label: "Alarm Sound"
                    content: Switch {
                        checked: typeof settingsController !== "undefined" ? settingsController.alarmSound : true
                        onToggled: {
                            if (typeof settingsController !== "undefined") {
                                settingsController.alarmSound = checked;
                                settingsController.save();
                            }
                        }
                    }
                }
                OptRow {
                    label: "Server Address"
                    content: Text {
                        text: (typeof networkBridge !== "undefined" && networkBridge.serverAddress) ? networkBridge.serverAddress : "--"
                        font.pixelSize: 13
                        color: Theme.isDark ? "#888" : "#666"
                    }
                }
                OptRow {
                    label: "Log Level"
                    content: Row {
                        spacing: 6
                        Repeater {
                            model: ["INFO", "WARN", "DEBUG"]
                            delegate: Rectangle {
                                width: 52
                                height: 26
                                radius: 6
                                color: logLevelButton.containsMouse ? Theme.hanwhaFirst : (Theme.isDark ? "#3A3A3C" : "#E5E5EA")
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData
                                    font.pixelSize: 11
                                    color: Theme.fontColor
                                }
                                MouseArea {
                                    id: logLevelButton
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: {
                                        if (typeof settingsController !== "undefined") {
                                            settingsController.logLevel = modelData;
                                            settingsController.save();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                OptRow {
                    label: "Version"
                    content: Text {
                        text: "1.0.0"
                        font.pixelSize: 13
                        color: Theme.isDark ? "#888" : "#666"
                    }
                }
            }
        }
    }

    component OptRow: Item { // options row
        property string label: ""
        property alias content: contentSlot.data
        width: optionDialogColumn.width
        height: 52
        Text {
            anchors {
                left: parent.left
                leftMargin: 24
                verticalCenter: parent.verticalCenter
            }
            text: parent.label
            font.pixelSize: 14
            color: Theme.fontColor
        }
        Item {
            id: contentSlot
            anchors {
                right: parent.right
                rightMargin: 24
                verticalCenter: parent.verticalCenter
            }
            width: childrenRect.width
            height: childrenRect.height
        }
        Rectangle {
            anchors {
                bottom: parent.bottom
                left: parent.left
                right: parent.right
                leftMargin: 16
                rightMargin: 16
            }
            height: 1
            color: Theme.isDark ? "#2A2A2C" : "#E5E5EA"
        }
    }
}
