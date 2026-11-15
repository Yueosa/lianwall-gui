pragma Singleton
import QtQuick
QtObject {
    property string current: "lian"
    property string accentScheme: "iris"
    readonly property bool isDark: current === "lian-dark"
    readonly property color accent: accentScheme === "blue" ? (isDark ? "#a5d1e9" : "#46758e") : accentScheme === "pink" ? (isDark ? "#f0b3ca" : "#a45f78") : (isDark ? "#d3b4ea" : "#79558f")
    readonly property color accentHover: Qt.lighter(accent, 1.08)
    readonly property color accentPressed: Qt.darker(accent, 1.08)
    readonly property color accentContainer: accentScheme === "blue" ? (isDark ? "#2b414e" : "#e1eff7") : accentScheme === "pink" ? (isDark ? "#4e303e" : "#f8e4ec") : (isDark ? "#44334f" : "#ede2f5")
    readonly property color primary: accent
    readonly property color primaryHover: accentHover
    readonly property color primaryPressed: accentPressed
    readonly property color background: isDark ? "#19171e" : "#f8f6fb"
    readonly property color surface: isDark ? "#24212b" : "#ffffff"
    readonly property color card: surface
    readonly property color popup: surface
    readonly property color input: isDark ? "#302b37" : "#f4f0f8"
    readonly property color text: isDark ? "#eee7f2" : "#292430"
    readonly property color textSecondary: isDark ? "#b0a6ba" : "#81798a"
    readonly property color textDisabled: isDark ? "#716779" : "#afa6b7"
    readonly property color textOnAccent: isDark ? "#34203f" : "#ffffff"
    readonly property color textOnPrimary: textOnAccent
    readonly property color border: isDark ? "#3c3545" : "#e8e2ee"
    readonly property color borderFocus: accent
    readonly property color borderHover: accent
    readonly property color success: isDark ? "#a2cfb6" : "#487b65"
    readonly property color warning: isDark ? "#edc98a" : "#9a742c"
    readonly property color error: isDark ? "#f2a5b6" : "#ad4e60"
    readonly property color info: accent
    readonly property color navBar: surface
    readonly property color navBarSelected: accentContainer
    readonly property color scrollbar: isDark ? "#51485b" : "#c7bdce"
    readonly property color scrollbarHover: accent
    readonly property color divider: border
    readonly property color shadow: "#16000000"
    readonly property color cardHover: isDark ? "#302b37" : "#f0ecf5"
    readonly property color cardSelected: accentContainer
    readonly property color cardLocked: cardHover
    readonly property color cardCooling: input
    readonly property color mtfBlue: "#5BCEFA"
    readonly property color mtfPink: "#F5A9B8"
    readonly property color mtfWhite: "#FFFFFF"
    readonly property int radiusSmall: 6
    readonly property int radiusMedium: 8
    readonly property int radiusLarge: 12
    readonly property int spacingTiny: 4
    readonly property int spacingSmall: 8
    readonly property int spacingMedium: 16
    readonly property int spacingLarge: 24
    readonly property int fontSizeSmall: 11
    readonly property int fontSizeMedium: 13
    readonly property int fontSizeLarge: 15
    readonly property int fontSizeTitle: 19
    readonly property int fontSizeHeader: 22
    readonly property var gradientAccent: Gradient {
        GradientStop { position: 0; color: "#5BCEFA" }
        GradientStop { position: 1; color: "#F5A9B8" }
    }
}
