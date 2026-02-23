import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import ".." as App
import "../components" as Components

/// Dashboard 仪表盘
/// 当前壁纸预览 + 状态信息 + 快捷操作
Item {
    id: dashRoot

    // 用于强制刷新预览图的计数器（每次递增，避免 Date.now() 溢出 int 上限）
    property int refreshCounter: 0

    // 判断当前壁纸是否为视频（基于文件扩展名，不依赖 mode — 因为 mode 和 path 不一定同时更新）
    readonly property bool isVideo: {
        var p = DaemonState.currentPath
        if (!p || p.length === 0) return false
        var ext = p.substring(p.lastIndexOf('.') + 1).toLowerCase()
        return ["mp4","mkv","webm","avi","mov","flv","wmv","m4v","3gp","ogv","ts","m2ts"].indexOf(ext) >= 0
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        Flickable {
            contentHeight: mainCol.height + App.Theme.spacingLarge * 2

            ColumnLayout {
                id: mainCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: App.Theme.spacingLarge
                spacing: App.Theme.spacingLarge

                // ============================================================
                // 壁纸预览卡片
                // ============================================================
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: previewCol.height + App.Theme.spacingMedium * 2
                    radius: App.Theme.radiusLarge
                    color: App.Theme.card
                    border.width: 1
                    border.color: App.Theme.border

                    ColumnLayout {
                        id: previewCol
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: App.Theme.spacingMedium
                        spacing: App.Theme.spacingMedium

                        // 壁纸预览
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: width * Screen.height / Screen.width
                            Layout.maximumHeight: 360
                            radius: App.Theme.radiusMedium
                            color: App.Theme.surface
                            clip: true

                            Image {
                                id: previewImage
                                anchors.fill: parent
                                source: {
                                    var p = DaemonState.currentPath
                                    if (!p || p.length === 0) return ""
                                    var n = dashRoot.refreshCounter
                                    if (dashRoot.isVideo) {
                                        return "image://thumbnail/" + encodeURIComponent(p) + "?t=" + n
                                    }
                                    // 图片用 fragment(#) 强制刷新，不影响 file:// 路径解析
                                    return "file://" + p + "#" + n
                                }
                                fillMode: Image.PreserveAspectFit
                                asynchronous: true
                                cache: false
                            }

                            // 无壁纸 / 加载中 / 错误占位
                            ColumnLayout {
                                anchors.centerIn: parent
                                spacing: App.Theme.spacingSmall
                                visible: previewImage.status !== Image.Ready

                                Text {
                                    Layout.alignment: Qt.AlignHCenter
                                    text: {
                                        if (!DaemonState.daemonConnected) return "🔌"
                                        if (!DaemonState.currentPath)     return "🖼️"
                                        if (previewImage.status === Image.Error) return "⚠️"
                                        return dashRoot.isVideo ? "🎬" : "🖼️"
                                    }
                                    font.pixelSize: 48
                                }
                                Text {
                                    Layout.alignment: Qt.AlignHCenter
                                    text: {
                                        if (!DaemonState.daemonConnected)
                                            return qsTr("守护进程未连接")
                                        if (!DaemonState.currentPath)
                                            return qsTr("暂无壁纸")
                                        if (previewImage.status === Image.Error)
                                            return qsTr("预览加载失败")
                                        return qsTr("加载中...")
                                    }
                                    font.pixelSize: App.Theme.fontSizeSmall
                                    color: App.Theme.textSecondary
                                }
                            }

                            // 模式标签（左上角）
                            Rectangle {
                                anchors.top: parent.top
                                anchors.left: parent.left
                                anchors.margins: App.Theme.spacingSmall
                                width: modeLabel.width + App.Theme.spacingSmall * 2
                                height: modeLabel.height + App.Theme.spacingTiny * 2
                                radius: App.Theme.radiusSmall
                                color: DaemonState.mode === "Video"
                                       ? Qt.rgba(App.Theme.accent.r, App.Theme.accent.g, App.Theme.accent.b, 0.85)
                                       : Qt.rgba(App.Theme.primary.r, App.Theme.primary.g, App.Theme.primary.b, 0.85)

                                Text {
                                    id: modeLabel
                                    anchors.centerIn: parent
                                    text: DaemonState.mode === "Video" ? "🎬 Video" : "🖼️ Image"
                                    font.pixelSize: App.Theme.fontSizeSmall
                                    font.bold: true
                                    color: App.Theme.textOnAccent
                                }
                            }
                        }

                        // 文件名
                        Text {
                            Layout.fillWidth: true
                            text: DaemonState.currentFilename || qsTr("未知")
                            font.pixelSize: App.Theme.fontSizeLarge
                            font.bold: true
                            color: App.Theme.text
                            elide: Text.ElideMiddle
                        }

                        // 路径
                        Text {
                            Layout.fillWidth: true
                            text: DaemonState.currentPath || "-"
                            font.pixelSize: App.Theme.fontSizeSmall
                            color: App.Theme.textSecondary
                            elide: Text.ElideMiddle
                        }
                    }
                }

                // ============================================================
                // 快捷操作按钮
                // ============================================================
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: actionsRow.height + App.Theme.spacingMedium * 2
                    radius: App.Theme.radiusLarge
                    color: App.Theme.card
                    border.width: 1
                    border.color: App.Theme.border

                    RowLayout {
                        id: actionsRow
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.margins: App.Theme.spacingMedium
                        spacing: App.Theme.spacingSmall

                        ActionButton {
                            icon: "⏮️"
                            label: qsTr("上一张")
                            onClicked: LianwallApp.daemonPrev()
                        }

                        ActionButton {
                            icon: "⏭️"
                            label: qsTr("下一张")
                            highlighted: true
                            onClicked: LianwallApp.daemonNext()
                        }

                        ActionButton {
                            icon: DaemonState.mode === "Video" ? "🖼️" : "🎬"
                            label: DaemonState.mode === "Video" ? qsTr("切到图片") : qsTr("切到视频")
                            onClicked: {
                                var target = DaemonState.mode === "Video" ? "Image" : "Video"
                                console.log("[Dashboard] Switch mode:", DaemonState.mode, "->", target)
                                LianwallApp.daemonSetMode(target)
                            }
                        }
                    }
                }

                // ============================================================                // 倒计时进度条
                // ============================================================
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: countdownContent.height + App.Theme.spacingMedium * 2
                    radius: App.Theme.radiusLarge
                    color: App.Theme.card
                    border.width: 1
                    border.color: App.Theme.border
                    visible: DaemonState.daemonConnected

                    Components.CountdownBar {
                        id: countdownContent
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: App.Theme.spacingMedium
                    }
                }

                // ============================================================                // 状态信息网格
                // ============================================================
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: statsGrid.height + App.Theme.spacingMedium * 2
                    radius: App.Theme.radiusLarge
                    color: App.Theme.card
                    border.width: 1
                    border.color: App.Theme.border

                    GridLayout {
                        id: statsGrid
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: App.Theme.spacingMedium
                        columns: 3
                        rowSpacing: App.Theme.spacingMedium
                        columnSpacing: App.Theme.spacingMedium

                        StatItem { label: qsTr("引擎");   value: DaemonState.engine || "none" }
                        StatItem { label: qsTr("总壁纸");  value: DaemonState.totalWallpapers }
                        StatItem { label: qsTr("可用");    value: DaemonState.availableCount }
                        StatItem { label: qsTr("已锁定");  value: DaemonState.lockedCount }
                        StatItem { label: qsTr("已扫描");  value: DaemonState.scannedCount }
                        StatItem {
                            label: qsTr("时间点")
                            value: DaemonState.timePointsCount > 0
                                   ? DaemonState.timePointsCount + " (" + (DaemonState.nextTimePoint || "-") + ")"
                                   : "0"
                        }
                    }
                }

                // ============================================================
                // 显存状态（仅在有显存数据时显示）
                // ============================================================
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: vramCol.height + App.Theme.spacingMedium * 2
                    radius: App.Theme.radiusLarge
                    color: App.Theme.card
                    border.width: 1
                    border.color: DaemonState.vramDegraded ? App.Theme.warning : App.Theme.border
                    visible: DaemonState.vramTotalMb > 0 && ConfigManager.vramEnabled

                    ColumnLayout {
                        id: vramCol
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: App.Theme.spacingMedium
                        spacing: App.Theme.spacingSmall

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: App.Theme.spacingSmall

                            Text {
                                text: DaemonState.vramDegraded ? "⚠️ " + qsTr("显存降级中") : "🎮 " + qsTr("显存")
                                font.pixelSize: App.Theme.fontSizeMedium
                                font.bold: true
                                color: DaemonState.vramDegraded ? App.Theme.warning : App.Theme.text
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: DaemonState.vramUsedMb + " / " + DaemonState.vramTotalMb + " MB"
                                font.pixelSize: App.Theme.fontSizeSmall
                                font.family: "monospace"
                                color: App.Theme.textSecondary
                            }
                        }

                        // 显存进度条
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 6
                            radius: 3
                            color: App.Theme.border

                            Rectangle {
                                height: parent.height
                                radius: parent.radius
                                width: DaemonState.vramTotalMb > 0
                                       ? parent.width * (DaemonState.vramUsedMb / DaemonState.vramTotalMb)
                                       : 0
                                color: {
                                    var usage = DaemonState.vramUsedMb / Math.max(1, DaemonState.vramTotalMb)
                                    if (usage > 0.85) return App.Theme.error
                                    if (usage > 0.7)  return App.Theme.warning
                                    return App.Theme.success
                                }

                                Behavior on width {
                                    NumberAnimation { duration: 500; easing.type: Easing.OutQuad }
                                }
                            }
                        }
                    }
                }

                // 底部间距
                Item { Layout.preferredHeight: App.Theme.spacingMedium }
            }
        }
    }

    // 壁纸/模式变化时刷新预览
    Connections {
        target: DaemonState
        function onCurrentPathChanged() {
            console.log("[Dashboard] currentPath changed:", DaemonState.currentPath)
            dashRoot.refreshCounter++
        }
        function onModeChanged() {
            console.log("[Dashboard] mode changed:", DaemonState.mode)
            dashRoot.refreshCounter++
        }
    }

    // ========================================================================
    // 内联组件
    // ========================================================================

    /// 操作按钮
    component ActionButton: Item {
        property string icon: ""
        property string label: ""
        property bool highlighted: false
        signal clicked()

        Layout.fillWidth: true
        implicitHeight: 56

        Rectangle {
            anchors.fill: parent
            radius: App.Theme.radiusMedium
            color: actionMouse.pressed
                   ? (highlighted ? App.Theme.accentPressed : App.Theme.cardHover)
                   : actionMouse.containsMouse
                     ? (highlighted ? App.Theme.accentHover : App.Theme.cardHover)
                     : highlighted ? App.Theme.accent : "transparent"
            border.width: highlighted ? 0 : 1
            border.color: App.Theme.border

            Behavior on color {
                ColorAnimation { duration: 150 }
            }

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 2

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: icon
                    font.pixelSize: 20
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: label
                    font.pixelSize: App.Theme.fontSizeSmall
                    color: highlighted ? App.Theme.textOnAccent : App.Theme.text
                }
            }

            MouseArea {
                id: actionMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: parent.parent.clicked()
            }
        }
    }

    /// 统计项
    component StatItem: Item {
        property string label: ""
        property var value: ""
        Layout.fillWidth: true
        implicitHeight: statCol.height

        ColumnLayout {
            id: statCol
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 2

            Text {
                text: label
                font.pixelSize: App.Theme.fontSizeSmall
                color: App.Theme.textSecondary
            }
            Text {
                text: String(value)
                font.pixelSize: App.Theme.fontSizeLarge
                font.bold: true
                color: App.Theme.text
            }
        }
    }
}
