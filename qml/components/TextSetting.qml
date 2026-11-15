import QtQuick
import QtQuick.Controls
import ".." as App
SettingRow {
    id: root
    property alias draftValue: field.text
    property string settingKey
    property string placeholder: ""
    property bool dirty: false
    readonly property string savedValue: { ConfigManager.document; return String(ConfigManager.value(settingKey, "")) }
    onSavedValueChanged: if (!field.activeFocus && !dirty) field.text = savedValue
    Component.onCompleted: field.text = savedValue
    TextField {
        id: field
        objectName: "field_" + root.settingKey
        implicitWidth: Math.min(330, root.width * 0.55); implicitHeight: 34
        placeholderText: root.placeholder
        color: App.Theme.text; font.pixelSize: 12
        selectByMouse: true
        enabled: DaemonState.daemonConnected && ConfigManager.pendingKeys.indexOf(root.settingKey) < 0
        background: Rectangle { color: App.Theme.input; radius: 7; border.color: field.activeFocus ? App.Theme.accent : "transparent" }
        onTextEdited: root.dirty = true
        onAccepted: { root.forceActiveFocus(); ConfigManager.setValue(root.settingKey, text) }
        Keys.onEscapePressed: { text = root.savedValue; root.dirty = false; root.forceActiveFocus() }
    }
    UiButton { iconName: "check"; kind: "flat"; enabled: root.dirty && field.enabled; onClicked: { root.forceActiveFocus(); ConfigManager.setValue(root.settingKey, field.text) } }
    Connections { target: ConfigManager; function onDocumentChanged() { if (!root.dirty && !field.activeFocus) field.text = String(ConfigManager.value(root.settingKey, "")) } function onWriteFinished(key, success, message) { if (key !== root.settingKey) return; if (success) root.dirty = false; root.errorText = success ? "" : message } }
}
