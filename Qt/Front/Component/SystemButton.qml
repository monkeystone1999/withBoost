import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import AnoMap.front

// QWindowKit Snap Layout 호환을 위해 MouseArea 대신 Button 사용
// (QWindowKit 공식 예제 QWKButton 패턴과 동일)
AbstractButton {
    id: root
    width: 46
    height: 48
    hoverEnabled: true

    property int duration: 400
    property url iconSource
    property color bgColor: Theme.bgPrimary   // 'background' 는 Button 내장 프로퍼티라 이름 변경
    property color hoverIconChange: Theme.iconChange
    property bool isClose: false

    // Button 기본 패딩/인셋 제거
    leftPadding: 0
    topPadding: 0
    rightPadding: 0
    bottomPadding: 0
    leftInset: 0
    topInset: 0
    rightInset: 0
    bottomInset: 0

    Component.onCompleted: {
        console.log("SystemButton created, source:", iconSource);
    }

    background: Rectangle {
        color: {
            if (!root.enabled)
                return root.bgColor;
            if (root.isClose && root.hovered)
                return "#E81123";
            if (root.hovered)
                return Theme.bgThird;
            return root.bgColor;
        }
        Behavior on color {
            ColorAnimation {
                duration: root.duration
            }
        }
    }

    contentItem: Item {
        Image {
            id: iconImage
            anchors.centerIn: parent
            visible: true
            layer.enabled: true
            source: root.iconSource
            sourceSize.width: 33
            sourceSize.height: 33
            fillMode: Image.PreserveAspectFit
        }

        MultiEffect {
            anchors.fill: iconImage
            source: iconImage
            colorization: 1.0
            colorizationColor: root.hovered ? root.hoverIconChange : Theme.iconColor
            Behavior on colorizationColor {
                ColorAnimation {
                    duration: root.duration
                }
            }
        }
    }
}
