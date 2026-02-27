pragma Singleton
import QtQuick

QtObject {
    property bool isDark: false
    property color hanwhaFirst: "#F37321"
    property color hanwhaSecond: "#F89B6C"
    property color hanwhaThird: "#FBB584"
    property color bgPrimary: isDark ? "#1C1C1E" : "#F2F2F7"
    property color bgSecondary: isDark ? "#252527" : "#FFFFFF"
    property color bgThird: isDark ? "#2C2C2E" : "#FFFFFF"
    property color bgAuto: isDark ? "#151516" : "#EFEFF4"
    property color fontColor: isDark ? "#FFFCFA" : "#000500"
    property color iconColor: isDark ? "#FFFCFA" : "#000500"
    property color iconChange: isDark ? "#F89B6C" : "#F37321"
}
