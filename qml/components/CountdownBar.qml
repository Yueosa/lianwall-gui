import QtQuick
import QtQuick.Layouts
import ".." as App

/// 倒计时进度条
/// 绑定 DaemonState.nextSwitchSecs，本地 Timer 每秒递减
/// 收到 wallpaperChanged 事件时自动重置
Item {
    id: countdownRoot
    implicitHeight: 48

    // 从 daemon 获取的初始倒计时秒数
    property int serverSecs: DaemonState.nextSwitchSecs

    // 本地递减显示值
    property int displaySecs: -1

    // 从配置推算的总间隔（用于进度条比例）
    // 当 daemon 首次推送 nextSwitchSecs 时，以此作为 total 基准
    property int totalSecs: 600

    // 是否有效（daemon 已连接且有倒计时值）
    readonly property bool isActive: DaemonState.daemonConnected && displaySecs >= 0

    // 进度 [0, 1]
    readonly property real progress: totalSecs > 0 && displaySecs >= 0
                                     ? Math.max(0, Math.min(1, displaySecs / totalSecs))
                                     : 0

    // 格式化时间
    function formatTime(secs) {
        if (secs < 0) return "--:--"
        var h = Math.floor(secs / 3600)
        var m = Math.floor((secs % 3600) / 60)
        var s = secs % 60
        if (h > 0)
            return "%1:%2:%3".arg(h).arg(String(m).padStart(2, '0')).arg(String(s).padStart(2, '0'))
        return "%1:%2".arg(String(m).padStart(2, '0')).arg(String(s).padStart(2, '0'))
    }

    // 当 daemon 推送新值时重置
    onServerSecsChanged: {
        if (serverSecs >= 0) {
            displaySecs = serverSecs
            // 首次或 server 值更大时更新 total 基准
            if (serverSecs > totalSecs * 0.9)
                totalSecs = serverSecs
        }
    }

    // 壁纸切换时重置
    Connections {
        target: DaemonState
        function onWallpaperChanged(path, filename, trigger) {
            // 等待 daemon 推送新的 nextSwitchSecs
            displaySecs = -1
        }
    }

    // 本地每秒递减
    Timer {
        id: tickTimer
        interval: 1000
        repeat: true
        running: countdownRoot.isActive
        onTriggered: {
            if (displaySecs > 0)
                displaySecs--
        }
    }

    // === UI ===
    ColumnLayout {
        anchors.fill: parent
        spacing: App.Theme.spacingTiny

        // 文字行：标签 + 剩余时间
        RowLayout {
            Layout.fillWidth: true
            spacing: App.Theme.spacingSmall

            Text {
                text: qsTr("下次切换")
                font.pixelSize: App.Theme.fontSizeSmall
                color: App.Theme.textSecondary
            }

            Item { Layout.fillWidth: true }

            Text {
                text: countdownRoot.formatTime(displaySecs)
                font.pixelSize: App.Theme.fontSizeMedium
                font.bold: true
                font.family: "monospace"
                color: displaySecs >= 0 && displaySecs <= 60
                       ? App.Theme.warning
                       : App.Theme.text
            }
        }

        // 进度条
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 6
            radius: 3
            color: App.Theme.border

            Rectangle {
                height: parent.height
                radius: parent.radius
                width: parent.width * countdownRoot.progress
                color: displaySecs >= 0 && displaySecs <= 60
                       ? App.Theme.warning
                       : App.Theme.accent

                Behavior on width {
                    NumberAnimation { duration: 300; easing.type: Easing.OutQuad }
                }
            }
        }
    }
}
