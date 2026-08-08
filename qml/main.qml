import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LianwallGui
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
        LianwallApp.hideMainWindow()
    }

    // 窗口重新显示时刷新 daemon 状态（恢复倒计时、VRAM 等）
    onVisibleChanged: {
        if (visible && DaemonState.daemonConnected) {
            DaemonState.refresh()
        }
    }

    // Daemon 错误提示
    Connections {
        target: DaemonState
        function onDaemonError(code, message, recoverable) {
            // 连接抖动 / IPC 超时：不是业务失败，不弹横幅
            if (code === "connection_lost" || code === "timeout")
                return
            errorLabel.text = message
            errorPopup.open()
            errorAutoClose.interval = 5000
        }
    }

    // 顶部错误横幅
    Popup {
        id: errorPopup
        x: (parent.width - width) / 2
        y: 8
        width: Math.min(parent.width - 40, 480)
        modal: false
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            radius: 8
            color: "#D32F2F"
            opacity: 0.95
        }

        contentItem: RowLayout {
            spacing: 8
            Label {
                id: errorLabel
                Layout.fillWidth: true
                color: "white"
                font.pixelSize: 13
                wrapMode: Text.Wrap
            }
            Label {
                text: "✕"
                color: "white"
                font.pixelSize: 14
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: errorPopup.close()
                }
            }
        }

        // 5 秒后自动关闭
        Timer {
            id: errorAutoClose
            interval: 5000
            running: errorPopup.visible
            onTriggered: errorPopup.close()
        }
    }

    // 主题初始化
    Component.onCompleted: {
        App.Theme.current = ConfigManager.theme
        App.Theme.accentScheme = ConfigManager.accentColor
    }

    Connections {
        target: ConfigManager
        function onThemeChanged(newTheme) {
            App.Theme.current = newTheme
        }
        function onAccentColorChanged(newAccent) {
            App.Theme.accentScheme = newAccent
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

            // 0 - Dashboard
            Pages.DashboardPage {}

            // 1 - Library
            Pages.LibraryPage {
                activated: root.currentPage === 1
            }

            // 2 - Settings
            Pages.SettingsPage {}

            // 3 - About
            Pages.AboutPage {}
        }
    }
}
