import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import Qt5Compat.GraphicalEffects
import AnoMap.front

// 좌측 슬라이드 사이드 네비게이션 패널
// open() / close() / toggle() 로 제어
Item {
    id: root

    property string currentPage: "Dashboard"

    signal requestPage(string pageName)

    property bool isOpen: false

    function open() {
        isOpen = true;
    }
    function close() {
        isOpen = false;
    }
    function toggle() {
        isOpen = !isOpen;
    }

    anchors.fill: parent
    enabled: isOpen

    // ── 딤 배경 ────────────────────────────────────────────────────────────
    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: root.isOpen ? 0.45 : 0.0
        visible: opacity > 0 // Opacity가 0보다 클 때만 렌더링되게 하여 클릭 흡수 방지
        Behavior on opacity {
            NumberAnimation {
                duration: 220
                easing.type: Easing.OutCubic
            }
        }
        MouseArea {
            anchors.fill: parent
            onClicked: root.close()
        }
    }

    // ── 패널 본체 ──────────────────────────────────────────────────────────
    Rectangle {
        id: panel
        width: 260
        height: parent.height
        x: root.isOpen ? 0 : -width
        color: Theme.bgSecondary

        Behavior on x {
            NumberAnimation {
                duration: 250
                easing.type: Easing.OutCubic
            }
        }

        // 오른쪽 경계선
        Rectangle {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 1
            color: Theme.isDark ? "#3A3A3C" : "#D1D1D6"
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.topMargin: 16
            anchors.bottomMargin: 16
            spacing: 0

            // 로고 + 앱 이름
            Row {
                Layout.leftMargin: 20
                Layout.bottomMargin: 8
                spacing: 10

                Image {
                    readonly property int logoH: 30
                    readonly property int logoW: logoH * (85 / 77)
                    anchors.verticalCenter: parent.verticalCenter
                    source: Icon.logo
                    height: logoH
                    width: logoW
                    sourceSize.width: logoW
                    sourceSize.height: logoH
                    fillMode: Image.PreserveAspectFit
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "AnoMAP"
                    font.family: Typography.fontFamilyPrimary
                    font.pixelSize: 22
                    color: Theme.fontColor
                }
            }

            // 구분선
            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 8
                Layout.bottomMargin: 8
                height: 1
                color: Theme.isDark ? "#3A3A3C" : "#D1D1D6"
            }

            // ── 메인 메뉴 항목 ────────────────────────────────────────────────
            Repeater {
                model: [
                    {
                        label: "Home",
                        icon: "▦",
                        page: "Home"
                    },
                    {
                        label: "AI",
                        icon: "🤖",
                        page: "AI"
                    },
                    {
                        label: "Device",
                        icon: "📷",
                        page: "Device"
                    },
                    {
                        label: "MyPage",
                        icon: "👤",
                        page: "MyPage"
                    },
                    {
                        label: "Admin Panel",
                        icon: "🛡",
                        page: "AdminDashboard"
                    }
                ]
                delegate: NavItem {
                    Layout.fillWidth: true
                    label: modelData.label
                    iconChar: modelData.icon
                    isActive: root.currentPage === modelData.page
                    visible: modelData.page !== "AdminDashboard" || (typeof loginController !== "undefined" && loginController.state === "admin")
                    onClicked: {
                        root.currentPage = modelData.page;
                        root.requestPage(modelData.page);
                        root.close();
                    }
                }
            }

            Item {
                Layout.fillHeight: true
            }

            // 구분선
            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 8
                Layout.bottomMargin: 8
                height: 1
                color: Theme.isDark ? "#3A3A3C" : "#D1D1D6"
            }

            // ── 하단: Logout & Option (Settings) ────────────────────────────────
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 56
                spacing: 0

                // Logout Button
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 56
                    color: logoutHover.containsMouse ? (Theme.isDark ? "#3A3A3C" : "#E5E5EA") : "transparent"

                    Row {
                        anchors.centerIn: parent
                        spacing: 8
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "🚪"
                            font.pixelSize: 18
                            color: Theme.fontColor
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "Logout"
                            font.family: Typography.fontFamilySecondary
                            font.pixelSize: Typography.thirdFont
                            color: Theme.fontColor
                        }
                    }

                    MouseArea {
                        id: logoutHover
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            // 콴 노 열려있는 모든 CameraWindow 닫기
                            if (typeof root.Window !== "undefined") {
                                var mainWin = root.Window.window;
                                if (mainWin && typeof mainWin.closeAllCameraWindows === "function")
                                    mainWin.closeAllCameraWindows();
                            }
                            if (typeof loginController !== "undefined" && loginController !== null) {
                                loginController.logout();
                            }
                            root.currentPage = "Login";
                            root.requestPage("Login");
                            root.close();
                        }
                    }
                }

                // 중앙 구분선
                Rectangle {
                    Layout.preferredHeight: 24
                    Layout.alignment: Qt.AlignVCenter
                    width: 1
                    color: Theme.isDark ? "#3A3A3C" : "#D1D1D6"
                }

                // Option Button
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 56
                    color: optHover.containsMouse ? (Theme.isDark ? "#3A3A3C" : "#E5E5EA") : "transparent"

                    Row {
                        anchors.centerIn: parent
                        spacing: 8

                        Item {
                            anchors.verticalCenter: parent.verticalCenter
                            width: 20
                            height: 20
                            Image {
                                id: optIcon
                                anchors.fill: parent
                                source: Icon.option
                                sourceSize.width: 20
                                sourceSize.height: 20
                                visible: false
                            }
                            ColorOverlay {
                                anchors.fill: optIcon
                                source: optIcon
                                color: Theme.fontColor
                            }
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "Option"
                            font.family: Typography.fontFamilySecondary
                            font.pixelSize: Typography.thirdFont
                            color: Theme.fontColor
                        }
                    }

                    MouseArea {
                        id: optHover
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            // Option은 별도 Page가 아닌 Floating Dialog
                            if (typeof root.Window !== "undefined") {
                                var mainWin = root.Window.window;
                                if (mainWin && typeof mainWin.openOptionDialog === "function")
                                    mainWin.openOptionDialog();
                            }
                            root.close();
                        }
                    }
                }
            }
        }
    }
}
