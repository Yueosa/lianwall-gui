import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".." as App
ColumnLayout {
    id: root
    property string settingKey
    property string title
    property string description: qsTr("每行是一个独立参数，空格和引号按原样传递，不经过 shell 拆词。")
    property var draft: []
    property bool dirty: false
    property string errorText: ""
    readonly property var savedValue: { ConfigManager.document; return ConfigManager.value(settingKey, []) }
    readonly property bool pending: ConfigManager.pendingKeys.indexOf(settingKey) >= 0
    spacing: 6
    function reset() { draft = Array.from(savedValue); dirty = false; errorText = ""; root.forceActiveFocus() }
    function commit() { errorText = ""; root.forceActiveFocus(); ConfigManager.setValue(settingKey, draft) }
    function edit(index, value) { const copy = draft.slice(); copy[index] = value; draft = copy; dirty = true }
    onSavedValueChanged: if (!dirty) draft = Array.from(savedValue)
    Component.onCompleted: draft = Array.from(savedValue)
    RowLayout {
        Layout.fillWidth: true
        Text { text: root.title; color: App.Theme.text; font.pixelSize: 12; textFormat: Text.PlainText; Layout.fillWidth: true; wrapMode: Text.Wrap }
        UiButton { text: qsTr("添加参数"); iconName: "plus"; kind: "flat"; enabled: DaemonState.daemonConnected && !root.pending; onClicked: { const copy = root.draft.slice(); copy.push(""); root.draft = copy; root.dirty = true } }
        UiButton { text: qsTr("保存"); iconName: "check"; kind: "flat"; enabled: root.dirty && DaemonState.daemonConnected && !root.pending; onClicked: root.commit() }
        UiButton { text: qsTr("取消"); kind: "flat"; enabled: root.dirty && !root.pending; onClicked: root.reset() }
    }
    Text { Layout.fillWidth: true; text: root.description; color: App.Theme.textSecondary; textFormat: Text.PlainText; font.pixelSize: 10; wrapMode: Text.Wrap }
    Repeater {
        model: root.draft.length
        RowLayout {
            required property int index
            Layout.fillWidth: true
            TextField {
                Layout.fillWidth: true; implicitHeight: 34
                objectName: "argument_" + root.settingKey + "_" + parent.index
                text: root.draft[parent.index]; color: App.Theme.text; font.pixelSize: 11; font.family: "monospace"; selectByMouse: true
                enabled: DaemonState.daemonConnected && !root.pending
                background: Rectangle { color: App.Theme.input; radius: 7; border.color: parent.activeFocus ? App.Theme.accent : "transparent" }
                onTextEdited: root.edit(parent.index, text)
                onAccepted: root.commit()
                Keys.onEscapePressed: root.reset()
            }
            UiButton { iconName: "close"; kind: "flat"; hint: qsTr("移除此参数"); enabled: DaemonState.daemonConnected && !root.pending; onClicked: { const copy = root.draft.slice(); copy.splice(parent.index, 1); root.draft = copy; root.dirty = true } }
        }
    }
    Text { Layout.fillWidth: true; visible: root.errorText.length > 0; text: root.errorText; textFormat: Text.PlainText; font.pixelSize: 11; color: App.Theme.error; wrapMode: Text.Wrap }
    Connections { target: ConfigManager; function onDocumentChanged() { if (!root.dirty) root.draft = Array.from(ConfigManager.value(root.settingKey, [])) } function onWriteFinished(key, success, message) { if (key !== root.settingKey) return; if (success) root.dirty = false; root.errorText = success ? "" : message } }
}
