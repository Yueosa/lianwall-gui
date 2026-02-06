import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".." as App
import "../components" as Components

/// Settings 设置页
/// 4 个配置分区 + Systemd 服务路径
Item {
    id: settingsRoot

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

                // 页面标题
                Text {
                    text: qsTr("⚙️ 设置")
                    font.pixelSize: 24
                    font.bold: true
                    color: App.Theme.text
                }

                // 未连接提示
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    radius: App.Theme.radiusMedium
                    color: Qt.rgba(App.Theme.warning.r, App.Theme.warning.g, App.Theme.warning.b, 0.15)
                    border.width: 1
                    border.color: App.Theme.warning
                    visible: !DaemonState.daemonConnected

                    Text {
                        anchors.centerIn: parent
                        text: qsTr("⚠️ 守护进程未连接，配置修改暂不可用")
                        font.pixelSize: App.Theme.fontSizeMedium
                        color: App.Theme.warning
                    }
                }

                // 未加载提示
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    radius: App.Theme.radiusMedium
                    color: App.Theme.surface
                    visible: DaemonState.daemonConnected && !ConfigManager.configLoaded

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: App.Theme.spacingSmall
                        BusyIndicator { width: 20; height: 20; running: true }
                        Text {
                            text: qsTr("正在加载配置...")
                            font.pixelSize: App.Theme.fontSizeMedium
                            color: App.Theme.textSecondary
                        }
                    }
                }

                // ============================================================
                // 分区 1：路径与模式
                // ============================================================
                ConfigSection {
                    title: qsTr("📂 路径与模式")

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: App.Theme.spacingMedium

                        // 运行模式
                        ConfigRow {
                            label: qsTr("运行模式")

                            RowLayout {
                                spacing: App.Theme.spacingSmall

                                ModeChip {
                                    text: "🎬 Video"
                                    selected: ConfigManager.mode === "Video"
                                    onClicked: ConfigManager.setMode("Video")
                                }
                                ModeChip {
                                    text: "🖼️ Image"
                                    selected: ConfigManager.mode === "Image"
                                    onClicked: ConfigManager.setMode("Image")
                                }
                            }
                        }

                        // 视频壁纸目录
                        ConfigRow {
                            label: qsTr("视频目录")

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: App.Theme.spacingSmall

                                Text {
                                    Layout.fillWidth: true
                                    text: ConfigManager.videoDir || "-"
                                    font.pixelSize: App.Theme.fontSizeSmall
                                    font.family: "monospace"
                                    color: App.Theme.text
                                    elide: Text.ElideMiddle
                                }

                                SmallButton {
                                    text: qsTr("📂 打开")
                                    onClicked: ConfigManager.openDirectory(ConfigManager.videoDir)
                                }
                            }
                        }

                        // 图片壁纸目录
                        ConfigRow {
                            label: qsTr("图片目录")

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: App.Theme.spacingSmall

                                Text {
                                    Layout.fillWidth: true
                                    text: ConfigManager.imageDir || "-"
                                    font.pixelSize: App.Theme.fontSizeSmall
                                    font.family: "monospace"
                                    color: App.Theme.text
                                    elide: Text.ElideMiddle
                                }

                                SmallButton {
                                    text: qsTr("📂 打开")
                                    onClicked: ConfigManager.openDirectory(ConfigManager.imageDir)
                                }
                            }
                        }
                    }
                }

                // ============================================================
                // 分区 2：动态壁纸引擎
                // ============================================================
                ConfigSection {
                    title: qsTr("🎬 动态壁纸引擎 (mpvpaper)")

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: App.Theme.spacingMedium

                        // 切换间隔
                        ConfigRow {
                            label: qsTr("切换间隔 (秒)")
                            hint: "10 – 86400"

                            SpinBox {
                                from: 10
                                to: 86400
                                stepSize: 10
                                value: ConfigManager.videoInterval
                                editable: true
                                onValueModified: ConfigManager.setVideoInterval(value)

                                background: Rectangle {
                                    radius: App.Theme.radiusSmall
                                    color: App.Theme.surface
                                    border.width: 1
                                    border.color: App.Theme.border
                                }
                            }
                        }

                        // 目标显示器
                        ConfigRow {
                            label: qsTr("目标显示器")
                            hint: qsTr('"*" 表示所有显示器')

                            ConfigInput {
                                text: ConfigManager.videoDisplay
                                onEditingFinished: ConfigManager.setVideoDisplay(text)
                            }
                        }

                        // mpvpaper 参数
                        Components.TagEditor {
                            Layout.fillWidth: true
                            label: qsTr("mpvpaper 参数")
                            tags: ConfigManager.mpvpaperArgs
                            onTagsEdited: function(newTags) {
                                ConfigManager.setMpvpaperArgs(newTags)
                            }
                        }

                        // mpv 参数
                        Components.TagEditor {
                            Layout.fillWidth: true
                            label: qsTr("mpv 参数")
                            tags: ConfigManager.mpvArgs
                            onTagsEdited: function(newTags) {
                                ConfigManager.setMpvArgs(newTags)
                            }
                        }
                    }
                }

                // ============================================================
                // 分区 3：静态壁纸引擎
                // ============================================================
                ConfigSection {
                    title: qsTr("🖼️ 静态壁纸引擎 (swww)")

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: App.Theme.spacingMedium

                        // 切换间隔
                        ConfigRow {
                            label: qsTr("切换间隔 (秒)")
                            hint: "10 – 86400"

                            SpinBox {
                                from: 10
                                to: 86400
                                stepSize: 10
                                value: ConfigManager.imageInterval
                                editable: true
                                onValueModified: ConfigManager.setImageInterval(value)

                                background: Rectangle {
                                    radius: App.Theme.radiusSmall
                                    color: App.Theme.surface
                                    border.width: 1
                                    border.color: App.Theme.border
                                }
                            }
                        }

                        // 输出目标
                        ConfigRow {
                            label: qsTr("输出目标 (outputs)")
                            hint: qsTr('留空或逗号分隔')

                            ConfigInput {
                                text: ConfigManager.imageOutputs
                                onEditingFinished: ConfigManager.setImageOutputs(text)
                            }
                        }

                        // swww 参数
                        Components.TagEditor {
                            Layout.fillWidth: true
                            label: qsTr("swww 参数")
                            tags: ConfigManager.swwwArgs
                            onTagsEdited: function(newTags) {
                                ConfigManager.setSwwwArgs(newTags)
                            }
                        }
                    }
                }

                // ============================================================
                // 分区 4：显存监控
                // ============================================================
                ConfigSection {
                    title: qsTr("🎮 显存监控 (VRAM)")

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: App.Theme.spacingMedium

                        // 启用开关
                        ConfigRow {
                            label: qsTr("启用监控")

                            Switch {
                                checked: ConfigManager.vramEnabled
                                onToggled: ConfigManager.setVramEnabled(checked)
                            }
                        }

                        // 降级阈值
                        ConfigRow {
                            label: qsTr("降级阈值 (%)")
                            hint: "5.0 – 50.0"
                            visible: ConfigManager.vramEnabled

                            RowLayout {
                                spacing: App.Theme.spacingSmall

                                Slider {
                                    Layout.fillWidth: true
                                    from: 5.0
                                    to: 50.0
                                    stepSize: 0.5
                                    value: ConfigManager.vramThresholdPercent
                                    onMoved: ConfigManager.setVramThresholdPercent(value)
                                }

                                Text {
                                    Layout.preferredWidth: 40
                                    text: ConfigManager.vramThresholdPercent.toFixed(1)
                                    font.pixelSize: App.Theme.fontSizeSmall
                                    font.family: "monospace"
                                    color: App.Theme.text
                                    horizontalAlignment: Text.AlignRight
                                }
                            }
                        }

                        // 恢复阈值
                        ConfigRow {
                            label: qsTr("恢复阈值 (%)")
                            hint: "20.0 – 80.0"
                            visible: ConfigManager.vramEnabled

                            RowLayout {
                                spacing: App.Theme.spacingSmall

                                Slider {
                                    Layout.fillWidth: true
                                    from: 20.0
                                    to: 80.0
                                    stepSize: 0.5
                                    value: ConfigManager.vramRecoveryPercent
                                    onMoved: ConfigManager.setVramRecoveryPercent(value)
                                }

                                Text {
                                    Layout.preferredWidth: 40
                                    text: ConfigManager.vramRecoveryPercent.toFixed(1)
                                    font.pixelSize: App.Theme.fontSizeSmall
                                    font.family: "monospace"
                                    color: App.Theme.text
                                    horizontalAlignment: Text.AlignRight
                                }
                            }
                        }

                        // 检测间隔
                        ConfigRow {
                            label: qsTr("检测间隔 (秒)")
                            hint: "1 – 60"
                            visible: ConfigManager.vramEnabled

                            SpinBox {
                                from: 1
                                to: 60
                                value: ConfigManager.vramCheckInterval
                                editable: true
                                onValueModified: ConfigManager.setVramCheckInterval(value)

                                background: Rectangle {
                                    radius: App.Theme.radiusSmall
                                    color: App.Theme.surface
                                    border.width: 1
                                    border.color: App.Theme.border
                                }
                            }
                        }

                        // 冷却时间
                        ConfigRow {
                            label: qsTr("冷却时间 (秒)")
                            hint: "10 – 600"
                            visible: ConfigManager.vramEnabled

                            SpinBox {
                                from: 10
                                to: 600
                                stepSize: 10
                                value: ConfigManager.vramCooldownSeconds
                                editable: true
                                onValueModified: ConfigManager.setVramCooldownSeconds(value)

                                background: Rectangle {
                                    radius: App.Theme.radiusSmall
                                    color: App.Theme.surface
                                    border.width: 1
                                    border.color: App.Theme.border
                                }
                            }
                        }
                    }
                }

                // ============================================================
                // 分区 5：守护进程
                // ============================================================
                ConfigSection {
                    title: qsTr("🔧 守护进程")

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: App.Theme.spacingMedium

                        // 日志级别
                        ConfigRow {
                            label: qsTr("日志级别")

                            ComboBox {
                                model: ["error", "warn", "info", "debug", "trace"]
                                currentIndex: model.indexOf(ConfigManager.logLevel)
                                onActivated: ConfigManager.setLogLevel(model[currentIndex])

                                background: Rectangle {
                                    radius: App.Theme.radiusSmall
                                    color: App.Theme.surface
                                    border.width: 1
                                    border.color: App.Theme.border
                                    implicitWidth: 140
                                    implicitHeight: 32
                                }
                            }
                        }

                        // Systemd 服务路径
                        ConfigRow {
                            label: qsTr("Systemd 服务")

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: App.Theme.spacingTiny

                                Text {
                                    Layout.fillWidth: true
                                    text: settingsRoot.systemdServicePath
                                    font.pixelSize: App.Theme.fontSizeSmall
                                    font.family: "monospace"
                                    color: App.Theme.textSecondary
                                    elide: Text.ElideMiddle
                                }

                                RowLayout {
                                    spacing: App.Theme.spacingSmall

                                    SmallButton {
                                        text: qsTr("📄 打开服务文件")
                                        onClicked: Qt.openUrlExternally("file://" + settingsRoot.systemdServicePath)
                                    }

                                    SmallButton {
                                        text: qsTr("📂 打开目录")
                                        onClicked: Qt.openUrlExternally("file://" + settingsRoot.systemdDirPath)
                                    }
                                }
                            }
                        }

                        // 重载配置按钮
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 36
                            radius: App.Theme.radiusMedium
                            color: reloadMouse.pressed ? App.Theme.accentPressed
                                   : reloadMouse.containsMouse ? App.Theme.accentHover
                                   : App.Theme.accent

                            Text {
                                anchors.centerIn: parent
                                text: qsTr("🔄 从文件重载 Daemon 配置")
                                font.pixelSize: App.Theme.fontSizeMedium
                                color: App.Theme.textOnAccent
                            }

                            MouseArea {
                                id: reloadMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: LianwallApp.daemonReloadConfig()
                            }
                        }
                    }
                }

                // ============================================================
                // 分区 6：GUI 设置（本地）
                // ============================================================
                ConfigSection {
                    title: qsTr("🎨 界面设置")

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: App.Theme.spacingMedium

                        // 主题
                        ConfigRow {
                            label: qsTr("主题")

                            RowLayout {
                                spacing: App.Theme.spacingSmall

                                ModeChip {
                                    text: "☀️ " + qsTr("浅色")
                                    selected: ConfigManager.theme === "lian"
                                    onClicked: ConfigManager.setTheme("lian")
                                }
                                ModeChip {
                                    text: "🌙 " + qsTr("深色")
                                    selected: ConfigManager.theme === "lian-dark"
                                    onClicked: ConfigManager.setTheme("lian-dark")
                                }
                            }
                        }

                        // 语言
                        ConfigRow {
                            label: qsTr("语言")

                            ComboBox {
                                model: [
                                    { text: "中文", value: "zh_CN" },
                                    { text: "English", value: "en" }
                                ]
                                textRole: "text"
                                valueRole: "value"
                                currentIndex: ConfigManager.language === "en" ? 1 : 0
                                onActivated: {
                                    ConfigManager.setLanguage(model[currentIndex].value)
                                    LianwallApp.switchLanguage(model[currentIndex].value)
                                }

                                background: Rectangle {
                                    radius: App.Theme.radiusSmall
                                    color: App.Theme.surface
                                    border.width: 1
                                    border.color: App.Theme.border
                                    implicitWidth: 140
                                    implicitHeight: 32
                                }
                            }
                        }
                    }
                }

                // 底部间距
                Item { Layout.preferredHeight: App.Theme.spacingLarge }
            }
        }
    }

    // ========================================================================
    // 辅助函数
    // ========================================================================

    /// Systemd 服务文件路径
    readonly property string systemdServicePath: HomeDir + "/.config/systemd/user/lianwalld.service"
    readonly property string systemdDirPath: HomeDir + "/.config/systemd/user"

    // ========================================================================
    // 内联组件
    // ========================================================================

    /// 配置分区卡片
    component ConfigSection: Rectangle {
        property string title: ""
        default property alias content: sectionContent.data

        Layout.fillWidth: true
        implicitHeight: sectionCol.height + App.Theme.spacingMedium * 2
        radius: App.Theme.radiusLarge
        color: App.Theme.card
        border.width: 1
        border.color: App.Theme.border

        ColumnLayout {
            id: sectionCol
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: App.Theme.spacingMedium
            spacing: App.Theme.spacingMedium

            Text {
                text: title
                font.pixelSize: App.Theme.fontSizeLarge
                font.bold: true
                color: App.Theme.text
            }

            // 分隔线
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: App.Theme.divider
            }

            ColumnLayout {
                id: sectionContent
                Layout.fillWidth: true
                spacing: App.Theme.spacingMedium
            }
        }
    }

    /// 配置行：左标签 + 右控件
    component ConfigRow: RowLayout {
        property string label: ""
        property string hint: ""

        Layout.fillWidth: true
        spacing: App.Theme.spacingMedium

        ColumnLayout {
            Layout.preferredWidth: 130
            spacing: 2

            Text {
                text: label
                font.pixelSize: App.Theme.fontSizeMedium
                color: App.Theme.text
            }
            Text {
                text: hint
                font.pixelSize: App.Theme.fontSizeSmall
                color: App.Theme.textSecondary
                visible: hint.length > 0
            }
        }
    }

    /// 模式选择标签
    component ModeChip: Rectangle {
        property string text: ""
        property bool selected: false
        signal clicked()

        width: chipLabel.width + App.Theme.spacingSmall * 2
        height: 32
        radius: App.Theme.radiusMedium
        color: selected ? App.Theme.accent
               : chipMouse.containsMouse ? App.Theme.cardHover : "transparent"
        border.width: selected ? 0 : 1
        border.color: App.Theme.border

        Behavior on color { ColorAnimation { duration: 150 } }

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

    /// 小按钮
    component SmallButton: Rectangle {
        property string text: ""
        signal clicked()

        width: btnLabel.width + App.Theme.spacingSmall * 2
        height: 28
        radius: App.Theme.radiusSmall
        color: btnMouse.pressed ? App.Theme.cardHover
               : btnMouse.containsMouse ? App.Theme.surface : "transparent"
        border.width: 1
        border.color: App.Theme.border

        Text {
            id: btnLabel
            anchors.centerIn: parent
            text: parent.text
            font.pixelSize: App.Theme.fontSizeSmall
            color: App.Theme.text
        }

        MouseArea {
            id: btnMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
        }
    }

    /// 配置输入框
    component ConfigInput: Rectangle {
        property alias text: input.text
        signal editingFinished()

        Layout.fillWidth: true
        Layout.preferredHeight: 32
        radius: App.Theme.radiusSmall
        color: App.Theme.surface
        border.width: 1
        border.color: input.activeFocus ? App.Theme.accent : App.Theme.border

        TextInput {
            id: input
            anchors.fill: parent
            anchors.margins: App.Theme.spacingSmall
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: App.Theme.fontSizeSmall
            font.family: "monospace"
            color: App.Theme.text
            clip: true
            onEditingFinished: parent.editingFinished()
        }
    }
}
