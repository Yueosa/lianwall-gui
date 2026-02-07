pragma Singleton
import QtQuick

/**
 * LianWall 主题系统
 * 
 * 使用 MTF 旗帜色作为主题色：
 * - Blue:  #5BCEFA (理性 / 交互)
 * - Pink:  #F5A9B8 (强调 / 情绪)
 * - White: #FFFFFF (高亮 / 反差)
 */
QtObject {
    id: theme
    
    // ========================================================================
    // 当前主题 - 绑定到 ConfigManager
    // ========================================================================
    property string current: "lian"
    property bool isDark: current === "lian-dark"
    
    // 主色调方案 - "blue" 或 "pink"
    property string accentScheme: "pink"
    
    // ========================================================================
    // 主题色 (不随主题变化)
    // ========================================================================
    readonly property color mtfBlue: "#5BCEFA"
    readonly property color mtfPink: "#F5A9B8"
    readonly property color mtfWhite: "#FFFFFF"
    
    // ========================================================================
    // 语义化颜色
    // ========================================================================
    
    // 强调色 - 根据 accentScheme 动态切换
    readonly property color accent: accentScheme === "pink" ? mtfPink : mtfBlue
    readonly property color accentHover: Qt.lighter(accent, 1.1)
    readonly property color accentPressed: Qt.darker(accent, 1.1)
    
    // 主色 - 用于重要元素、选中状态
    readonly property color primary: mtfPink
    readonly property color primaryHover: Qt.lighter(mtfPink, 1.1)
    readonly property color primaryPressed: Qt.darker(mtfPink, 1.1)
    
    // ========================================================================
    // 背景色
    // ========================================================================
    
    // 窗口背景 (Dracula: #282a36)
    readonly property color background: isDark ? "#1e1e2e" : "#FAFAFA"
    
    // 表面/卡片背景
    readonly property color surface: isDark ? "#282a36" : "#FFFFFF"
    
    // 卡片/面板背景
    readonly property color card: isDark ? "#313244" : "#FFFFFF"
    
    // 悬浮/弹出层背景
    readonly property color popup: isDark ? "#383a4a" : "#FFFFFF"
    
    // 输入框背景
    readonly property color input: isDark ? "#1e1e2e" : "#FFFFFF"
    
    // ========================================================================
    // 文字颜色
    // ========================================================================
    
    // 主文字
    readonly property color text: isDark ? "#CDD6F4" : "#212121"
    
    // 次要文字
    readonly property color textSecondary: isDark ? "#A6ADC8" : "#757575"
    
    // 禁用文字
    readonly property color textDisabled: isDark ? "#6C7086" : "#BDBDBD"
    
    // 反色文字 (用于深色背景上)
    readonly property color textOnAccent: "#FFFFFF"
    readonly property color textOnPrimary: "#FFFFFF"
    
    // ========================================================================
    // 边框颜色
    // ========================================================================
    
    readonly property color border: isDark ? "#45475A" : "#E0E0E0"
    readonly property color borderFocus: accent
    readonly property color borderHover: isDark ? "#585B70" : "#BDBDBD"
    
    // ========================================================================
    // 状态颜色
    // ========================================================================
    
    readonly property color success: "#A6E3A1"
    readonly property color warning: "#F9E2AF"
    readonly property color error: "#F38BA8"
    readonly property color info: mtfBlue
    
    // ========================================================================
    // 特殊元素
    // ========================================================================
    
    // 导航栏
    readonly property color navBar: isDark ? "#181825" : "#F5F5F5"
    readonly property color navBarSelected: isDark ? Qt.rgba(accent.r, accent.g, accent.b, 0.2) 
                                                   : Qt.rgba(accent.r, accent.g, accent.b, 0.15)
    
    // 滚动条
    readonly property color scrollbar: isDark ? "#45475A" : "#BDBDBD"
    readonly property color scrollbarHover: isDark ? "#585B70" : "#9E9E9E"
    
    // 分隔线
    readonly property color divider: isDark ? "#313244" : "#EEEEEE"
    
    // 阴影 (用于 DropShadow)
    readonly property color shadow: isDark ? "#00000080" : "#00000020"
    
    // ========================================================================
    // 壁纸卡片专用
    // ========================================================================
    
    readonly property color cardHover: isDark ? "#3B3D4F" : "#F5F5F5"
    readonly property color cardSelected: isDark ? Qt.rgba(mtfPink.r, mtfPink.g, mtfPink.b, 0.3)
                                                 : Qt.rgba(mtfPink.r, mtfPink.g, mtfPink.b, 0.2)
    readonly property color cardLocked: isDark ? Qt.rgba(1, 0.8, 0, 0.15) : Qt.rgba(1, 0.8, 0, 0.1)
    readonly property color cardCooling: isDark ? Qt.rgba(0.5, 0.5, 1, 0.15) : Qt.rgba(0.5, 0.5, 1, 0.1)
    
    // ========================================================================
    // 渐变
    // ========================================================================
    
    // 标题渐变 (蓝→粉)
    readonly property var gradientAccent: Gradient {
        GradientStop { position: 0.0; color: theme.mtfBlue }
        GradientStop { position: 1.0; color: theme.mtfPink }
    }
    
    // ========================================================================
    // 尺寸 & 间距
    // ========================================================================
    
    readonly property int radiusSmall: 4
    readonly property int radiusMedium: 8
    readonly property int radiusLarge: 12
    
    readonly property int spacingTiny: 4
    readonly property int spacingSmall: 8
    readonly property int spacingMedium: 16
    readonly property int spacingLarge: 24
    
    // ========================================================================
    // 字体大小
    // ========================================================================
    
    readonly property int fontSizeSmall: 12
    readonly property int fontSizeMedium: 14
    readonly property int fontSizeLarge: 16
    readonly property int fontSizeTitle: 20
    readonly property int fontSizeHeader: 24
}
