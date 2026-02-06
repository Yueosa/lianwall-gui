import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".." as App

/// 壁纸详情弹窗
/// 大图预览 + 文件信息 + 操作按钮
Dialog {
    id: detailDialog

    // 调用者设置
    property int wallpaperRow: -1
    property string wallpaperPath: ""
    property string wallpaperFilename: ""
    property bool wallpaperLocked: false
    property bool wallpaperIsCurrent: false
    property bool wallpaperIsVideo: false

    anchors.centerIn: parent
    width: Math.min(parent.width * 0.85, 720)
    height: Math.min(parent.height * 0.9, 680)
    modal: true
    dim: true

    background: Rectangle {
        radius: App.Theme.radiusLarge
        color: App.Theme.card
        border.width: 1
        border.color: App.Theme.border
    }

    header: RowLayout {
        spacing: App.Theme.spacingSmall

        Item { Layout.preferredWidth: App.Theme.spacingMedium }

        Text {
            Layout.fillWidth: true
            text: detailDialog.wallpaperFilename || qsTr("壁纸详情")
            font.pixelSize: App.Theme.fontSizeLarge
            font.bold: true
            color: App.Theme.text
            elide: Text.ElideMiddle
            Layout.topMargin: App.Theme.spacingMedium
        }

        Button {
            text: "✕"
            flat: true
            Layout.topMargin: App.Theme.spacingSmall
            Layout.rightMargin: App.Theme.spacingSmall
            onClicked: detailDialog.close()
            background: Rectangle {
                radius: App.Theme.radiusSmall
                color: parent.hovered ? App.Theme.cardHover : "transparent"
            }
            contentItem: Text {
                text: parent.text
                font.pixelSize: App.Theme.fontSizeMedium
                color: App.Theme.textSecondary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    contentItem: ColumnLayout {
        spacing: App.Theme.spacingMedium

        // 大图预览
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: width * 9 / 16
            radius: App.Theme.radiusMedium
            color: App.Theme.surface
            clip: true

            Image {
                id: detailPreview
                anchors.fill: parent
                source: {
                    if (!detailDialog.wallpaperPath) return ""
                    if (detailDialog.wallpaperIsVideo) {
                        return "image://thumbnail/" + encodeURIComponent(detailDialog.wallpaperPath)
                    }
                    return "file://" + detailDialog.wallpaperPath
                }
                fillMode: Image.PreserveAspectFit
                asynchronous: true
                cache: false
            }

            // 加载状态
            ColumnLayout {
                anchors.centerIn: parent
                spacing: App.Theme.spacingSmall
                visible: detailPreview.status !== Image.Ready

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: detailDialog.wallpaperIsVideo ? "🎬" : "🖼️"
                    font.pixelSize: 48
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: detailPreview.status === Image.Loading ? qsTr("加载中...") : qsTr("无法预览")
                    font.pixelSize: App.Theme.fontSizeSmall
                    color: App.Theme.textSecondary
                }
            }

            // 类型标签
            Rectangle {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.margins: App.Theme.spacingSmall
                width: typeLabel.width + App.Theme.spacingSmall * 2
                height: typeLabel.height + App.Theme.spacingTiny * 2
                radius: App.Theme.radiusSmall
                color: detailDialog.wallpaperIsVideo
                       ? Qt.rgba(App.Theme.accent.r, App.Theme.accent.g, App.Theme.accent.b, 0.85)
                       : Qt.rgba(App.Theme.primary.r, App.Theme.primary.g, App.Theme.primary.b, 0.85)

                Text {
                    id: typeLabel
                    anchors.centerIn: parent
                    text: detailDialog.wallpaperIsVideo ? "Video" : "Image"
                    font.pixelSize: App.Theme.fontSizeSmall
                    font.bold: true
                    color: App.Theme.textOnAccent
                }
            }
        }

        // 文件信息
        ColumnLayout {
            Layout.fillWidth: true
            spacing: App.Theme.spacingTiny

            InfoRow { label: qsTr("文件名"); value: detailDialog.wallpaperFilename }
            InfoRow { label: qsTr("路径");   value: detailDialog.wallpaperPath }
            InfoRow {
                label: qsTr("状态")
                value: {
                    var parts = []
                    if (detailDialog.wallpaperLocked)    parts.push(qsTr("🔒 已锁定"))
                    if (detailDialog.wallpaperIsCurrent) parts.push(qsTr("▶ 当前壁纸"))
                    return parts.length > 0 ? parts.join("  ") : qsTr("正常")
                }
            }
        }

        // 操作按钮
        RowLayout {
            Layout.fillWidth: true
            spacing: App.Theme.spacingSmall

            Button {
                Layout.fillWidth: true
                text: detailDialog.wallpaperIsCurrent ? qsTr("✓ 当前壁纸") : qsTr("设为当前")
                enabled: !detailDialog.wallpaperIsCurrent && DaemonState.daemonConnected
                onClicked: {
                    WallpaperFilterModel.setAsCurrent(detailDialog.wallpaperRow)
                    detailDialog.close()
                }
                background: Rectangle {
                    radius: App.Theme.radiusMedium
                    color: parent.enabled
                           ? (parent.pressed ? App.Theme.accentPressed
                              : parent.hovered ? App.Theme.accentHover
                              : App.Theme.accent)
                           : App.Theme.surface
                }
                contentItem: Text {
                    text: parent.text
                    font.pixelSize: App.Theme.fontSizeMedium
                    color: parent.enabled ? App.Theme.textOnAccent : App.Theme.textSecondary
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            Button {
                Layout.fillWidth: true
                text: detailDialog.wallpaperLocked ? qsTr("🔓 解锁") : qsTr("🔒 锁定")
                enabled: DaemonState.daemonConnected
                onClicked: {
                    WallpaperFilterModel.toggleLock(detailDialog.wallpaperRow)
                    detailDialog.wallpaperLocked = !detailDialog.wallpaperLocked
                }
                background: Rectangle {
                    radius: App.Theme.radiusMedium
                    color: parent.pressed ? App.Theme.cardHover : parent.hovered ? App.Theme.cardHover : "transparent"
                    border.width: 1
                    border.color: App.Theme.border
                }
                contentItem: Text {
                    text: parent.text
                    font.pixelSize: App.Theme.fontSizeMedium
                    color: App.Theme.text
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            Button {
                Layout.fillWidth: true
                text: qsTr("📂 打开目录")
                onClicked: {
                    var dir = detailDialog.wallpaperPath.substring(0, detailDialog.wallpaperPath.lastIndexOf('/'))
                    Qt.openUrlExternally("file://" + dir)
                }
                background: Rectangle {
                    radius: App.Theme.radiusMedium
                    color: parent.pressed ? App.Theme.cardHover : parent.hovered ? App.Theme.cardHover : "transparent"
                    border.width: 1
                    border.color: App.Theme.border
                }
                contentItem: Text {
                    text: parent.text
                    font.pixelSize: App.Theme.fontSizeMedium
                    color: App.Theme.text
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
    }

    // 内联组件
    component InfoRow: RowLayout {
        property string label: ""
        property string value: ""
        Layout.fillWidth: true
        spacing: App.Theme.spacingSmall

        Text {
            Layout.preferredWidth: 50
            text: label
            font.pixelSize: App.Theme.fontSizeSmall
            color: App.Theme.textSecondary
            horizontalAlignment: Text.AlignRight
        }
        Text {
            Layout.fillWidth: true
            text: value
            font.pixelSize: App.Theme.fontSizeSmall
            color: App.Theme.text
            elide: Text.ElideMiddle
            wrapMode: Text.NoWrap
        }
    }
}
