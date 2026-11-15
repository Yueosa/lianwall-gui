import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".." as App
import "../components" as C
Item {
    id: root
    property int section: 0
    onSectionChanged: if (section === 3 && DaemonState.daemonConnected) Frontend.refreshLogs()
    ColumnLayout {
        anchors.fill: parent; spacing: 12
        RowLayout {
            Layout.fillWidth: true
            Text { textFormat: Text.PlainText; text: qsTr("设置"); color: App.Theme.text; font.pixelSize: 18; font.weight: Font.DemiBold }
            Text { textFormat: Text.PlainText; text: qsTr("常规、外观与诊断"); color: App.Theme.textSecondary; font.pixelSize: 11 }
            Item { Layout.fillWidth: true }
            Text { textFormat: Text.PlainText; text: ConfigManager.pendingKeys.length > 0 ? qsTr("正在保存…") : ""; color: App.Theme.accent; font.pixelSize: 11 }
        }
        RowLayout {
            Layout.fillWidth: true; spacing: 4
            Repeater {
                model: [qsTr("常规"), qsTr("外观"), qsTr("显存监控"), qsTr("日志与诊断")]
                C.UiButton { required property int index; required property string modelData; text: modelData; kind: "flat"; selected: root.section === index; onClicked: root.section = index }
            }
            Item { Layout.fillWidth: true }
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: App.Theme.border }
        ScrollView {
            id: scroll
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true
            contentWidth: availableWidth
            ColumnLayout {
                width: scroll.availableWidth; spacing: 14
                C.UiCard {
                    Layout.fillWidth: true; visible: root.section === 0
                    ColumnLayout {
                        width: parent.width; spacing: 10
                        Text { textFormat: Text.PlainText; text: qsTr("轮换与壁纸"); color: App.Theme.text; font.pixelSize: 14; font.weight: Font.DemiBold; bottomPadding: 8 }
                        C.SettingRow {
                            Layout.fillWidth: true; title: qsTr("启动模式"); description: qsTr("只决定服务启动时的模式；底部按钮控制当前会话。")
                            C.ConfigChoice { settingKey: "paths.mode"; values: ["Video", "Image"]; labels: [qsTr("视频"), qsTr("图片")]; fallbackValue: "Video" }
                        }
                        C.NumberSetting { Layout.fillWidth: true; title: qsTr("视频切换间隔"); description: qsTr("Enter 保存并释放焦点，Esc 取消。0 表示持久关闭自动轮换。"); settingKey: "video_engine.interval"; defaultValue: 600; factor: 60; minValue: 10; maxValue: 86400; allowZero: true; unit: qsTr("分钟") }
                        C.NumberSetting { Layout.fillWidth: true; title: qsTr("图片切换间隔"); settingKey: "image_engine.interval"; defaultValue: 600; factor: 60; minValue: 10; maxValue: 86400; allowZero: true; unit: qsTr("分钟") }
                        C.TextSetting { Layout.fillWidth: true; title: qsTr("图片目录"); settingKey: "paths.image_dir" }
                        C.TextSetting { Layout.fillWidth: true; title: qsTr("视频目录"); settingKey: "paths.video_dir" }
                        RowLayout {
                            Layout.fillWidth: true
                            Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: qsTr("底部“暂停轮换”仅当前服务会话有效，重启服务后恢复；视频仍继续播放。"); font.pixelSize: 11; color: App.Theme.textSecondary; wrapMode: Text.Wrap }
                            C.UiButton { text: qsTr("重新扫描"); iconName: "refresh"; enabled: DaemonState.daemonConnected; onClicked: LianwallApp.daemonRescan() }
                        }
                    }
                }
                C.UiCard {
                    Layout.fillWidth: true; visible: root.section === 0
                    ColumnLayout {
                        width: parent.width; spacing: 10
                        Text { textFormat: Text.PlainText; text: qsTr("本地数据"); color: App.Theme.text; font.pixelSize: 14; font.weight: Font.DemiBold }
                        C.SettingRow { Layout.fillWidth: true; title: qsTr("配置文件"); description: ConfigManager.configDirPath(); C.UiButton { text: qsTr("打开目录"); iconName: "folder"; onClicked: ConfigManager.openDirectory(ConfigManager.configDirPath()) } }
                        C.SettingRow { Layout.fillWidth: true; title: qsTr("缩略图缓存"); description: qsTr("可重新生成；与收藏资料分开保存。"); divider: false; C.UiButton { text: qsTr("打开目录"); iconName: "folder"; onClicked: ConfigManager.openDirectory(ConfigManager.cacheDirPath()) } }
                    }
                }
                C.UiCard {
                    Layout.fillWidth: true; visible: root.section === 0
                    ColumnLayout {
                        width: parent.width; spacing: 10
                        Text { text: qsTr("轮换算法"); textFormat: Text.PlainText; color: App.Theme.text; font.pixelSize: 14; font.weight: Font.DemiBold }
                        C.NumberSetting { Layout.fillWidth: true; title: qsTr("方向偏置"); settingKey: "algorithm.bias_lambda"; defaultValue: 0.8; minValue: 0; maxValue: 4; decimals: 2 }
                        C.NumberSetting { Layout.fillWidth: true; title: qsTr("随机温度"); description: qsTr("较低数值更偏向邻近候选，较高数值更分散。"); settingKey: "algorithm.temperature"; defaultValue: 0.28; minValue: 0.01; maxValue: 5; decimals: 2 }
                    }
                }
                C.UiCard {
                    Layout.fillWidth: true; visible: root.section === 1
                    ColumnLayout {
                        width: parent.width; spacing: 10
                        Text { textFormat: Text.PlainText; text: qsTr("外观"); color: App.Theme.text; font.pixelSize: 14; font.weight: Font.DemiBold }
                        C.SettingRow {
                            Layout.fillWidth: true; title: qsTr("主题")
                            C.UiButton { text: qsTr("浅色"); selected: !App.Theme.isDark; kind: "flat"; onClicked: ConfigManager.setTheme("lian") }
                            C.UiButton { text: qsTr("深色"); selected: App.Theme.isDark; kind: "flat"; onClicked: ConfigManager.setTheme("lian-dark") }
                        }
                        C.SettingRow {
                            Layout.fillWidth: true; title: qsTr("强调色")
                            C.UiButton { text: qsTr("鸢尾"); selected: App.Theme.accentScheme === "iris"; kind: "flat"; onClicked: ConfigManager.setAccentColor("iris") }
                            C.UiButton { text: qsTr("樱粉"); selected: App.Theme.accentScheme === "pink"; kind: "flat"; onClicked: ConfigManager.setAccentColor("pink") }
                            C.UiButton { text: qsTr("天空"); selected: App.Theme.accentScheme === "blue"; kind: "flat"; onClicked: ConfigManager.setAccentColor("blue") }
                        }
                        C.SettingRow {
                            Layout.fillWidth: true; title: qsTr("语言"); divider: false
                            C.UiComboBox { model: ["简体中文", "English"]; currentIndex: ConfigManager.language === "en" ? 1 : 0; onActivated: ConfigManager.setLanguage(index ? "en" : "zh_CN"); font.pixelSize: 12 }
                        }
                    }
                }
                C.UiCard {
                    Layout.fillWidth: true; visible: root.section === 2
                    ColumnLayout {
                        width: parent.width; spacing: 10
                        Text { textFormat: Text.PlainText; text: qsTr("显存与自动降级"); color: App.Theme.text; font.pixelSize: 14; font.weight: Font.DemiBold }
                        C.SettingRow {
                            Layout.fillWidth: true; title: qsTr("显存监控"); description: qsTr("余量不足时切换到图片，恢复后再切回视频。")
                            Switch { checked: ConfigManager.vramEnabled; enabled: DaemonState.daemonConnected; onClicked: ConfigManager.setVramEnabled(checked) }
                        }
                        C.SettingRow {
                            Layout.fillWidth: true; title: qsTr("读取来源")
                            C.ConfigChoice { settingKey: "vram.backend"; values: ["auto", "custom"]; labels: [qsTr("自动检测"), qsTr("自定义脚本")]; fallbackValue: "auto" }
                        }
                        C.TextSetting { id: vramCommand; Layout.fillWidth: true; visible: ConfigManager.vramBackend === "custom"; title: qsTr("读取命令"); settingKey: "vram.custom_command" }
                        C.NumberSetting { Layout.fillWidth: true; title: qsTr("降级阈值"); description: qsTr("可用显存低于此比例时切换为图片。"); settingKey: "vram.threshold_percent"; defaultValue: 25; minValue: 5; maxValue: 50; unit: "%" }
                        C.NumberSetting { Layout.fillWidth: true; title: qsTr("恢复阈值"); description: qsTr("可用显存高于此比例时恢复视频。"); settingKey: "vram.recovery_percent"; defaultValue: 40; minValue: 20; maxValue: 80; unit: "%" }
                        C.NumberSetting { Layout.fillWidth: true; title: qsTr("检查间隔"); settingKey: "vram.check_interval"; defaultValue: 2; minValue: 1; maxValue: 60; unit: qsTr("秒") }
                        C.NumberSetting { Layout.fillWidth: true; title: qsTr("恢复冷却"); settingKey: "vram.cooldown_seconds"; defaultValue: 30; minValue: 10; maxValue: 600; unit: qsTr("秒") }
                        RowLayout {
                            Layout.fillWidth: true; spacing: 12
                            Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: qsTr("测试当前来源与输入命令，不保存草稿，也不修改监控、壁纸模式或轮换状态。"); color: App.Theme.textSecondary; font.pixelSize: 11; wrapMode: Text.Wrap }
                            C.UiButton { text: qsTr("测试读取"); iconName: "bolt"; kind: "tonal"; enabled: DaemonState.daemonConnected && Frontend.pending.indexOf("TestVram") < 0 && ConfigManager.pendingKeys.indexOf("vram.backend") < 0; onClicked: Frontend.testVram(ConfigManager.vramBackend, vramCommand.draftValue) }
                        }
                        BusyIndicator { visible: running; running: Frontend.pending.indexOf("TestVram") >= 0; implicitWidth: 28; implicitHeight: 28 }
                        Text {
                            Layout.fillWidth: true; visible: Object.keys(Frontend.vramTest).length > 0
                            text: Frontend.vramTest.success ? qsTr("%1 · 已用 %2 / %3 MB · 可用 %4% · %5 ms").arg(Frontend.vramTest.backend).arg(Frontend.vramTest.used_mb).arg(Frontend.vramTest.total_mb).arg(Number(Frontend.vramTest.free_percent).toFixed(1)).arg(Frontend.vramTest.elapsed_ms) : qsTr("测试失败：%1").arg(Frontend.vramTest.error || "")
                            textFormat: Text.PlainText; color: Frontend.vramTest.success ? App.Theme.success : App.Theme.error; font.pixelSize: 12; wrapMode: Text.Wrap
                        }
                        TextArea {
                            Layout.fillWidth: true; visible: !!Frontend.vramTest.stdout || !!Frontend.vramTest.stderr
                            text: (Frontend.vramTest.stdout || "") + (Frontend.vramTest.stderr ? "\nSTDERR:\n" + Frontend.vramTest.stderr : "")
                            readOnly: true; color: App.Theme.text; font.pixelSize: 11; font.family: "monospace"; selectByMouse: true; wrapMode: TextEdit.WrapAnywhere
                            background: Rectangle { color: App.Theme.input; radius: 8 }
                        }
                    }
                }
                C.UiCard {
                    Layout.fillWidth: true; visible: root.section === 3
                    ColumnLayout {
                        width: parent.width; spacing: 10
                        Text { textFormat: Text.PlainText; text: qsTr("日志与诊断"); color: App.Theme.text; font.pixelSize: 14; font.weight: Font.DemiBold }
                        C.SettingRow {
                            Layout.fillWidth: true; title: qsTr("日志级别")
                            C.ConfigChoice { settingKey: "daemon.log_level"; values: ["trace", "debug", "info", "warn", "error"]; labels: ["Trace", "Debug", "Info", "Warn", "Error"]; fallbackValue: "info" }
                        }
                        Flow {
                            Layout.fillWidth: true; spacing: 10
                            C.UiButton { text: qsTr("查看日志"); iconName: "log"; kind: "tonal"; enabled: DaemonState.daemonConnected; onClicked: { logs.open() } }
                            C.UiButton { text: qsTr("打开日志目录"); iconName: "folder"; enabled: Frontend.logPath.length > 0; onClicked: Frontend.openLogsDirectory() }
                            C.UiButton { text: qsTr("导出当前日志"); iconName: "log"; enabled: Frontend.logs.length > 0; onClicked: Frontend.exportLogs() }
                        }
                    }
                }
            }
        }
    }
    Dialog {
        id: logs
        onOpened: Frontend.refreshLogs(levelChoice.currentIndex ? levelChoice.currentText.toLowerCase() : "")
        anchors.centerIn: parent; width: Math.min(800, root.width - 20); height: Math.min(490, root.height - 20)
        title: qsTr("服务日志")
        modal: true; standardButtons: Dialog.Close
        background: Rectangle { color: App.Theme.surface; radius: 12; border.color: App.Theme.border }
        ColumnLayout {
            anchors.fill: parent
            RowLayout {
                C.UiComboBox { id: levelChoice; model: [qsTr("全部"), "TRACE", "DEBUG", "INFO", "WARN", "ERROR"]; onActivated: Frontend.refreshLogs(index ? currentText.toLowerCase() : ""); font.pixelSize: 12 }
                C.UiButton { text: qsTr("刷新"); iconName: "refresh"; onClicked: Frontend.refreshLogs(levelChoice.currentIndex ? levelChoice.currentText.toLowerCase() : "") }
                BusyIndicator { visible: running; running: Frontend.pending.indexOf("GetLogs") >= 0; implicitWidth: 24; implicitHeight: 24 }
                Item { Layout.fillWidth: true }
                C.UiButton { text: qsTr("导出"); enabled: Frontend.logs.length > 0; onClicked: Frontend.exportLogs() }
            }
            Text { Layout.fillWidth: true; textFormat: Text.PlainText; text: Frontend.logEnvironmentOverride ? qsTr("RUST_LOG 正在覆盖已保存的日志级别：%1").arg(Frontend.logFilter) : qsTr("运行中过滤器：%1").arg(Frontend.logFilter); color: Frontend.logEnvironmentOverride ? App.Theme.warning : App.Theme.textSecondary; font.pixelSize: 11; wrapMode: Text.Wrap }
            Text { Layout.fillWidth: true; visible: Frontend.logsError.length > 0; textFormat: Text.PlainText; text: Frontend.logsError; color: App.Theme.warning; font.pixelSize: 11; wrapMode: Text.Wrap }
            ScrollView {
                Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                TextArea { text: Frontend.logs.map(function(entry) { return typeof entry === "string" ? entry : ((entry.timestamp || "") + " " + (entry.line || "")) }).join("\n"); readOnly: true; selectByMouse: true; color: App.Theme.text; font.family: "monospace"; font.pixelSize: 11; wrapMode: TextEdit.WrapAnywhere; background: Rectangle { color: App.Theme.input } }
            }
            Text { textFormat: Text.PlainText; visible: Frontend.logs.length === 0; text: qsTr("暂无可读取的日志。请确认服务支持日志读取接口。"); color: App.Theme.textSecondary; font.pixelSize: 11 }
        }
    }
}
