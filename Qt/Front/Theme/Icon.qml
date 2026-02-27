pragma Singleton
import QtQuick

QtObject{
    // Explicit full paths to SVG resources
    readonly property string alarm    : "image://svg/Core/Icons/Alarm.svg"
    readonly property string close    : "image://svg/Core/Icons/Exit.svg"
    readonly property string help     : "image://svg/Core/Icons/Help.svg"
    readonly property string maximize : "image://svg/Core/Icons/Maximize.svg"
    readonly property string minimize : "image://svg/Core/Icons/Minimize.svg"
    readonly property string option   : "image://svg/Core/Icons/Option.svg"
    readonly property string logo     : "image://svg/Core/Logos/OnlyLogo.svg"
}
