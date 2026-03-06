pragma Singleton
import QtQuick

QtObject {
    // ── Font Loaders ─────────────────────────────────────────────
    readonly property FontLoader _hanwhaB: FontLoader {
        source: Qt.resolvedUrl("../Assets/Core/Fonts/01HanwhaB.ttf")
    }
    readonly property FontLoader _hanwhaR: FontLoader {
        source: Qt.resolvedUrl("../Assets/Core/Fonts/02HanwhaR.ttf")
    }
    readonly property FontLoader _hanwhaL: FontLoader {
        source: Qt.resolvedUrl("../Assets/Core/Fonts/03HanwhaL.ttf")
    }
    readonly property FontLoader _gothicB: FontLoader {
        source: Qt.resolvedUrl("../Assets/Core/Fonts/04HanwhaGothicB.ttf")
    }
    readonly property FontLoader _gothicR: FontLoader {
        source: Qt.resolvedUrl("../Assets/Core/Fonts/05HanwhaGothicR.ttf")
    }
    readonly property FontLoader _gothicL: FontLoader {
        source: Qt.resolvedUrl("../Assets/Core/Fonts/06HanwhaGothicL.ttf")
    }
    readonly property FontLoader _gothicEL: FontLoader {
        source: Qt.resolvedUrl("../Assets/Core/Fonts/07HanwhaGothicEL.ttf")
    }
    readonly property FontLoader _gothicT: FontLoader {
        source: Qt.resolvedUrl("../Assets/Core/Fonts/08HanwhaGothicT.ttf")
    }

    // ── Expose Family Names (Maintaining Existing Variable Names) ──
    readonly property string fontFamilyPrimary: _hanwhaR.font.family
    readonly property string fontFamilyPrimaryBold: _hanwhaB.font.family

    readonly property string fontFamilySecondary: _gothicR.font.family
    readonly property string fontFamilySecondaryBold: _gothicB.font.family
    readonly property string fontFamilySecondaryExtraLight: _gothicEL.font.family
    readonly property string fontFamilySecondaryThin: _gothicT.font.family

    // ── Size Scales ─────────────────────────────────────────
    readonly property int brandFont: 32
    readonly property int primaryFont: 24
    readonly property int secondaryFont: 18
    readonly property int thirdFont: 16
    readonly property int teriaryFont: 14
}
