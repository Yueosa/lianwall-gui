import QtQuick
import QtQuick.Layouts
import ".." as App

/// 占位页面（后续 Phase 会替换为真正实现）
Item {
    property string pageName: "Page"
    property string pageIcon: "📄"

    ColumnLayout {
        anchors.centerIn: parent
        spacing: App.Theme.spacingMedium

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: pageIcon
            font.pixelSize: 48
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: pageName
            font.pixelSize: App.Theme.fontSizeHeader
            font.bold: true
            color: App.Theme.text
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("即将实现")
            font.pixelSize: App.Theme.fontSizeMedium
            color: App.Theme.textSecondary
        }

        // daemon 连接状态
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: DaemonState.daemonConnected
                  ? "✅ " + qsTr("Daemon 已连接")
                  : "❌ " + qsTr("Daemon 未连接")
            font.pixelSize: App.Theme.fontSizeSmall
            color: DaemonState.daemonConnected ? App.Theme.success : App.Theme.error
        }
    }
}
