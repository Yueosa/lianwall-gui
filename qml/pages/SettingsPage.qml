import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".." as App
import "../components" as Components

/// Settings 设置页
/// 壁纸路径 + 界面设置
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
                // 分区 1：壁纸路径
                // ============================================================
                ConfigSection {
                    title: qsTr("📂 壁纸路径")

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: App.Theme.spacingMedium

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

                        // 配置文件目录
                        ConfigRow {
                            label: qsTr("配置目录")

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: App.Theme.spacingSmall

                                Text {
                                    Layout.fillWidth: true
                                    text: ConfigManager.configDirPath()
                                    font.pixelSize: App.Theme.fontSizeSmall
                                    font.family: "monospace"
                                    color: App.Theme.text
                                    elide: Text.ElideMiddle
                                }

                                SmallButton {
                                    text: qsTr("📂 打开")
                                    onClicked: ConfigManager.openDirectory(ConfigManager.configDirPath())
                                }
                            }
                        }

                        // 缓存目录
                        ConfigRow {
                            label: qsTr("缓存目录")

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: App.Theme.spacingSmall

                                Text {
                                    Layout.fillWidth: true
                                    text: ConfigManager.cacheDirPath()
                                    font.pixelSize: App.Theme.fontSizeSmall
                                    font.family: "monospace"
                                    color: App.Theme.text
                                    elide: Text.ElideMiddle
                                }

                                SmallButton {
                                    text: qsTr("📂 打开")
                                    onClicked: ConfigManager.openDirectory(ConfigManager.cacheDirPath())
                                }
                            }
                        }
                    }
                }

                // ============================================================
                // 分区 2：界面设置
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

                        // 主色调
                        ConfigRow {
                            label: qsTr("主色调")

                            RowLayout {
                                spacing: App.Theme.spacingSmall

                                ColorChip {
                                    chipColor: "#5BCEFA"
                                    label: qsTr("MTF蓝")
                                    selected: ConfigManager.accentColor === "blue"
                                    onClicked: ConfigManager.setAccentColor("blue")
                                }
                                ColorChip {
                                    chipColor: "#F5A9B8"
                                    label: qsTr("MTF粉")
                                    selected: ConfigManager.accentColor === "pink"
                                    onClicked: ConfigManager.setAccentColor("pink")
                                }
                            }
                        }

                        // 语言
                        ConfigRow {
                            label: qsTr("语言")

                            StyledSelect {
                                id: langSelect
                                selectWidth: 140
                                model: [
                                    { text: "中文", value: "zh_CN" },
                                    { text: "English", value: "en" }
                                ]
                                currentIndex: ConfigManager.language === "en" ? 1 : 0
                                onSelected: function(index) {
                                    ConfigManager.setLanguage(model[index].value)
                                    LianwallApp.switchLanguage(model[index].value)
                                }
                            }
                        }
                    }
                }

                // ============================================================
                // 分区 3：运行模式
                // ============================================================
                ConfigSection {
                    title: qsTr("🔀 运行模式")

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: App.Theme.spacingMedium

                        ConfigRow {
                            label: qsTr("默认模式")
                            hint: qsTr("守护进程启动时使用")

                            RowLayout {
                                spacing: App.Theme.spacingSmall

                                ModeChip {
                                    text: "🎬 Video"
                                    selected: ConfigManager.mode === "Video"
                                    onClicked: {
                                        if (DaemonState.daemonConnected)
                                            ConfigManager.setMode("Video")
                                    }
                                }
                                ModeChip {
                                    text: "🖼️ Image"
                                    selected: ConfigManager.mode === "Image"
                                    onClicked: {
                                        if (DaemonState.daemonConnected)
                                            ConfigManager.setMode("Image")
                                    }
                                }
                            }
                        }
                    }
                }

                // ============================================================
                // 分区 4：动态壁纸引擎
                // ============================================================
                ConfigSection {
                    title: qsTr("🎬 动态壁纸引擎")

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: App.Theme.spacingMedium

                        ConfigRow {
                            label: qsTr("切换间隔")
                            hint: qsTr("10 ~ 86400 (最大24h)")

                            NumberInput {
                                value: ConfigManager.videoInterval
                                minValue: 10
                                maxValue: 86400
                                suffix: qsTr(" 秒")
                                enabled: DaemonState.daemonConnected
                                onValueEdited: function(v) { ConfigManager.setVideoInterval(v) }
                            }
                        }

                        ConfigRow {
                            label: qsTr("显示器")
                            hint: qsTr("* = 全部")

                            StyledTextInput {
                                inputText: ConfigManager.videoDisplay
                                placeholder: "*"
                                enabled: DaemonState.daemonConnected
                                onTextCommitted: function(t) { ConfigManager.setVideoDisplay(t) }
                            }
                        }

                        ConfigRow {
                            label: qsTr("mpvpaper 参数")

                            Components.TagEditor {
                                Layout.fillWidth: true
                                tags: ConfigManager.mpvpaperArgs
                                onTagsEdited: function(newTags) {
                                    if (DaemonState.daemonConnected)
                                        ConfigManager.setMpvpaperArgs(newTags)
                                }
                            }
                        }

                        ConfigRow {
                            label: qsTr("mpv 参数")

                            Components.TagEditor {
                                Layout.fillWidth: true
                                tags: ConfigManager.mpvArgs
                                onTagsEdited: function(newTags) {
                                    if (DaemonState.daemonConnected)
                                        ConfigManager.setMpvArgs(newTags)
                                }
                            }
                        }
                    }
                }

                // ============================================================
                // 分区 5：静态壁纸引擎
                // ============================================================
                ConfigSection {
                    title: qsTr("🖼️ 静态壁纸引擎")

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: App.Theme.spacingMedium

                        ConfigRow {
                            label: qsTr("切换间隔")
                            hint: qsTr("10 ~ 86400 (最大24h)")

                            NumberInput {
                                value: ConfigManager.imageInterval
                                minValue: 10
                                maxValue: 86400
                                suffix: qsTr(" 秒")
                                enabled: DaemonState.daemonConnected
                                onValueEdited: function(v) { ConfigManager.setImageInterval(v) }
                            }
                        }

                        ConfigRow {
                            label: qsTr("显示器")
                            hint: qsTr("空 = 全部")

                            StyledTextInput {
                                inputText: ConfigManager.imageOutputs
                                placeholder: ""
                                enabled: DaemonState.daemonConnected
                                onTextCommitted: function(t) { ConfigManager.setImageOutputs(t) }
                            }
                        }

                        ConfigRow {
                            label: qsTr("swww 参数")

                            Components.TagEditor {
                                Layout.fillWidth: true
                                tags: ConfigManager.swwwArgs
                                onTagsEdited: function(newTags) {
                                    if (DaemonState.daemonConnected)
                                        ConfigManager.setSwwwArgs(newTags)
                                }
                            }
                        }
                    }
                }

                // ============================================================
                // 分区 6：显存监控
                // ============================================================
                ConfigSection {
                    title: qsTr("🎮 显存监控")

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: App.Theme.spacingMedium

                        ConfigRow {
                            label: qsTr("启用监控")

                            Switch {
                                checked: ConfigManager.vramEnabled
                                enabled: DaemonState.daemonConnected
                                onToggled: ConfigManager.setVramEnabled(checked)
                            }
                        }

                        ConfigRow {
                            label: qsTr("后端")
                            hint: qsTr("auto=自动检测, custom=自定义命令")

                            StyledSelect {
                                selectWidth: 140
                                model: [
                                    { text: qsTr("Auto（自动检测）"), value: "auto" },
                                    { text: qsTr("Custom（自定义）"), value: "custom" }
                                ]
                                currentIndex: ConfigManager.vramBackend === "custom" ? 1 : 0
                                enabled: DaemonState.daemonConnected && ConfigManager.vramEnabled
                                onSelected: function(idx) {
                                    console.log("Selected backend:", model[idx].value);
                                    ConfigManager.setVramBackend(model[idx].value)
                                }
                            }
                        }

                        ConfigRow {
                            label: qsTr("自定义命令")
                            hint: qsTr("输出需含 used_mb=N 和 total_mb=N")
                            visible: ConfigManager.vramBackend === "custom"

                            StyledTextInput {
                                inputText: ConfigManager.vramCustomCommand
                                placeholder: "~/.config/lianwall/intel_vram.sh"
                                enabled: DaemonState.daemonConnected && ConfigManager.vramEnabled
                                onTextCommitted: function(t) { ConfigManager.setVramCustomCommand(t) }
                            }
                        }

                        ConfigRow {
                            label: qsTr("降级阈值")
                            hint: "5% ~ 50%"

                            StyledSlider {
                                Layout.fillWidth: true
                                value: ConfigManager.vramThresholdPercent
                                minValue: 5.0
                                maxValue: 50.0
                                suffix: "%"
                                enabled: DaemonState.daemonConnected && ConfigManager.vramEnabled
                                onValueEdited: function(v) { ConfigManager.setVramThresholdPercent(v) }
                            }
                        }

                        ConfigRow {
                            label: qsTr("恢复阈值")
                            hint: "20% ~ 80%"

                            StyledSlider {
                                Layout.fillWidth: true
                                value: ConfigManager.vramRecoveryPercent
                                minValue: 20.0
                                maxValue: 80.0
                                suffix: "%"
                                enabled: DaemonState.daemonConnected && ConfigManager.vramEnabled
                                onValueEdited: function(v) { ConfigManager.setVramRecoveryPercent(v) }
                            }
                        }

                        ConfigRow {
                            label: qsTr("检测间隔")
                            hint: "1 ~ 60"

                            NumberInput {
                                value: ConfigManager.vramCheckInterval
                                minValue: 1
                                maxValue: 60
                                suffix: qsTr(" 秒")
                                enabled: DaemonState.daemonConnected && ConfigManager.vramEnabled
                                onValueEdited: function(v) { ConfigManager.setVramCheckInterval(v) }
                            }
                        }

                        ConfigRow {
                            label: qsTr("冷却时间")
                            hint: "10 ~ 600"

                            NumberInput {
                                value: ConfigManager.vramCooldownSeconds
                                minValue: 10
                                maxValue: 600
                                suffix: qsTr(" 秒")
                                enabled: DaemonState.daemonConnected && ConfigManager.vramEnabled
                                onValueEdited: function(v) { ConfigManager.setVramCooldownSeconds(v) }
                            }
                        }
                    }
                }

                // ============================================================
                // 分区 7：守护进程
                // ============================================================
                ConfigSection {
                    title: qsTr("🔧 守护进程")

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: App.Theme.spacingMedium

                        ConfigRow {
                            label: qsTr("日志级别")

                            StyledSelect {
                                selectWidth: 140
                                model: [
                                    { text: "Error", value: "error" },
                                    { text: "Warn", value: "warn" },
                                    { text: "Info", value: "info" },
                                    { text: "Debug", value: "debug" },
                                    { text: "Trace", value: "trace" }
                                ]
                                currentIndex: {
                                    var lvl = ConfigManager.logLevel
                                    if (lvl === "error") return 0
                                    if (lvl === "warn") return 1
                                    if (lvl === "info") return 2
                                    if (lvl === "debug") return 3
                                    if (lvl === "trace") return 4
                                    return 2
                                }
                                onSelected: function(idx) {
                                    if (DaemonState.daemonConnected)
                                        ConfigManager.setLogLevel(model[idx].value)
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

    /// 色彩选择标签（带色点）
    component ColorChip: Rectangle {
        property color chipColor: "#5BCEFA"
        property string label: ""
        property bool selected: false
        signal clicked()

        width: colorChipRow.width + App.Theme.spacingSmall * 2
        height: 32
        radius: App.Theme.radiusMedium
        color: selected ? Qt.rgba(chipColor.r, chipColor.g, chipColor.b, 0.25)
               : colorChipMouse.containsMouse ? App.Theme.cardHover : "transparent"
        border.width: selected ? 2 : 1
        border.color: selected ? chipColor : App.Theme.border

        Behavior on color { ColorAnimation { duration: 150 } }
        Behavior on border.color { ColorAnimation { duration: 150 } }

        Row {
            id: colorChipRow
            anchors.centerIn: parent
            spacing: 6

            Rectangle {
                width: 14; height: 14
                radius: 7
                color: chipColor
                anchors.verticalCenter: parent.verticalCenter
                border.width: selected ? 2 : 0
                border.color: Qt.darker(chipColor, 1.3)
            }

            Text {
                text: label
                font.pixelSize: App.Theme.fontSizeSmall
                color: selected ? chipColor : App.Theme.text
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        MouseArea {
            id: colorChipMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
        }
    }

    /// 自定义下拉选择器（替代默认 ComboBox）
    component StyledSelect: Item {
        id: selectRoot
        property int selectWidth: 140
        property var model: []
        property int currentIndex: 0
        signal selected(int index)

        width: selectWidth
        height: 32

        // 当前选中项按钮
        Rectangle {
            id: selectButton
            anchors.fill: parent
            radius: App.Theme.radiusMedium
            color: selectMouse.pressed ? App.Theme.cardHover
                   : selectMouse.containsMouse ? App.Theme.surface : App.Theme.input
            border.width: 1
            border.color: selectPopup.visible ? App.Theme.accent : App.Theme.border

            Behavior on border.color { ColorAnimation { duration: 150 } }

            Row {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 8
                spacing: 4

                Text {
                    width: parent.width - dropdownArrow.width - 4
                    anchors.verticalCenter: parent.verticalCenter
                    text: selectRoot.model[selectRoot.currentIndex]?.text ?? ""
                    font.pixelSize: App.Theme.fontSizeSmall
                    color: App.Theme.text
                    elide: Text.ElideRight
                }

                Text {
                    id: dropdownArrow
                    anchors.verticalCenter: parent.verticalCenter
                    text: selectPopup.visible ? "▲" : "▼"
                    font.pixelSize: 8
                    color: App.Theme.textSecondary
                }
            }

            MouseArea {
                id: selectMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: selectPopup.visible = !selectPopup.visible
            }
        }

        // 下拉面板（使用 Popup 自动处理点击外部关闭）
        Popup {
            id: selectPopup
            y: selectButton.height + 4
            width: selectRoot.selectWidth
            padding: 4
            closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

            background: Rectangle {
                radius: App.Theme.radiusMedium
                color: App.Theme.popup
                border.width: 1
                border.color: App.Theme.border
            }

            contentItem: Column {
                id: dropdownCol
                spacing: 0

                Repeater {
                    model: selectRoot.model

                    Rectangle {
                        width: selectRoot.selectWidth - 8
                        height: 32
                        radius: App.Theme.radiusSmall
                        color: index === selectRoot.currentIndex ? App.Theme.accent
                               : optMouse.containsMouse ? App.Theme.cardHover : "transparent"

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 8
                            text: modelData.text
                            font.pixelSize: App.Theme.fontSizeSmall
                            color: index === selectRoot.currentIndex ? App.Theme.textOnAccent
                                   : App.Theme.text
                        }

                        MouseArea {
                            id: optMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                selectRoot.currentIndex = index
                                selectRoot.selected(index)
                                selectPopup.close()
                            }
                        }
                    }
                }
            }
        }
    }

    /// 数字输入框（带验证和范围约束）
    component NumberInput: Item {
        id: numInputRoot
        property int value: 0
        property int minValue: 0
        property int maxValue: 99999
        property string suffix: ""
        signal valueEdited(int newValue)

        implicitWidth: 160
        implicitHeight: 32

        Rectangle {
            anchors.fill: parent
            radius: App.Theme.radiusSmall
            color: numInputRoot.enabled ? App.Theme.input : App.Theme.surface
            border.width: 1
            border.color: numField.activeFocus ? App.Theme.accent : App.Theme.border
            opacity: numInputRoot.enabled ? 1.0 : 0.5

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: App.Theme.spacingSmall
                anchors.rightMargin: App.Theme.spacingSmall
                spacing: 2

                TextInput {
                    id: numField
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: App.Theme.fontSizeMedium
                    font.family: "monospace"
                    color: App.Theme.text
                    text: numInputRoot.value.toString()
                    validator: IntValidator { bottom: numInputRoot.minValue; top: numInputRoot.maxValue }
                    selectByMouse: true
                    clip: true
                    enabled: numInputRoot.enabled

                    onEditingFinished: {
                        var v = parseInt(text)
                        if (!isNaN(v)) {
                            v = Math.max(numInputRoot.minValue, Math.min(numInputRoot.maxValue, v))
                            numInputRoot.valueEdited(v)
                        }
                    }
                }

                Text {
                    text: numInputRoot.suffix
                    font.pixelSize: App.Theme.fontSizeSmall
                    color: App.Theme.textSecondary
                    visible: numInputRoot.suffix.length > 0
                }
            }
        }

        onValueChanged: {
            if (!numField.activeFocus)
                numField.text = value.toString()
        }
    }

    /// 文本输入框（Enter / 失焦提交）
    component StyledTextInput: Item {
        id: textInputRoot
        property string inputText: ""
        property string placeholder: ""
        signal textCommitted(string newText)

        implicitWidth: 200
        implicitHeight: 32

        Rectangle {
            anchors.fill: parent
            radius: App.Theme.radiusSmall
            color: textInputRoot.enabled ? App.Theme.input : App.Theme.surface
            border.width: 1
            border.color: textField.activeFocus ? App.Theme.accent : App.Theme.border
            opacity: textInputRoot.enabled ? 1.0 : 0.5

            TextInput {
                id: textField
                anchors.fill: parent
                anchors.margins: App.Theme.spacingSmall
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: App.Theme.fontSizeMedium
                color: App.Theme.text
                text: textInputRoot.inputText
                selectByMouse: true
                clip: true
                enabled: textInputRoot.enabled

                onEditingFinished: {
                    textInputRoot.textCommitted(text)
                }

                Text {
                    anchors.fill: parent
                    verticalAlignment: Text.AlignVCenter
                    text: textInputRoot.placeholder
                    font.pixelSize: App.Theme.fontSizeMedium
                    color: App.Theme.textSecondary
                    visible: !textField.text && !textField.activeFocus
                }
            }
        }

        onInputTextChanged: {
            if (!textField.activeFocus)
                textField.text = inputText
        }
    }

    /// 滑块（带数值显示）
    component StyledSlider: RowLayout {
        id: sliderRoot
        property real value: 0
        property real minValue: 0
        property real maxValue: 100
        property real stepSize: 0.5
        property string suffix: "%"
        signal valueEdited(real newValue)

        spacing: App.Theme.spacingSmall

        Slider {
            id: sliderCtrl
            Layout.fillWidth: true
            from: sliderRoot.minValue
            to: sliderRoot.maxValue
            stepSize: sliderRoot.stepSize
            value: sliderRoot.value
            enabled: sliderRoot.enabled
            onMoved: sliderRoot.valueEdited(value)
        }

        Text {
            text: sliderCtrl.value.toFixed(1) + sliderRoot.suffix
            font.pixelSize: App.Theme.fontSizeSmall
            font.family: "monospace"
            color: App.Theme.text
            Layout.preferredWidth: 54
            horizontalAlignment: Text.AlignRight
        }
    }
}
