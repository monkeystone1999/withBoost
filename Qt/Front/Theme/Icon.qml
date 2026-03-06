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
}
