import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".." as App
import "../components" as C
Item {
    id: root
    onVisibleChanged: if (visible && DaemonState.daemonConnected) Frontend.refreshHooks()
    ColumnLayout {
        anchors.fill: parent; spacing: 12
        RowLayout {
            Layout.fillWidth: true
            Text { textFormat: Text.PlainText; text: qsTr("自动化"); font.pixelSize: 18; font.weight: Font.DemiBold; color: App.Theme.text }
            Text { textFormat: Text.PlainText; text: qsTr("Hooks 规则与触发条件"); font.pixelSize: 11; color: App.Theme.textSecondary }
            Item { Layout.fillWidth: true }
            C.UiButton { text: qsTr("编辑配置"); iconName: "folder"; onClicked: Frontend.openHooksFile() }
            C.UiButton { text: qsTr("重新加载"); iconName: "refresh"; enabled: DaemonState.daemonConnected && Frontend.pending.indexOf("ReloadHooks") < 0; onClicked: Frontend.reloadHooks() }
        }
        Text { textFormat: Text.PlainText; visible: Frontend.hooksError.length > 0; text: Frontend.hooksError; color: App.Theme.error; font.pixelSize: 12; Layout.fillWidth: true; wrapMode: Text.Wrap }
        C.UiCard {
            Layout.fillWidth: true; Layout.fillHeight: true; implicitHeight: 300
            ColumnLayout {
                anchors.fill: parent
                spacing: 10
                RowLayout {
                    Layout.fillWidth: true
                    Text { textFormat: Text.PlainText; text: qsTr("规则列表"); color: App.Theme.text; font.pixelSize: 14; font.weight: Font.DemiBold }
                    Text { textFormat: Text.PlainText; text: qsTr("%1 条规则").arg(Frontend.hooks.length); color: App.Theme.textSecondary; font.pixelSize: 11 }
                    Item { Layout.fillWidth: true }
                    C.UiButton { iconName: "refresh"; kind: "flat"; enabled: DaemonState.daemonConnected; onClicked: Frontend.refreshHooks() }
                }
                ListView {
                    id: list
                    Layout.fillWidth: true; Layout.fillHeight: true
                    model: Frontend.hooks
                    clip: true; spacing: 0
                    ScrollBar.vertical: ScrollBar {}
                    delegate: Item {
                        required property var modelData
                        width: list.width; height: 88
                        RowLayout {
                            anchors.fill: parent; anchors.topMargin: 12; anchors.bottomMargin: 12
                            spacing: 13
                            C.UiIcon { name: "automation"; Layout.preferredWidth: 26; Layout.preferredHeight: 26 }
                            ColumnLayout {
                                Layout.fillWidth: true; spacing: 4
                                Text { textFormat: Text.PlainText; text: modelData.name || "—"; color: App.Theme.text; font.pixelSize: 13; Layout.fillWidth: true; elide: Text.ElideRight }
                                Text { textFormat: Text.PlainText; text: modelData.command || ""; color: App.Theme.textSecondary; font.pixelSize: 10; Layout.fillWidth: true; elide: Text.ElideMiddle; font.family: "monospace" }
                            }
                            ColumnLayout {
                                Layout.preferredWidth: 170; spacing: 4
                                Text { textFormat: Text.PlainText; text: modelData.on || ""; color: App.Theme.accent; font.pixelSize: 11 }
                                Text { textFormat: Text.PlainText; text: (modelData.mode || qsTr("所有模式")) + (modelData.trigger ? " · " + modelData.trigger : ""); color: App.Theme.textSecondary; font.pixelSize: 10 }
                            }
                            Text { textFormat: Text.PlainText; text: modelData.enabled ? qsTr("启用") : qsTr("停用"); color: modelData.enabled ? App.Theme.success : App.Theme.textSecondary; font.pixelSize: 11 }
                            C.UiButton { iconName: "info"; kind: "flat"; hint: qsTr("规则详情"); onClicked: { details.hook = modelData; details.open() } }
                        }
                        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: App.Theme.border }
                    }
                    Text { textFormat: Text.PlainText; anchors.centerIn: parent; visible: list.count === 0; text: DaemonState.daemonConnected ? qsTr("尚未配置 Hook。可以编辑配置后重新加载。") : qsTr("连接壁纸服务后查看规则。"); color: App.Theme.textSecondary; font.pixelSize: 12 }
                }
            }
        }
    }
    Dialog {
        id: details
        property var hook: ({})
        title: hook.name || qsTr("规则详情")
        anchors.centerIn: parent
        width: Math.min(560, root.width - 30)
        modal: true; standardButtons: Dialog.Close
        background: Rectangle { color: App.Theme.surface; radius: 12; border.color: App.Theme.border }
        ColumnLayout {
            width: parent.width
            Text { textFormat: Text.PlainText; text: details.hook.on || ""; color: App.Theme.accent; font.pixelSize: 12 }
            TextArea { Layout.fillWidth: true; text: details.hook.command || ""; readOnly: true; wrapMode: TextEdit.WrapAnywhere; color: App.Theme.text; font.family: "monospace"; font.pixelSize: 12; selectByMouse: true; background: Rectangle { color: App.Theme.input; radius: 7 } }
            Text { textFormat: Text.PlainText; text: qsTr("超时：%1 秒").arg(details.hook.timeout || 0); color: App.Theme.textSecondary; font.pixelSize: 11 }
        }
    }
}
