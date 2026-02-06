import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LianwallGui 1.0
import "." as App
import "components" as Components
import "pages" as Pages

ApplicationWindow {
    id: root
    width: 900
    height: 640
    minimumWidth: 720
    minimumHeight: 480
    visible: false          // 静默启动，由 Application 控制
    title: qsTr("LianWall")

    color: App.Theme.background

    // 当前页面索引: 0=Dashboard, 1=Library, 2=Settings, 3=About
    property int currentPage: 0

    // 关闭窗口 = 隐藏到托盘
    onClosing: function(close) {
        close.accepted = false
        App.hideMainWindow()
    }

    // 主题初始化
    Component.onCompleted: {
        App.Theme.current = ConfigManager.theme
    }

    Connections {
        target: ConfigManager
        function onThemeChanged(newTheme) {
            App.Theme.current = newTheme
        }
    }

    // ====================================================================
    // 主布局：左侧导航栏 + 右侧内容
    // ====================================================================
    RowLayout {
        anchors.fill: parent
        spacing: 0

        // 左侧导航栏
        Components.NavBar {
            id: navBar
            Layout.fillHeight: true
            currentIndex: root.currentPage
            daemonConnected: DaemonState.daemonConnected
            onNavigated: function(index) {
                root.currentPage = index
            }
        }

        // 分隔线
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 1
            color: App.Theme.divider
        }

        // 右侧内容区
        StackLayout {
            id: contentStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: root.currentPage

            // 0 - Dashboard（占位）
            Pages.PlaceholderPage {
                pageName: "Dashboard"
                pageIcon: "🏠"
            }

            // 1 - Library（占位）
            Pages.PlaceholderPage {
                pageName: "Library"
                pageIcon: "📚"
            }

            // 2 - Settings（占位）
            Pages.PlaceholderPage {
                pageName: "Settings"
                pageIcon: "⚙️"
            }

            // 3 - About
            Pages.AboutPage {}
        }
    }
}
