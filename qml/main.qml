import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LianwallGui
import "." as App
import "components" as C
import "pages" as Pages
ApplicationWindow {
    id: root
    objectName: "mainWindow"
    width: 1280; height: 720
    minimumWidth: 840; minimumHeight: 600
    visible: false
    title: "LianWall"
    font.pixelSize: 13
    palette.window: App.Theme.surface
    palette.windowText: App.Theme.text
    palette.text: App.Theme.text
    palette.base: App.Theme.input
    palette.button: App.Theme.surface
    palette.placeholderText: App.Theme.textSecondary
    palette.buttonText: App.Theme.text
    palette.highlight: App.Theme.accent
    palette.highlightedText: App.Theme.textOnAccent
    color: App.Theme.background
    property int currentPage: 0
    readonly property bool busy: LianwallApp.wallpaperSwitching || Frontend.runtime.phase === "preparing"
    function notify(message) { toastLabel.text = message; toast.open(); toastTimer.restart() }
    onClosing: function(close) { close.accepted = false; LianwallApp.hideMainWindow() }
    onVisibleChanged: if (visible && DaemonState.daemonConnected) { DaemonState.refresh(); Frontend.refreshRotation(); Frontend.refreshEngine() }
    Component.onCompleted: { App.Theme.current = ConfigManager.theme; App.Theme.accentScheme = ConfigManager.accentColor }
    Connections {
        target: ConfigManager
        function onThemeChanged(theme) { App.Theme.current = theme }
        function onAccentColorChanged(accent) { App.Theme.accentScheme = accent }
    }
    Connections {
        target: DaemonState
        function onDaemonError(code, message, recoverable) { if (code !== "connection_lost") root.notify(message) }
    }
    header: Rectangle {
        height: 54; color: App.Theme.surface
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: App.Theme.border }
        Row {
            id: tabs
            anchors.centerIn: parent
            height: parent.height
            spacing: 4
            Repeater {
                model: [
                    { icon: "player", label: qsTr("正在播放") }, { icon: "library", label: qsTr("壁纸库") },
                    { icon: "engine", label: qsTr("引擎") }, { icon: "automation", label: qsTr("自动化") },
                    { icon: "settings", label: qsTr("设置") }
                ]
                delegate: Button {
                    id: tab
                    required property int index
                    required property var modelData
                    width: Math.max(96, contentItem.implicitWidth + 24); height: tabs.height
                    hoverEnabled: true
                    objectName: "tab_" + index
                    background: Rectangle { color: tab.hovered ? App.Theme.cardHover : "transparent" }
                    contentItem: RowLayout {
                        spacing: 7
                        C.UiIcon { name: tab.modelData.icon; color: root.currentPage === tab.index ? App.Theme.accent : App.Theme.textSecondary; Layout.preferredWidth: 16; Layout.preferredHeight: 16 }
                        Text { textFormat: Text.PlainText; text: tab.modelData.label; font.pixelSize: 12; color: root.currentPage === tab.index ? App.Theme.accent : App.Theme.textSecondary }
                    }
                    Rectangle { visible: root.currentPage === tab.index; anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter; width: 65; height: 3; radius: 2; color: App.Theme.accent }
                    onClicked: root.currentPage = index
                    Keys.onRightPressed: root.currentPage = (index + 1) % 5
                    Keys.onLeftPressed: root.currentPage = (index + 4) % 5
                }
            }
        }
        Row {
            anchors.right: parent.right; anchors.rightMargin: 12; anchors.verticalCenter: parent.verticalCenter
            spacing: 4
            C.UiButton { iconName: App.Theme.isDark ? "sun" : "moon"; kind: "flat"; hint: qsTr("切换深浅主题"); onClicked: ConfigManager.setTheme(App.Theme.isDark ? "lian" : "lian-dark") }
            C.UiButton { iconName: "more"; kind: "flat"; hint: qsTr("应用菜单"); onClicked: appMenu.open() }
        }
    }
    StackLayout {
        anchors.fill: parent
        anchors.margins: 16
        currentIndex: root.currentPage
        Pages.PlayerPage { onNavigate: function(page) { root.currentPage = page } }
        Pages.LibraryPage { activated: root.currentPage === 1 }
        Pages.EnginePage {}
        Pages.AutomationPage {}
        Pages.PreferencesPage {}
    }
    footer: ColumnLayout {
        spacing: 0
        Rectangle {
            Layout.fillWidth: true; implicitHeight: 58; color: App.Theme.surface
            Rectangle { width: parent.width; height: 1; color: App.Theme.border }
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 16
                spacing: 12
                Image { Layout.preferredWidth: 52; Layout.preferredHeight: 34; source: DaemonState.currentPath ? "image://thumbnail/" + encodeURIComponent(DaemonState.currentPath) : ""; sourceSize: Qt.size(320, 180); fillMode: Image.PreserveAspectFit; asynchronous: true }
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 3
                    Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: DaemonState.currentFilename || qsTr("暂无壁纸"); color: App.Theme.text; font.pixelSize: 11; elide: Text.ElideMiddle }
                    Text { textFormat: Text.PlainText; text: root.busy ? qsTr("正在切换，等待确认…") : DaemonState.engine; font.pixelSize: 10; color: App.Theme.textSecondary }
                }
                C.UiButton { iconName: "prev"; kind: "flat"; hint: qsTr("上一张壁纸"); enabled: DaemonState.daemonConnected && !root.busy; onClicked: LianwallApp.daemonPrev() }
                C.UiButton { text: Frontend.rotationPaused ? (Frontend.temporaryPause ? qsTr("恢复轮换") : qsTr("轮换已关闭")) : qsTr("暂停轮换"); iconName: Frontend.rotationPaused ? "refresh" : "pause"; kind: "tonal"; hint: qsTr("只控制自动轮换，视频继续播放"); enabled: DaemonState.daemonConnected; onClicked: { if (Frontend.rotationPaused && !Frontend.temporaryPause) root.currentPage = 4; else Frontend.toggleRotation() } }
                C.UiButton { iconName: "next"; kind: "flat"; hint: qsTr("下一张壁纸"); enabled: DaemonState.daemonConnected && !root.busy; onClicked: LianwallApp.daemonNext() }
                Item { Layout.fillWidth: true }
                Text { textFormat: Text.PlainText; text: root.busy ? qsTr("切换中") : Frontend.runtime.phase === "failed" ? qsTr("引擎已停止") : DaemonState.daemonConnected ? qsTr("服务运行中") : qsTr("未连接"); font.pixelSize: 10; color: Frontend.runtime.phase === "failed" ? App.Theme.error : App.Theme.textSecondary }
            }
        }
        Rectangle {
            Layout.fillWidth: true; implicitHeight: 24; color: App.Theme.input
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12
                spacing: 7
                Rectangle { width: 5; height: 5; radius: 3; color: DaemonState.daemonConnected ? App.Theme.success : App.Theme.error }
                Text { textFormat: Text.PlainText; text: DaemonState.daemonConnected ? qsTr("壁纸服务已连接") : qsTr("壁纸服务未连接"); color: App.Theme.textSecondary; font.pixelSize: 10 }
                Text { textFormat: Text.PlainText; text: "· " + (Frontend.runtime.decoder || DaemonState.engine); color: App.Theme.textSecondary; font.pixelSize: 10 }
                Item { Layout.fillWidth: true }
                Text { textFormat: Text.PlainText; text: "LianWall · " + AppVersion; color: App.Theme.textSecondary; font.pixelSize: 10 }
            }
        }
    }
    Shortcut { sequence: "Alt+1"; onActivated: root.currentPage = 0 }
    Shortcut { sequence: "Alt+2"; onActivated: root.currentPage = 1 }
    Shortcut { sequence: "Alt+3"; onActivated: root.currentPage = 2 }
    Shortcut { sequence: "Alt+4"; onActivated: root.currentPage = 3 }
    Shortcut { sequence: "Alt+5"; onActivated: root.currentPage = 4 }
    Menu {
        id: appMenu
        x: root.width - width - 12; y: 50
        MenuItem { text: qsTr("关于 LianWall"); onTriggered: about.open() }
        MenuItem { text: qsTr("打开配置目录"); onTriggered: ConfigManager.openDirectory(ConfigManager.configDirPath()) }
        MenuSeparator {}
        MenuItem { text: qsTr("退出 GUI 与服务"); onTriggered: LianwallApp.quit() }
    }
    Dialog {
        id: about
        title: qsTr("关于 LianWall")
        anchors.centerIn: parent
        width: 410; modal: true; standardButtons: Dialog.Close
        background: Rectangle { color: App.Theme.surface; radius: 12; border.color: App.Theme.border }
        ColumnLayout {
            width: parent.width; spacing: 12
            Image { source: "qrc:/icons/lianwall.png"; Layout.preferredWidth: 88; Layout.preferredHeight: 88; Layout.alignment: Qt.AlignHCenter; fillMode: Image.PreserveAspectFit }
            Text { textFormat: Text.PlainText; text: "LianWall"; font.pixelSize: 25; color: App.Theme.text; Layout.alignment: Qt.AlignHCenter }
            Text { textFormat: Text.PlainText; text: qsTr("壁纸、图片 / 视频播放与桌面自动化"); font.pixelSize: 12; color: App.Theme.textSecondary; Layout.alignment: Qt.AlignHCenter }
            Text { textFormat: Text.PlainText; text: "GUI " + AppVersion + " · " + AppAuthor; font.pixelSize: 11; color: App.Theme.textSecondary; Layout.alignment: Qt.AlignHCenter }
        }
    }
    Popup {
        id: toast
        anchors.centerIn: Overlay.overlay
        y: root.height - height - 110
        width: Math.min(root.width - 60, 600)
        padding: 14
        background: Rectangle { color: App.Theme.text; radius: 10 }
        contentItem: Text { textFormat: Text.PlainText; id: toastLabel; color: App.Theme.surface; font.pixelSize: 12; wrapMode: Text.Wrap }
    }
    Timer { id: toastTimer; interval: 6000; onTriggered: toast.close() }
}
