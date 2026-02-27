pragma Singleton
import QtQuick

QtObject {
    // 로그 기반 정확한 패밀리명으로 수정
    property string fontFamilyPrimary: "Hanwha"          // Regular, Light 포함
    property string fontFamilyPrimaryBold: "Hanwha B"    // Bold 별도 패밀리

    property string fontFamilySecondary: "hanwhaGothic"  // Regular, Light 포함
    property string fontFamilySecondaryBold: "hanwhaGothic B"
    property string fontFamilySecondaryExtraLight: "hanwhaGothic EL"
    property string fontFamilySecondaryThin: "hanwhaGothic T"

    readonly property int brandFont: 32
    readonly property int primaryFont: 24
    readonly property int secondaryFont: 18
    readonly property int thirdFont: 16
    readonly property int teriaryFont: 14
}
