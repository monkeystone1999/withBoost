import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.front

// 좌측 슬라이드 사이드 네비게이션 패널
// open() / close() / toggle() 로 제어
Item {
    id: root

    property string currentPage: "Dashboard"

    signal requestPage(string pageName)

    property bool isOpen: false

    function open()   { isOpen = true  }
    function close()  { isOpen = false }
    function toggle() { isOpen = !isOpen }

    anchors.fill: parent
    enabled: isOpen

    // ── 딤 배경 ────────────────────────────────────────────────────────────
    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: root.isOpen ? 0.45 : 0.0
        Behavior on opacity {
            NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
        }
        MouseArea {
            anchors.fill: parent
            enabled: root.isOpen
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
            NumberAnimation { duration: 250; easing.type: Easing.OutCubic }
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
                    anchors.verticalCenter: parent.verticalCenter
                    source: Icon.logo
                    height: 30
                    width: height * (85 / 77)
                    sourceSize.width: width
                    sourceSize.height: height
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
                    { label: "Dashboard", icon: "▦", page: "Dashboard" },
                    { label: "Alarms",    icon: "🔔", page: "Alarms"   }
                ]
                delegate: NavItem {
                    Layout.fillWidth: true
                    label: modelData.label
                    iconChar: modelData.icon
                    isActive: root.currentPage === modelData.page
                    onClicked: {
                        root.currentPage = modelData.page
                        root.requestPage(modelData.page)
                        root.close()
                    }
                }
            }

            Item { Layout.fillHeight: true }

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

            // ── 하단: Settings ────────────────────────────────────────────────
            NavItem {
                Layout.fillWidth: true
                label: "Settings"
                iconChar: "⚙"
                isActive: root.currentPage === "Settings"
                onClicked: {
                    root.currentPage = "Settings"
                    root.requestPage("Settings")
                    root.close()
                }
            }
        }
    }
}
