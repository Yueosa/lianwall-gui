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
    Column {
        width: parent.width
        spacing: 12
        RowLayout {
            id: header
            width: parent.width
            Text { textFormat: Text.PlainText; text: qsTr("正在播放"); color: App.Theme.text; font.pixelSize: 18; font.weight: Font.DemiBold }
            Text { textFormat: Text.PlainText; text: qsTr("当前壁纸与轮换控制"); color: App.Theme.textSecondary; font.pixelSize: 11 }
            Item { Layout.fillWidth: true }
            C.UiButton { text: root.runtime.output || DaemonState.engine || qsTr("未连接"); iconName: "player"; onClicked: root.navigate(2) }
        }
        Item {
            id: stage
            width: parent.width
            readonly property real imageWidth: width - sideScroll.width - 14
            height: root.height - header.height - details.height - 24
            Item {
                width: stage.imageWidth
                height: stage.height
                    Rectangle {
                    id: previewFrame
                    objectName: "playerPreviewFrame"
                    anchors.fill: parent
                    color: App.Theme.cardHover; radius: 12; clip: true
                    Image {
                        id: preview
                        anchors.fill: parent
                        source: DaemonState.currentPath ? "image://thumbnail/" + encodeURIComponent(DaemonState.currentPath) + "?t=" + root.refreshToken : ""
                        sourceSize: Qt.size(1280, 720)
                        fillMode: Image.PreserveAspectCrop
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
            ScrollView {
                ScrollBar.vertical.policy: ScrollBar.AlwaysOff
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                id: sideScroll
                objectName: "playerSidebar"
                anchors.right: parent.right; width: Math.max(244, Math.min(320, stage.width * 0.30)); height: stage.height
                clip: true
                contentWidth: availableWidth
                ColumnLayout {
                    id: sideContent
                    width: sideScroll.availableWidth
                    spacing: 12
                    C.UiCard {
                        id: rotationCard
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
                        Layout.preferredHeight: Math.max(144, stage.height - rotationCard.height - sideContent.spacing)
                        Column {
                            width: parent.width
                            spacing: 10
                            Text { text: qsTr("显存占用"); color: App.Theme.text; font.pixelSize: 14; font.weight: Font.DemiBold }
                            RowLayout {
                                width: parent.width
                                spacing: 12
                                Item {
                                    id: gauge
                                    readonly property real total: DaemonState.daemonConnected ? DaemonState.vramTotalMb : 0
                                    readonly property real used: Math.min(total, DaemonState.vramUsedMb)
                                    readonly property real fraction: total > 0 ? used / total : 0
                                    Layout.preferredWidth: Math.min(124, Math.max(74, stage.height - rotationCard.height - 96), parent.width * 0.46)
                                    Layout.preferredHeight: width
                                    Canvas {
                                        anchors.fill: parent
                                        property real fraction: gauge.fraction
                                        property color track: App.Theme.border
                                        property color usedColor: App.Theme.accent
                                        property color freeColor: App.Theme.success
                                        property bool available: gauge.total > 0
                                        onFractionChanged: requestPaint()
                                        onTrackChanged: requestPaint()
                                        onUsedColorChanged: requestPaint()
                                        onFreeColorChanged: requestPaint()
                                        onAvailableChanged: requestPaint()
                                        onPaint: {
                                            const ctx = getContext("2d")
                                            ctx.clearRect(0, 0, width, height)
                                            const center = width / 2
                                            function ring(radius, thickness, part, ink) {
                                                ctx.lineWidth = thickness
                                                ctx.lineCap = "round"
                                                for (const [amount, color] of [[1, track], [part, ink]]) {
                                                    if (amount <= 0) continue
                                                    ctx.beginPath()
                                                    ctx.strokeStyle = color
                                                    ctx.arc(center, center, radius, -Math.PI / 2, Math.PI * (2 * amount - 0.5))
                                                    ctx.stroke()
                                                }
                                            }
                                            ring(center * 0.9, width * 0.067, fraction, usedColor)
                                            ring(center * 0.70, width * 0.038, available ? 1 - fraction : 0, freeColor)
                                        }
                                    }
                                    Text { anchors.centerIn: parent; text: gauge.total ? Math.round(gauge.fraction * 100) + "%" : "—"; color: App.Theme.text; font.pixelSize: Math.round(parent.width * 0.21); font.weight: Font.DemiBold }
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 6
                                    Text { text: qsTr("已用") + "  " + (gauge.total ? gauge.used + " MB" : "—"); color: App.Theme.accent; font.pixelSize: 12 }
                                    Text { text: qsTr("可用") + "  " + (gauge.total ? (gauge.total - gauge.used) + " MB" : "—"); color: App.Theme.success; font.pixelSize: 12 }
                                    Text { text: gauge.total ? qsTr("总计") + "  " + gauge.total + " MB" : qsTr("暂无显存读数"); color: App.Theme.textSecondary; font.pixelSize: 11 }
                                }
                            }
                        }
                    }
                }
            }
        }
        RowLayout {
            id: details
            width: parent.width
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Text { Layout.fillWidth: true; textFormat: Text.PlainText; text: DaemonState.currentFilename || qsTr("暂无壁纸"); color: App.Theme.text; font.pixelSize: 19; elide: Text.ElideMiddle }
                    Text { Layout.fillWidth: true; textFormat: Text.PlainText; text: DaemonState.currentPath || (DaemonState.daemonConnected ? "" : qsTr("壁纸服务未连接")); color: App.Theme.textSecondary; font.pixelSize: 10; elide: Text.ElideMiddle }
                }
                C.UiButton { iconName: "prev"; kind: "flat"; hint: qsTr("上一张壁纸"); enabled: DaemonState.daemonConnected && !root.busy; onClicked: LianwallApp.daemonPrev() }
                C.UiButton { text: Frontend.rotationPaused ? (Frontend.temporaryPause ? qsTr("恢复轮换") : qsTr("轮换已关闭")) : qsTr("暂停轮换"); iconName: Frontend.rotationPaused ? "refresh" : "pause"; kind: "tonal"; hint: qsTr("只控制自动轮换，视频继续播放"); enabled: DaemonState.daemonConnected; onClicked: { if (Frontend.rotationPaused && !Frontend.temporaryPause) root.navigate(4); else Frontend.toggleRotation() } }
                C.UiButton { iconName: "next"; kind: "flat"; hint: qsTr("下一张壁纸"); enabled: DaemonState.daemonConnected && !root.busy; onClicked: LianwallApp.daemonNext() }
        }
    }
}
