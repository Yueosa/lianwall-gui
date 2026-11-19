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
    width: Math.min(1080, Screen.desktopAvailableWidth); height: Math.min(640, Screen.desktopAvailableHeight)
    minimumWidth: 760; minimumHeight: 520
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
            C.UiButton { id: menuButton; iconName: "more"; kind: "flat"; hint: qsTr("应用菜单"); onClicked: appMenu.open() }
        }
    }
    StackLayout {
        anchors.fill: parent
        anchors.margins: root.width >= 1000 ? 24 : 16
        currentIndex: root.currentPage
        Pages.PlayerPage { onNavigate: function(page) { root.currentPage = page } }
        Pages.LibraryPage { activated: root.currentPage === 1 }
        Pages.EnginePage {}
        Pages.AutomationPage {}
        Pages.PreferencesPage {}
    }
    Shortcut { sequence: "Alt+1"; onActivated: root.currentPage = 0 }
    Shortcut { sequence: "Alt+2"; onActivated: root.currentPage = 1 }
    Shortcut { sequence: "Alt+3"; onActivated: root.currentPage = 2 }
    Shortcut { sequence: "Alt+4"; onActivated: root.currentPage = 3 }
    Shortcut { sequence: "Alt+5"; onActivated: root.currentPage = 4 }
    component ActionMenuItem: MenuItem {
        id: item
        implicitHeight: 40
        leftPadding: 14; rightPadding: 14
        contentItem: Text {
            text: item.text; color: item.enabled ? App.Theme.text : App.Theme.textDisabled
            font.pixelSize: 13; verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: 0
            color: item.highlighted || item.hovered ? App.Theme.cardHover : "transparent"
        }
    }
    Menu {
        id: appMenu
        parent: menuButton
        x: menuButton.width - width; y: menuButton.height + 6
        popupType: Popup.Item
        width: 192; padding: 6
        background: Rectangle { color: App.Theme.surface; radius: 0; border.color: App.Theme.border }
        ActionMenuItem { text: qsTr("关于 LianWall"); onTriggered: about.open() }
        ActionMenuItem { text: qsTr("重启服务"); enabled: DaemonState.daemonConnected; onTriggered: LianwallApp.restartService() }
        ActionMenuItem { text: qsTr("退出 GUI 与服务"); onTriggered: LianwallApp.quit() }
    }
    Popup {
        id: about
        parent: Overlay.overlay; anchors.centerIn: parent
        width: Math.min(448, root.width - 48); padding: 24
        modal: true; focus: true
        background: Rectangle { color: App.Theme.surface; radius: 16; border.color: App.Theme.border }
        contentItem: ColumnLayout {
            spacing: 18
            RowLayout {
                spacing: 16
                Image { source: "qrc:/icons/lianwall.png"; Layout.preferredWidth: 56; Layout.preferredHeight: 56; fillMode: Image.PreserveAspectFit }
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 5
                    Text { text: "LianWall"; font.pixelSize: 25; font.weight: Font.DemiBold; color: App.Theme.text }
                    Text { text: "GUI " + AppVersion + "  ·  " + AppAuthor; color: App.Theme.textSecondary }
                }
            }
            Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: App.Theme.border }
            RowLayout {
                Layout.fillWidth: true; spacing: 8
                C.UiButton { text: "GitHub · CLI"; onClicked: Qt.openUrlExternally("https://github.com/Yueosa/lianwall") }
                C.UiButton { text: "GitHub · GUI"; onClicked: Qt.openUrlExternally("https://github.com/Yueosa/lianwall-gui") }
                Item { Layout.fillWidth: true }
                C.UiButton { text: qsTr("关闭"); kind: "tonal"; onClicked: about.close() }
            }
        }
    }
    Popup {
        id: toast
        x: (root.width - width) / 2
        y: root.height - height - 20
        width: Math.min(root.width - 60, 600)
        padding: 14
        background: Rectangle { color: App.Theme.text; radius: 10 }
        contentItem: Text { textFormat: Text.PlainText; id: toastLabel; color: App.Theme.surface; font.pixelSize: 12; wrapMode: Text.Wrap }
    }
    Timer { id: toastTimer; interval: 6000; onTriggered: toast.close() }
}
