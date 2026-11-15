import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".." as App
import "../components" as C
Item {
    id: root
    signal navigate(int page)
    property int refreshToken: 0
    readonly property var runtime: Frontend.runtime
    readonly property bool busy: LianwallApp.wallpaperSwitching || runtime.phase === "preparing"
    Connections { target: DaemonState; function onWallpaperChanged() { root.refreshToken++ } }
    ColumnLayout {
        anchors.fill: parent
        spacing: 12
        RowLayout {
            Layout.fillWidth: true
            Text { textFormat: Text.PlainText; text: qsTr("正在播放"); color: App.Theme.text; font.pixelSize: 18; font.weight: Font.DemiBold }
            Text { textFormat: Text.PlainText; text: qsTr("当前壁纸与轮换控制"); color: App.Theme.textSecondary; font.pixelSize: 11 }
            Item { Layout.fillWidth: true }
            C.UiButton { text: root.runtime.output || DaemonState.engine || qsTr("未连接"); iconName: "player"; onClicked: root.navigate(2) }
        }
        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true
            spacing: 14
            ColumnLayout {
                id: previewColumn
                Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumWidth: 0; Layout.preferredWidth: 1; Layout.alignment: Qt.AlignTop
                spacing: 13
                Item {
                    id: previewSpace
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    Layout.preferredHeight: Math.min(width * 9 / 16, Math.max(120, previewColumn.height - 130))
                    Rectangle {
                    id: previewFrame
                    objectName: "playerPreviewFrame"
                    anchors.centerIn: parent
                    width: Math.min(parent.width, parent.height * 16 / 9)
                    height: width * 9 / 16
                    color: App.Theme.cardHover; radius: 12; clip: true
                    Image {
                        id: preview
                        anchors.fill: parent
                        source: DaemonState.currentPath ? "image://thumbnail/" + encodeURIComponent(DaemonState.currentPath) + "?t=" + root.refreshToken : ""
                        sourceSize: Qt.size(1280, 720)
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true; cache: false
                    }
                    Text { textFormat: Text.PlainText;
                        anchors.centerIn: parent
                        text: !DaemonState.daemonConnected ? qsTr("壁纸服务未连接") : preview.status === Image.Error ? qsTr("预览生成失败，实际壁纸不受影响") : !DaemonState.currentPath ? qsTr("选择一张壁纸开始") : qsTr("正在生成预览…")
                        color: App.Theme.textSecondary; font.pixelSize: 13
                        visible: preview.status !== Image.Ready
                    }
                    Rectangle {
                        anchors.fill: parent; color: "#60000000"; visible: root.busy
                        ColumnLayout {
                            anchors.centerIn: parent
                            BusyIndicator { Layout.alignment: Qt.AlignHCenter; running: root.busy; implicitWidth: 35; implicitHeight: 35 }
                            Text { textFormat: Text.PlainText; text: qsTr("正在切换，等待呈现确认…"); color: "white"; font.pixelSize: 12 }
                        }
                    }
                }
                }
                ColumnLayout {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: previewFrame.width
                    spacing: 6
                    visible: DaemonState.currentPath.length > 0
                    Text { Layout.fillWidth: true; textFormat: Text.PlainText; text: DaemonState.currentFilename; color: App.Theme.text; font.pixelSize: 19; elide: Text.ElideMiddle }
                    Text { Layout.fillWidth: true; textFormat: Text.PlainText; text: DaemonState.currentPath; color: App.Theme.textSecondary; font.pixelSize: 10; elide: Text.ElideMiddle }
                }
                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: previewFrame.width
                    Repeater {
                        model: [qsTr("实际引擎"), qsTr("实际解码"), qsTr("当前模式")]
                        ColumnLayout {
                            required property int index
                            required property string modelData
                            Layout.fillWidth: true
                            spacing: 5
                            Text { textFormat: Text.PlainText; text: modelData; font.pixelSize: 10; color: App.Theme.textSecondary }
                            Text { textFormat: Text.PlainText;
                                text: index === 0 ? DaemonState.engine : index === 1 ? (root.runtime.decoder === "vulkan" ? qsTr("Vulkan 硬解") : root.runtime.decoder === "software" ? qsTr("软件解码") : "—") : (DaemonState.mode === "Video" ? qsTr("视频") : qsTr("图片"))
                                color: App.Theme.text; font.pixelSize: 12
                            }
                        }
                    }
                }
                Item { Layout.fillHeight: true }
            }
            ScrollView {
                id: sideScroll
                objectName: "playerSidebar"
                Layout.preferredWidth: 280; Layout.minimumWidth: 280; Layout.maximumWidth: 280; Layout.fillHeight: true
                clip: true
                contentWidth: availableWidth
                ColumnLayout {
                    width: sideScroll.availableWidth
                    spacing: 12
                    C.UiCard {
                        Layout.fillWidth: true
                        ColumnLayout {
                            width: parent.width; spacing: 11
                            RowLayout {
                                Layout.fillWidth: true
                                Text { textFormat: Text.PlainText; text: qsTr("自动轮换"); color: App.Theme.text; font.pixelSize: 14; font.weight: Font.DemiBold }
                                Item { Layout.fillWidth: true }
                                Text { textFormat: Text.PlainText; text: Frontend.rotationPaused ? qsTr("已暂停") : qsTr("进行中"); color: App.Theme.accent; font.pixelSize: 10 }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                C.UiButton { Layout.fillWidth: true; text: qsTr("视频"); iconName: "play"; selected: DaemonState.mode === "Video"; kind: "flat"; enabled: DaemonState.daemonConnected && !root.busy; onClicked: LianwallApp.daemonSetMode("Video") }
                                C.UiButton { Layout.fillWidth: true; text: qsTr("图片"); iconName: "library"; selected: DaemonState.mode === "Image"; kind: "flat"; enabled: DaemonState.daemonConnected && !root.busy; onClicked: LianwallApp.daemonSetMode("Image") }
                            }
                            C.CountdownBar { Layout.fillWidth: true }
                            Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: Frontend.rotationPaused ? (Frontend.temporaryPause ? qsTr("仅本次服务会话暂停；视频继续播放。") : qsTr("配置已关闭自动轮换，请设置正间隔恢复。")) : qsTr("暂停轮换不会暂停视频播放。"); color: App.Theme.textSecondary; font.pixelSize: 10; wrapMode: Text.Wrap }
                        }
                    }
                    C.UiCard {
                        Layout.fillWidth: true
                        ColumnLayout {
                            width: parent.width; spacing: 12
                            RowLayout {
                                Layout.fillWidth: true
                                Text { textFormat: Text.PlainText; text: qsTr("自动化状态"); font.pixelSize: 14; color: App.Theme.text; font.weight: Font.DemiBold }
                                Item { Layout.fillWidth: true }
                                C.UiButton { iconName: "chevron"; kind: "flat"; onClicked: root.navigate(3) }
                            }
                            Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: Frontend.hooksError || qsTr("已配置 %1 条 Hook 规则").arg(Frontend.hooks.length); color: App.Theme.textSecondary; font.pixelSize: 11; wrapMode: Text.Wrap }
                            Repeater {
                                model: Frontend.hooks.slice(0, 2)
                                RowLayout {
                                    required property var modelData
                                    Layout.fillWidth: true
                                    C.UiIcon { name: "automation" }
                                    Text { textFormat: Text.PlainText; Layout.fillWidth: true; text: modelData.name; font.pixelSize: 11; color: App.Theme.text; elide: Text.ElideRight }
                                    Text { textFormat: Text.PlainText; text: modelData.enabled ? qsTr("启用") : qsTr("停用"); color: App.Theme.textSecondary; font.pixelSize: 10 }
                                }
                            }
                        }
                    }
                    C.UiCard {
                        Layout.fillWidth: true
                        ColumnLayout {
                            width: parent.width; spacing: 10
                            Text { textFormat: Text.PlainText; text: qsTr("显存状态"); color: App.Theme.text; font.pixelSize: 14; font.weight: Font.DemiBold }
                            Text { textFormat: Text.PlainText; text: DaemonState.vramTotalMb > 0 ? qsTr("整卡已用 %1 / %2 MB").arg(DaemonState.vramUsedMb).arg(DaemonState.vramTotalMb) : qsTr("暂无显存读数"); color: App.Theme.textSecondary; font.pixelSize: 11 }
                            ProgressBar { Layout.fillWidth: true; value: DaemonState.vramTotalMb > 0 ? DaemonState.vramUsedMb / DaemonState.vramTotalMb : 0 }
                            Text { textFormat: Text.PlainText; text: DaemonState.vramDegraded ? qsTr("显存降级中") : qsTr("未降级"); color: DaemonState.vramDegraded ? App.Theme.warning : App.Theme.success; font.pixelSize: 11 }
                        }
                    }
                }
            }
        }
    }
}
