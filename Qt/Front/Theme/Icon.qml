pragma Singleton
import QtQuick

QtObject {
    // Explicit full paths to SVG resources using relative paths
    readonly property url alarm: Qt.resolvedUrl("../Assets/Core/Icons/Alarm.svg")
    readonly property url close: Qt.resolvedUrl("../Assets/Core/Icons/Exit.svg")
    readonly property url help: Qt.resolvedUrl("../Assets/Core/Icons/Help.svg")
    readonly property url maximize: Qt.resolvedUrl("../Assets/Core/Icons/Maximize.svg")
    readonly property url minimize: Qt.resolvedUrl("../Assets/Core/Icons/Minimize.svg")
    readonly property url option: Qt.resolvedUrl("../Assets/Core/Icons/Option.svg")
    readonly property url logo: Qt.resolvedUrl("../Assets/Core/Logos/OnlyLogo.svg")

    // Navigation Icons
    readonly property url homeIcon: Qt.resolvedUrl("../Assets/Core/Icons/Home.svg")
    readonly property url aiIcon: Qt.resolvedUrl("../Assets/Core/Icons/Ai.svg")
    readonly property url deviceIcon: Qt.resolvedUrl("../Assets/Core/Icons/Device.svg")
    readonly property url profileIcon: Qt.resolvedUrl("../Assets/Core/Icons/Profile.svg")
    readonly property url adminIcon: Qt.resolvedUrl("../Assets/Core/Icons/Admin.svg")
    readonly property url logoutIcon: Qt.resolvedUrl("../Assets/Core/Icons/Logout.svg")
}
