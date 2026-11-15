import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".." as App
import "../components" as C
Item {
    id: root
    property bool video: true
    readonly property string section: video ? "video_engine" : "image_engine"
    readonly property var runtime: Frontend.runtime
    readonly property var statistics: runtime.statistics || ({})
    readonly property string backend: { ConfigManager.document; return ConfigManager.value(section + ".backend", video ? "mpvpaper" : "awww") }
    function capability(id) {
        const entries = Frontend.engineInfo.backends || []
        for (let i = 0; i < entries.length; ++i) if (entries[i].id === id) return entries[i]
        return null
    }
    function unavailableLabel(id) {
        const entry = capability(id)
        return entry && entry.availability_basis === "not_compiled" ? qsTr("此构建未包含内置引擎") : qsTr("未安装，请先安装此引擎或选择内置引擎")
    }
    onVisibleChanged: if (visible && DaemonState.daemonConnected) Frontend.refreshOutputs()
    ColumnLayout {
        anchors.fill: parent; spacing: 12
        RowLayout {
            Layout.fillWidth: true
            Text { textFormat: Text.PlainText; text: qsTr("引擎"); font.pixelSize: 18; font.weight: Font.DemiBold; color: App.Theme.text }
            Text { textFormat: Text.PlainText; text: qsTr("配置偏好与实际运行状态"); font.pixelSize: 11; color: App.Theme.textSecondary }
            Item { Layout.fillWidth: true }
            C.UiButton { text: qsTr("刷新状态"); iconName: "refresh"; enabled: DaemonState.daemonConnected; onClicked: { Frontend.refreshEngine(); Frontend.refreshOutputs() } }
        }
        ScrollView {
            id: scroll
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true
            contentWidth: availableWidth
            RowLayout {
                width: scroll.availableWidth; spacing: 14
                ColumnLayout {
                    Layout.fillWidth: true; Layout.alignment: Qt.AlignTop; spacing: 12
                    C.UiCard {
                        Layout.fillWidth: true
                        ColumnLayout {
                            width: parent.width; spacing: 12
                            RowLayout {
                                Layout.fillWidth: true
                                Text { textFormat: Text.PlainText; text: qsTr("引擎偏好"); font.pixelSize: 14; font.weight: Font.DemiBold; color: App.Theme.text }
                                Item { Layout.fillWidth: true }
                                C.UiButton { text: qsTr("视频"); kind: "flat"; selected: root.video; onClicked: root.video = true }
                                C.UiButton { text: qsTr("图片"); kind: "flat"; selected: !root.video; onClicked: root.video = false }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                Repeater {
                                    model: root.video ? ["native", "mpvpaper"] : ["native", "awww"]
                                    C.UiButton {
                                        required property string modelData
                                        Layout.fillWidth: true; implicitHeight: 68
                                        text: (modelData === "native" ? qsTr("LianWall 内置") : modelData) + (root.capability(modelData) && !root.capability(modelData).available ? " · " + qsTr("不可用") : "")
                                        iconName: modelData === "native" ? "engine" : "player"
                                        selected: root.backend === modelData
                                        enabled: DaemonState.daemonConnected && (!root.capability(modelData) || root.capability(modelData).available)
                                        hint: root.capability(modelData) ? root.capability(modelData).available ? "" : root.unavailableLabel(modelData) : ""
                                        onClicked: ConfigManager.setValue(root.section + ".backend", modelData)
                                    }
                                }
                            }
                            Text {
                                Layout.fillWidth: true; wrapMode: Text.Wrap
                                textFormat: Text.PlainText; font.pixelSize: 11; color: App.Theme.warning
                                visible: !!root.capability(root.backend) && !root.capability(root.backend).available
                                text: root.backend + ": " + root.unavailableLabel(root.backend)
                            }
                            C.SettingRow {
                                Layout.fillWidth: true; visible: root.video && root.backend === "native"
                                title: qsTr("解码策略"); description: qsTr("实际使用的解码方式由右侧运行状态确认。")
                                C.ConfigChoice { settingKey: "video_engine.decode"; values: ["auto", "vulkan", "software"]; labels: [qsTr("自动 · 优先硬解"), qsTr("仅 Vulkan 硬解"), qsTr("软件解码")]; fallbackValue: "software" }
                            }
                            C.SettingRow {
                                Layout.fillWidth: true; visible: root.backend === "native"
                                title: qsTr("画面适配")
                                C.ConfigChoice { settingKey: root.section + ".fit"; values: ["crop", "fit"]; labels: [qsTr("填充"), qsTr("适应")]; fallbackValue: "crop" }
                            }
                            C.SettingRow {
                                Layout.fillWidth: true
                                title: qsTr("目标输出"); description: root.backend === "native" ? qsTr("内置引擎需要明确的单个输出；更换正在使用的输出可能需要重启服务。") : ""
                                C.UiComboBox {
                                    id: displayChoice
                                    implicitWidth: 180
                                    textRole: "name"; valueRole: "name"
                                    model: Frontend.outputs
                                    currentIndex: {
                                        ConfigManager.document
                                        const saved = String(ConfigManager.value(root.video ? "video_engine.display" : "image_engine.outputs", ""))
                                        for (let i = 0; i < count; ++i)
                                            if (String(model[i].name) === saved) return i
                                        return -1
                                    }
                                    displayText: currentIndex >= 0 ? currentText : qsTr("从已发现输出选择")
                                    font.pixelSize: 12
                                    enabled: DaemonState.daemonConnected && count > 0
                                    onActivated: ConfigManager.setValue(root.video ? "video_engine.display" : "image_engine.outputs", currentValue)
                                    background: Rectangle { color: App.Theme.surface; radius: 8; border.color: App.Theme.border }
                                }
                            }
                            C.TextSetting {
                                Layout.fillWidth: true
                                title: qsTr("手动指定输出")
                                description: root.backend === "native" ? qsTr("填写一个输出名称，例如 eDP-1。") : root.video ? qsTr("* 表示全部输出，或填写一个输出名称。") : qsTr("留空表示全部输出，也可填写逗号分隔的多个名称。")
                                settingKey: root.video ? "video_engine.display" : "image_engine.outputs"
                            }
                            Text { textFormat: Text.PlainText; visible: Frontend.outputsError.length > 0; text: Frontend.outputsError; Layout.fillWidth: true; wrapMode: Text.Wrap; font.pixelSize: 11; color: App.Theme.warning }
                            C.SettingRow {
                                Layout.fillWidth: true
                                visible: root.video || root.backend === "native"
                                title: qsTr("过渡效果")
                                C.ConfigChoice { settingKey: root.section + ".transition"; values: ["none", "fade"]; labels: [qsTr("无"), qsTr("淡化")]; fallbackValue: root.video ? "none" : "fade" }
                            }
                            C.NumberSetting { Layout.fillWidth: true; visible: root.backend === "native"; title: qsTr("过渡时长"); settingKey: root.section + ".transition_ms"; defaultValue: 400; minValue: 0; maxValue: 2000; unit: "ms"; divider: false }
                            C.NumberSetting { Layout.fillWidth: true; visible: root.backend === "native"; title: qsTr("动画帧率上限"); description: qsTr("仅限制切换动画，不改变视频播放帧率。"); settingKey: root.section + ".transition_fps"; defaultValue: 60; minValue: 1; maxValue: 120; unit: "fps" }
                            Text { textFormat: Text.PlainText; visible: root.video && root.backend === "mpvpaper"; Layout.fillWidth: true; text: qsTr("外部视频淡入固定为 0.4 秒；图片动画由 awww 参数控制。"); color: App.Theme.textSecondary; font.pixelSize: 10; wrapMode: Text.Wrap }
                            Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: qsTr("配置在下一次应用壁纸时生效。外部参数仅作用于对应的外部工具。"); color: App.Theme.textSecondary; font.pixelSize: 10; wrapMode: Text.Wrap }
                        }
                    }
                    C.UiCard {
                        Layout.fillWidth: true; visible: root.backend !== "native"
                        ColumnLayout {
                            width: parent.width; spacing: 12
                            Text { Layout.fillWidth: true; text: qsTr("外部引擎参数"); textFormat: Text.PlainText; font.pixelSize: 14; font.weight: Font.DemiBold; color: App.Theme.text }
                            C.ArgumentsSetting { Layout.fillWidth: true; visible: root.video; settingKey: "video_engine.mpvpaper_args"; title: qsTr("mpvpaper 参数") }
                            C.ArgumentsSetting { Layout.fillWidth: true; visible: root.video; settingKey: "video_engine.mpv_args"; title: qsTr("mpv 参数") }
                            C.ArgumentsSetting { Layout.fillWidth: true; visible: !root.video; settingKey: "image_engine.awww_args"; title: qsTr("awww 参数") }
                            C.TextSetting { Layout.fillWidth: true; visible: root.video; settingKey: "video_engine.ipc_socket"; title: qsTr("mpv IPC 路径"); description: qsTr("默认留空，切换时重建外部进程；填写路径可启用复用，需自行观察长期内存增长。") }
                        }
                    }
                }
                C.UiCard {
                    Layout.preferredWidth: Math.min(330, scroll.availableWidth * 0.38); Layout.alignment: Qt.AlignTop
                    ColumnLayout {
                        width: parent.width; spacing: 10
                        RowLayout {
                            Layout.fillWidth: true
                            Text { textFormat: Text.PlainText; text: qsTr("运行状态"); color: App.Theme.text; font.pixelSize: 14; font.weight: Font.DemiBold }
                            Item { Layout.fillWidth: true }
                            Text { textFormat: Text.PlainText; text: root.runtime.phase || DaemonState.engine; color: root.runtime.phase === "failed" ? App.Theme.error : App.Theme.success; font.pixelSize: 11 }
                        }
                        Repeater {
                            model: [
                                { label: qsTr("实际后端"), value: DaemonState.engine },
                                { label: qsTr("实际解码"), value: root.runtime.decoder || "—" },
                                { label: qsTr("当前输出"), value: root.runtime.output || "—" },
                                { label: qsTr("视频呈现帧"), value: root.statistics.video_frames === undefined ? "—" : String(root.statistics.video_frames) },
                                { label: qsTr("累计丢帧"), value: root.statistics.video_dropped === undefined ? "—" : String(root.statistics.video_dropped) }
                            ]
                            C.SettingRow {
                                required property var modelData
                                Layout.fillWidth: true; title: modelData.label
                                Text { textFormat: Text.PlainText; text: modelData.value; color: App.Theme.text; font.pixelSize: 11; elide: Text.ElideMiddle }
                            }
                        }
                        Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: root.runtime.pending ? qsTr("正在准备：%1").arg(root.runtime.pending.path) : ""; visible: text.length > 0; color: App.Theme.accent; font.pixelSize: 11; wrapMode: Text.WrapAnywhere }
                        Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: root.runtime.fallback_reason || ""; visible: text.length > 0; color: App.Theme.warning; font.pixelSize: 11; wrapMode: Text.Wrap }
                        Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: root.runtime.last_error || ""; visible: text.length > 0; color: App.Theme.error; font.pixelSize: 11; wrapMode: Text.Wrap }
                        Text { textFormat: Text.PlainText; Layout.fillWidth: true; visible: Object.keys(Frontend.engineInfo).length === 0; text: qsTr("此服务尚未提供引擎诊断接口；已有壁纸控制仍可使用。"); wrapMode: Text.Wrap; color: App.Theme.textSecondary; font.pixelSize: 11 }
                    }
                }
            }
        }
    }
}
