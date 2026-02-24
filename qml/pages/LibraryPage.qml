import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".." as App
import "../dialogs" as Dialogs

/// Library 壁纸库
/// GridView 显示当前模式所有壁纸 + 搜索/过滤 + 详情弹窗
Item {
    id: libRoot

    // 页面激活时懒加载
    property bool activated: false
    onActivatedChanged: {
        if (activated && DaemonState.daemonConnected) {
            WallpaperModel.load()
        }
    }

    // daemon 连接后也触发加载
    Connections {
        target: DaemonState
        function onDaemonConnectedChanged() {
            if (DaemonState.daemonConnected && libRoot.activated) {
                WallpaperModel.load()
            }
        }
    }

    // daemon SpaceUpdated / 壁纸切换后自动刷新
    Connections {
        target: DaemonState
        function onTotalWallpapersChanged() {
            if (libRoot.activated && DaemonState.daemonConnected) {
                refreshTimer.restart()
            }
        }
        function onWallpaperChanged(path, filename, trigger) {
            if (libRoot.activated && DaemonState.daemonConnected) {
                refreshTimer.restart()
            }
        }
    }

    // 防抖定时器（避免频繁刷新）
    Timer {
        id: refreshTimer
        interval: 500
        repeat: false
        onTriggered: WallpaperModel.load()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: App.Theme.spacingMedium
        spacing: App.Theme.spacingMedium

        // ================================================================
        // 顶部工具栏：搜索 + 过滤 + 刷新
        // ================================================================
        RowLayout {
            Layout.fillWidth: true
            spacing: App.Theme.spacingSmall

            // 搜索框
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                radius: App.Theme.radiusMedium
                color: App.Theme.surface
                border.width: 1
                border.color: searchInput.activeFocus ? App.Theme.accent : App.Theme.border

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: App.Theme.spacingSmall
                    anchors.rightMargin: App.Theme.spacingSmall
                    spacing: App.Theme.spacingSmall

                    Text {
                        text: "🔍"
                        font.pixelSize: App.Theme.fontSizeMedium
                        color: App.Theme.textSecondary
                    }

                    TextInput {
                        id: searchInput
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: App.Theme.fontSizeMedium
                        color: App.Theme.text
                        clip: true
                        onTextChanged: WallpaperFilterModel.searchText = text

                        Text {
                            anchors.fill: parent
                            verticalAlignment: Text.AlignVCenter
                            text: qsTr("搜索壁纸...")
                            font.pixelSize: App.Theme.fontSizeMedium
                            color: App.Theme.textSecondary
                            visible: !searchInput.text && !searchInput.activeFocus
                        }
                    }

                    // 清除按钮
                    Text {
                        text: "✕"
                        font.pixelSize: App.Theme.fontSizeSmall
                        color: App.Theme.textSecondary
                        visible: searchInput.text.length > 0
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: searchInput.text = ""
                        }
                    }
                }
            }

            // 锁定状态过滤
            Row {
                spacing: 2

                FilterChip {
                    text: qsTr("全部")
                    selected: WallpaperFilterModel.lockFilter === -1
                    onClicked: WallpaperFilterModel.lockFilter = -1
                }
                FilterChip {
                    text: qsTr("🔓 未锁定")
                    selected: WallpaperFilterModel.lockFilter === 0
                    onClicked: WallpaperFilterModel.lockFilter = 0
                }
                FilterChip {
                    text: qsTr("🔒 已锁定")
                    selected: WallpaperFilterModel.lockFilter === 1
                    onClicked: WallpaperFilterModel.lockFilter = 1
                }
            }

            // 刷新按钮
            Rectangle {
                implicitWidth: refreshRow.implicitWidth + 16
                height: 36
                radius: App.Theme.radiusMedium
                color: refreshMouse.pressed ? App.Theme.cardHover
                       : refreshMouse.containsMouse ? App.Theme.surface : "transparent"
                border.width: 1
                border.color: App.Theme.border

                Row {
                    id: refreshRow
                    anchors.centerIn: parent
                    spacing: 4

                    Text {
                        id: refreshIcon
                        text: "🔄"
                        font.pixelSize: 16

                        RotationAnimation on rotation {
                            running: WallpaperModel.loading
                            from: 0; to: 360
                            duration: 1000
                            loops: Animation.Infinite
                        }
                    }

                    Text {
                        text: qsTr("刷新")
                        font.pixelSize: App.Theme.fontSizeSmall
                        color: App.Theme.text
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                MouseArea {
                    id: refreshMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        LianwallApp.daemonReloadConfig()
                        WallpaperModel.load()
                    }
                }
            }
        }

        // 统计信息
        RowLayout {
            Layout.fillWidth: true
            spacing: App.Theme.spacingSmall

            Text {
                text: qsTr("模式: %1").arg(WallpaperModel.mode || DaemonState.mode)
                font.pixelSize: App.Theme.fontSizeSmall
                color: App.Theme.textSecondary
            }
            Text {
                text: "•"
                font.pixelSize: App.Theme.fontSizeSmall
                color: App.Theme.textSecondary
            }
            Text {
                text: qsTr("显示 %1 / %2").arg(WallpaperFilterModel.count).arg(WallpaperModel.count)
                font.pixelSize: App.Theme.fontSizeSmall
                color: App.Theme.textSecondary
            }
            Item { Layout.fillWidth: true }
        }

        // ================================================================
        // 壁纸网格
        // ================================================================
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // 加载状态
            ColumnLayout {
                anchors.centerIn: parent
                spacing: App.Theme.spacingSmall
                visible: WallpaperModel.loading && WallpaperModel.count === 0

                BusyIndicator {
                    Layout.alignment: Qt.AlignHCenter
                    running: WallpaperModel.loading
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("加载中...")
                    font.pixelSize: App.Theme.fontSizeMedium
                    color: App.Theme.textSecondary
                }
            }

            // 空状态
            ColumnLayout {
                anchors.centerIn: parent
                spacing: App.Theme.spacingSmall
                visible: !WallpaperModel.loading && WallpaperModel.count === 0 && DaemonState.daemonConnected

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "📭"
                    font.pixelSize: 48
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("暂无壁纸")
                    font.pixelSize: App.Theme.fontSizeMedium
                    color: App.Theme.textSecondary
                }
            }

            // 未连接
            ColumnLayout {
                anchors.centerIn: parent
                spacing: App.Theme.spacingSmall
                visible: !DaemonState.daemonConnected

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "🔌"
                    font.pixelSize: 48
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("等待守护进程连接...")
                    font.pixelSize: App.Theme.fontSizeMedium
                    color: App.Theme.textSecondary
                }
            }

            // 过滤无结果
            ColumnLayout {
                anchors.centerIn: parent
                spacing: App.Theme.spacingSmall
                visible: !WallpaperModel.loading && WallpaperModel.count > 0 && WallpaperFilterModel.count === 0

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "🔍"
                    font.pixelSize: 48
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("未找到匹配的壁纸")
                    font.pixelSize: App.Theme.fontSizeMedium
                    color: App.Theme.textSecondary
                }
            }

            GridView {
                id: wallpaperGrid
                anchors.fill: parent
                visible: WallpaperFilterModel.count > 0
                clip: true

                cellWidth: {
                    // 自适应列数：最少 2 列，每列最小 160px
                    var cols = Math.max(2, Math.floor(width / 200))
                    return Math.floor(width / cols)
                }
                cellHeight: cellWidth * 9 / 16 + 32   // 16:9 缩略图 + 文件名高度

                model: WallpaperFilterModel

                delegate: Item {
                    width: wallpaperGrid.cellWidth
                    height: wallpaperGrid.cellHeight

                    Rectangle {
                        id: thumbCard
                        anchors.fill: parent
                        anchors.margins: 4
                        radius: App.Theme.radiusMedium
                        color: thumbMouse.containsMouse ? App.Theme.cardHover : App.Theme.card
                        border.width: wallpaperIsCurrent ? 2 : 1
                        border.color: wallpaperIsCurrent ? App.Theme.accent : App.Theme.border
                        clip: true

                        Behavior on color {
                            ColorAnimation { duration: 150 }
                        }

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 0

                            // 缩略图区域
                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true

                                Image {
                                    id: thumbImage
                                    anchors.fill: parent
                                    anchors.margins: 1
                                    source: {
                                        if (wallpaperIsVideo) {
                                            return "image://thumbnail/" + encodeURIComponent(wallpaperPath)
                                        }
                                        return "image://thumbnail/" + encodeURIComponent(wallpaperPath)
                                    }
                                    fillMode: Image.PreserveAspectCrop
                                    asynchronous: true
                                    cache: true

                                    // 加载占位
                                    Rectangle {
                                        anchors.fill: parent
                                        color: App.Theme.surface
                                        visible: thumbImage.status !== Image.Ready

                                        Text {
                                            anchors.centerIn: parent
                                            text: wallpaperIsVideo ? "🎬" : "🖼️"
                                            font.pixelSize: 24
                                        }

                                        BusyIndicator {
                                            anchors.bottom: parent.bottom
                                            anchors.horizontalCenter: parent.horizontalCenter
                                            anchors.bottomMargin: 4
                                            width: 16; height: 16
                                            running: thumbImage.status === Image.Loading
                                            visible: running
                                        }
                                    }
                                }

                                // 状态标记（左上角）
                                Row {
                                    anchors.top: parent.top
                                    anchors.left: parent.left
                                    anchors.margins: 4
                                    spacing: 2

                                    // 当前播放标记
                                    Rectangle {
                                        width: 20; height: 20
                                        radius: 10
                                        color: Qt.rgba(App.Theme.accent.r, App.Theme.accent.g, App.Theme.accent.b, 0.9)
                                        visible: wallpaperIsCurrent
                                        Text {
                                            anchors.centerIn: parent
                                            text: "▶"
                                            font.pixelSize: 10
                                            color: App.Theme.textOnAccent
                                        }
                                    }

                                    // 锁定标记
                                    Rectangle {
                                        width: 20; height: 20
                                        radius: 10
                                        color: Qt.rgba(0, 0, 0, 0.6)
                                        visible: wallpaperLocked
                                        Text {
                                            anchors.centerIn: parent
                                            text: "🔒"
                                            font.pixelSize: 10
                                        }
                                    }

                                    // 冷却标记
                                    Rectangle {
                                        width: 20; height: 20
                                        radius: 10
                                        color: Qt.rgba(0, 0, 0, 0.6)
                                        visible: wallpaperInCooldown
                                        Text {
                                            anchors.centerIn: parent
                                            text: "❄️"
                                            font.pixelSize: 10
                                        }
                                    }
                                }
                            }

                            // 文件名
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 28
                                color: "transparent"

                                Text {
                                    anchors.fill: parent
                                    anchors.leftMargin: App.Theme.spacingSmall
                                    anchors.rightMargin: App.Theme.spacingSmall
                                    text: wallpaperFilename
                                    font.pixelSize: App.Theme.fontSizeSmall
                                    color: App.Theme.text
                                    elide: Text.ElideMiddle
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                        }

                        MouseArea {
                            id: thumbMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                detailDialog.wallpaperRow = index
                                detailDialog.wallpaperPath = wallpaperPath
                                detailDialog.wallpaperFilename = wallpaperFilename
                                detailDialog.wallpaperLocked = wallpaperLocked
                                detailDialog.wallpaperIsCurrent = wallpaperIsCurrent
                                detailDialog.wallpaperIsVideo = wallpaperIsVideo
                                detailDialog.open()
                            }
                        }
                    }
                }

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }
            }
        }
    }

    // ================================================================
    // 详情弹窗
    // ================================================================
    Dialogs.WallpaperDetailDialog {
        id: detailDialog
        parent: Overlay.overlay
    }

    // ================================================================
    // 内联组件
    // ================================================================

    /// 过滤标签
    component FilterChip: Rectangle {
        property string text: ""
        property bool selected: false
        signal clicked()

        width: chipLabel.width + App.Theme.spacingSmall * 2
        height: 32
        radius: App.Theme.radiusMedium
        color: selected ? App.Theme.accent : (chipMouse.containsMouse ? App.Theme.cardHover : "transparent")
        border.width: selected ? 0 : 1
        border.color: App.Theme.border

        Behavior on color {
            ColorAnimation { duration: 150 }
        }

        Text {
            id: chipLabel
            anchors.centerIn: parent
            text: parent.text
            font.pixelSize: App.Theme.fontSizeSmall
            color: parent.selected ? App.Theme.textOnAccent : App.Theme.text
        }

        MouseArea {
            id: chipMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
        }
    }
}
