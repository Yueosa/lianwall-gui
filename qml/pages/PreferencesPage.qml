import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".." as App
import "../components" as C
Item {
    id: root
    property int section: 0
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
                ScrollBar.vertical.policy: ScrollBar.AlwaysOff
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
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
                            Layout.fillWidth: true; title: qsTr("启动模式"); description: qsTr("只决定服务启动时的模式；播放页可切换当前会话。")
                            C.ConfigChoice { settingKey: "paths.mode"; values: ["Video", "Image"]; labels: [qsTr("视频"), qsTr("图片")]; fallbackValue: "Video" }
                        }
                        C.NumberSetting { Layout.fillWidth: true; title: qsTr("视频切换间隔"); description: qsTr("Enter 保存并释放焦点，Esc 取消。0 表示持久关闭自动轮换。"); settingKey: "video_engine.interval"; defaultValue: 600; factor: 60; minValue: 10; maxValue: 86400; allowZero: true; unit: qsTr("分钟") }
                        C.NumberSetting { Layout.fillWidth: true; title: qsTr("图片切换间隔"); settingKey: "image_engine.interval"; defaultValue: 600; factor: 60; minValue: 10; maxValue: 86400; allowZero: true; unit: qsTr("分钟") }
                        C.TextSetting { Layout.fillWidth: true; title: qsTr("图片目录"); settingKey: "paths.image_dir" }
                        C.TextSetting { Layout.fillWidth: true; title: qsTr("视频目录"); settingKey: "paths.video_dir" }
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
                            C.UiButton { text: qsTr("跟随系统"); selected: App.Theme.current === "system"; kind: "flat"; onClicked: ConfigManager.setTheme("system") }
                            C.UiButton { text: qsTr("浅色"); selected: App.Theme.current === "lian"; kind: "flat"; onClicked: ConfigManager.setTheme("lian") }
                            C.UiButton { text: qsTr("深色"); selected: App.Theme.current === "lian-dark"; kind: "flat"; onClicked: ConfigManager.setTheme("lian-dark") }
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
                    }
                }
                C.UiCard {
                    Layout.fillWidth: true; visible: root.section === 2
                    ColumnLayout {
                        width: parent.width; spacing: 10
                        Text { text: qsTr("显存读取测试"); color: App.Theme.text; font.pixelSize: 14 }
                        RowLayout {
                            Layout.fillWidth: true; spacing: 12
                            Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: qsTr("测试当前来源与输入命令，不保存草稿，也不修改监控、壁纸模式或轮换状态。"); color: App.Theme.textSecondary; font.pixelSize: 11; wrapMode: Text.Wrap }
                            C.UiButton { text: qsTr("测试读取"); iconName: "bolt"; kind: "tonal"; enabled: DaemonState.daemonConnected && Frontend.pending.indexOf("TestVram") < 0 && ConfigManager.pendingKeys.indexOf("vram.backend") < 0; onClicked: Frontend.testVram(ConfigManager.vramBackend, vramCommand.draftValue) }
                        }
                        Text {
                            Layout.fillWidth: true; Layout.preferredHeight: 44
                            text: Frontend.pending.indexOf("TestVram") >= 0 ? qsTr("正在读取…")
                                  : Object.keys(Frontend.vramTest).length === 0 ? qsTr("尚未测试")
                                  : Frontend.vramTest.success ? qsTr("已用 %1 / %2 MB").arg(Frontend.vramTest.used_mb).arg(Frontend.vramTest.total_mb)
                                  : qsTr("测试失败：%1").arg(Frontend.vramTest.error || "")
                            color: App.Theme.textSecondary; wrapMode: Text.Wrap; elide: Text.ElideRight
                            textFormat: Text.PlainText
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
                            C.UiButton { text: qsTr("打开日志目录"); iconName: "folder"; onClicked: Frontend.openLogsDirectory() }
                        }
                    }
                }
            }
        }
    }
}
