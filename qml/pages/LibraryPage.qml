import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".." as App
import "../components" as C
import "../dialogs" as Dialogs
Item {
    id: root
    property bool activated: false
    property string media: "All"
    property bool listMode: false
    onActivatedChanged: if (activated) refresh.restart()
    function reload() { if (activated && DaemonState.daemonConnected) { WallpaperModel.load(media); WallpaperModel.refreshMetadata() } }
    Timer { id: refresh; interval: 120; onTriggered: root.reload() }
    Connections {
        target: WallpaperModel
        function onMetadataChanged() {
            if (WallpaperModel.metadataUnsupported) {
                WallpaperFilterModel.favoritesOnly = false
                if (["name", "modified"].indexOf(WallpaperFilterModel.sortKey) < 0) WallpaperFilterModel.sortKey = "name"
            }
        }
    }
    Connections {
        target: DaemonState
        function onDaemonConnectedChanged() { if (DaemonState.daemonConnected && root.activated) refresh.restart() }
        function onSpaceUpdated() { if (root.activated) refresh.restart() }
        function onWallpaperChanged() { if (root.activated) WallpaperModel.refreshMetadata() }
        function onModeChanged() { if (root.activated) WallpaperModel.refreshMetadata() }
    }
    ColumnLayout {
        anchors.fill: parent; spacing: 10
        RowLayout {
            Layout.fillWidth: true
            Text { textFormat: Text.PlainText; text: qsTr("壁纸库"); color: App.Theme.text; font.pixelSize: 18; font.weight: Font.DemiBold }
            Text { textFormat: Text.PlainText; text: qsTr("管理、筛选与查找壁纸"); color: App.Theme.textSecondary; font.pixelSize: 11 }
            Item { Layout.fillWidth: true }
            C.UiButton {
                text: ConfigManager.imageDir === ConfigManager.videoDir ? qsTr("打开壁纸目录") : qsTr("打开图片目录")
                iconName: "folder"; enabled: ConfigManager.imageDir.length > 0
                onClicked: ConfigManager.openDirectory(ConfigManager.imageDir)
            }
            C.UiButton {
                text: qsTr("打开视频目录"); iconName: "folder"
                visible: ConfigManager.imageDir !== ConfigManager.videoDir
                enabled: ConfigManager.videoDir.length > 0
                onClicked: ConfigManager.openDirectory(ConfigManager.videoDir)
            }
            C.UiButton { iconName: "refresh"; hint: qsTr("重新扫描壁纸"); enabled: DaemonState.daemonConnected && !WallpaperModel.loading; onClicked: LianwallApp.daemonRescan() }
        }
        RowLayout {
            Layout.fillWidth: true; spacing: 9
            TextField {
                Layout.fillWidth: true; implicitHeight: 35
                placeholderText: qsTr("搜索名称、文件或目录…")
                placeholderTextColor: App.Theme.textSecondary
                color: App.Theme.text; font.pixelSize: 12; selectByMouse: true
                leftPadding: 12
                background: Rectangle { color: App.Theme.surface; radius: 8; border.color: parent.activeFocus ? App.Theme.accent : App.Theme.border }
                onTextChanged: WallpaperFilterModel.searchText = text
            }
            C.UiComboBox {
                property var keys: ["name", "modified", "added", "recent", "count", "duration"]
                model: WallpaperModel.metadataAvailable ? [qsTr("名称 A–Z"), qsTr("最近修改"), qsTr("最近添加"), qsTr("最近使用"), qsTr("切换次数"), qsTr("驻留时长")] : [qsTr("名称 A–Z"), qsTr("最近修改")]
                currentIndex: Math.max(0, keys.indexOf(WallpaperFilterModel.sortKey))
                onActivated: WallpaperFilterModel.sortKey = keys[index]
                font.pixelSize: 12; implicitHeight: 35
            }
            C.UiButton { iconName: "library"; kind: "flat"; selected: !root.listMode; hint: qsTr("网格视图"); onClicked: root.listMode = false }
            C.UiButton { iconName: "list"; kind: "flat"; selected: root.listMode; hint: qsTr("列表视图"); onClicked: root.listMode = true }
        }
        RowLayout {
            Layout.fillWidth: true; spacing: 5
            Repeater {
                model: [{name:qsTr("全部"),value:"All"},{name:qsTr("视频"),value:"Video"},{name:qsTr("图片"),value:"Image"}]
                C.UiButton { required property var modelData; text: modelData.name; selected: root.media === modelData.value; kind: "flat"; onClicked: { root.media = modelData.value; root.reload() } }
            }
            C.UiButton { text: qsTr("收藏"); iconName: "heart"; visible: WallpaperModel.metadataAvailable; selected: WallpaperFilterModel.favoritesOnly; kind: "flat"; onClicked: WallpaperFilterModel.favoritesOnly = !WallpaperFilterModel.favoritesOnly }
            C.UiButton { text: qsTr("已排除轮换"); iconName: "lock"; selected: WallpaperFilterModel.lockFilter === 1; kind: "flat"; onClicked: WallpaperFilterModel.lockFilter = WallpaperFilterModel.lockFilter === 1 ? -1 : 1 }
            Item { Layout.fillWidth: true }
            BusyIndicator { running: WallpaperModel.loading; visible: running; implicitWidth: 24; implicitHeight: 24 }
            Text { textFormat: Text.PlainText; text: qsTr("%1 / %2 张").arg(WallpaperFilterModel.count).arg(WallpaperModel.count); color: App.Theme.textSecondary; font.pixelSize: 11 }
        }
        Text { Layout.fillWidth: true; visible: WallpaperModel.metadataError.length > 0; text: qsTr("收藏与统计：%1").arg(WallpaperModel.metadataError); textFormat: Text.PlainText; color: App.Theme.warning; font.pixelSize: 11; wrapMode: Text.Wrap }
        GridView {
            id: grid
            Layout.fillWidth: true; Layout.fillHeight: true
            model: WallpaperFilterModel
            clip: true
            cellWidth: root.listMode ? width : width / Math.max(2, Math.floor(width / 235))
            readonly property int actionSize: 32
            readonly property int footerPadding: 8
            readonly property int footerHeight: 2 * footerPadding + titleMetrics.height + 4 + actionSize
            FontMetrics { id: titleMetrics; font.pixelSize: 12 }
            cellHeight: root.listMode ? Math.max(87, footerHeight + 10) : 15 + (cellWidth - 20) * 9 / 16 + footerHeight
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOff }
            delegate: Item {
                id: entry
                required property int index
                required property bool wallpaperOutsideSchedule
                required property var wallpaperTimeConstraints
                readonly property string schedule: wallpaperTimeConstraints.join(" ∩ ")
                required property string wallpaperPath
                required property string wallpaperFilename
                required property bool wallpaperLocked
                required property bool wallpaperIsCurrent
                required property bool wallpaperFavorite
                required property real wallpaperPlayCount
                required property real wallpaperDisplaySeconds
                required property bool wallpaperIsVideo
                width: grid.cellWidth; height: grid.cellHeight
                Rectangle {
                    anchors.fill: parent; anchors.rightMargin: 10; anchors.bottomMargin: 10
                    color: App.Theme.surface; radius: 10; clip: true
                    border.color: entry.wallpaperIsCurrent ? App.Theme.accent : App.Theme.border
                    border.width: entry.wallpaperIsCurrent ? 2 : 1
                    Image {
                        id: art
                        x: 5; y: 5
                        width: root.listMode ? 122 : parent.width - 10
                        height: root.listMode ? parent.height - 10 : width * 9 / 16
                        source: "image://thumbnail/" + encodeURIComponent(entry.wallpaperPath) + "?t=" + WallpaperModel.refreshToken
                        sourceSize: Qt.size(640, 360); asynchronous: true; cache: true
                        fillMode: Image.PreserveAspectCrop

                    }
                    Rectangle {
                        x: 8; y: 8; radius: 5; height: 21; width: tag.implicitWidth + 12
                        color: "#a0303040"
                        Text { textFormat: Text.PlainText; id: tag; anchors.centerIn: parent; text: entry.wallpaperIsCurrent ? qsTr("正在使用") : entry.wallpaperIsVideo ? qsTr("视频") : qsTr("图片"); color: "white"; font.pixelSize: 10 }
                    }
                    ColumnLayout {
                        x: root.listMode ? 138 : 11
                        y: (root.listMode ? 0 : art.y + art.height) + grid.footerPadding
                        height: parent.height - y - grid.footerPadding

                        width: parent.width - x - 10
                        spacing: 4
                        Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: entry.wallpaperFilename; color: App.Theme.text; font.pixelSize: 12; elide: Text.ElideMiddle }
                        RowLayout {
                            Layout.fillWidth: true; Layout.preferredHeight: grid.actionSize; spacing: 4
                            Text { textFormat: Text.PlainText; Layout.fillWidth: true; elide: Text.ElideRight; Layout.minimumWidth: 0; text: entry.wallpaperOutsideSchedule ? qsTr("时段外") + " · " + entry.schedule : entry.schedule || (entry.wallpaperLocked ? qsTr("已排除自动轮换") : entry.wallpaperIsVideo ? qsTr("视频壁纸") : qsTr("静态壁纸")); color: App.Theme.textSecondary; font.pixelSize: 10 }
                            C.UiButton { iconName: "heart"; visible: WallpaperModel.metadataAvailable; selected: entry.wallpaperFavorite; kind: "flat"; padding: (grid.actionSize - 17) / 2; implicitWidth: grid.actionSize; implicitHeight: grid.actionSize; hint: entry.wallpaperFavorite ? qsTr("取消收藏") : qsTr("收藏壁纸"); enabled: DaemonState.daemonConnected && WallpaperModel.pendingFavorites.indexOf(entry.wallpaperPath) < 0; onClicked: WallpaperModel.setFavorite(entry.wallpaperPath, !entry.wallpaperFavorite) }
                            C.UiButton { iconName: "lock"; selected: entry.wallpaperLocked; kind: "flat"; padding: (grid.actionSize - 17) / 2; implicitWidth: grid.actionSize; implicitHeight: grid.actionSize; hint: qsTr("排除 / 加入自动轮换"); enabled: DaemonState.daemonConnected && !entry.wallpaperOutsideSchedule; onClicked: WallpaperFilterModel.toggleLockByPath(entry.wallpaperPath) }
                            C.UiButton { iconName: "play"; kind: "flat"; padding: (grid.actionSize - 17) / 2; implicitWidth: grid.actionSize; implicitHeight: grid.actionSize; hint: qsTr("设为壁纸"); enabled: DaemonState.daemonConnected && !LianwallApp.wallpaperSwitching; onClicked: WallpaperFilterModel.setAsCurrentByPath(entry.wallpaperPath) }
                        }
                    }
                    TapHandler { enabled: DaemonState.daemonConnected && !LianwallApp.wallpaperSwitching; acceptedButtons: Qt.LeftButton; onDoubleTapped: WallpaperFilterModel.setAsCurrentByPath(entry.wallpaperPath) }
                    HoverHandler { id: hover }
                    C.UiToolTip {
                    visible: hover.hovered
                    text: entry.wallpaperPath + (entry.schedule ? "\n" + entry.schedule : "") + "\n" + qsTr("双击设为壁纸") + (WallpaperModel.metadataAvailable ? "\n" + qsTr("成功切换 %1 次 · 驻留 %2 分钟").arg(entry.wallpaperPlayCount).arg(Math.floor(entry.wallpaperDisplaySeconds / 60)) : "")
                    delay: 900
                    }
                }
            }
            Text { textFormat: Text.PlainText; anchors.centerIn: parent; visible: grid.count === 0 && !WallpaperModel.loading; text: DaemonState.daemonConnected ? qsTr("没有匹配的壁纸，试试其他搜索或筛选条件。") : qsTr("连接壁纸服务后查看媒体库。"); color: App.Theme.textSecondary; font.pixelSize: 12 }
        }
    }
}
