import QtQuick
import QtQuick.Controls
import ".." as App
SettingRow {
    id: root
    property string settingKey
    property real defaultValue: 0
    property real factor: 1
    property int decimals: 0
    property real minValue: 0
    property real maxValue: 86400
    property string unit: ""
    property bool allowZero: false
    property bool dirty: false
    readonly property real savedValue: { ConfigManager.document; return Number(ConfigManager.value(settingKey, defaultValue)) }
    readonly property bool pending: ConfigManager.pendingKeys.indexOf(settingKey) >= 0
    onSavedValueChanged: if (!field.activeFocus && !dirty) field.text = String(savedValue / factor)
    Component.onCompleted: field.text = String(savedValue / factor)
    function cancel() { dirty = false; errorText = ""; field.text = String(savedValue / factor); root.forceActiveFocus() }
    function commit() {
        const n = Number(field.text) * factor
        if (!field.text.trim().length || !isFinite(n) || (!allowZero || n !== 0) && (n < minValue || n > maxValue)) {
            errorText = qsTr("请输入有效数值，范围 %1–%2。").arg(minValue / factor).arg(maxValue / factor)
            return
        }
        errorText = ""
        root.forceActiveFocus()
        const scale = Math.pow(10, decimals)
        ConfigManager.setValue(settingKey, Math.round(n * scale) / scale)
    }
    TextField {
        id: field
        objectName: "field_" + root.settingKey
        implicitWidth: 76; implicitHeight: 33
        enabled: !root.pending && DaemonState.daemonConnected
        color: App.Theme.text; font.pixelSize: 12
        horizontalAlignment: TextInput.AlignHCenter
        selectByMouse: true
        background: Rectangle { radius: 7; color: App.Theme.input; border.color: field.activeFocus ? App.Theme.accent : "transparent" }
        onTextEdited: { root.dirty = true; root.errorText = "" }
        onAccepted: root.commit()
        Keys.onEscapePressed: root.cancel()
    }
    Text { text: root.unit; color: App.Theme.textSecondary; font.pixelSize: 11 }
    UiButton { iconName: "check"; kind: "flat"; enabled: root.dirty && !root.pending && DaemonState.daemonConnected; hint: qsTr("保存 · Enter"); onClicked: root.commit() }
    Connections {
        target: ConfigManager
        function onDocumentChanged() { if (!root.dirty && !field.activeFocus) field.text = String(Number(ConfigManager.value(root.settingKey, root.defaultValue)) / root.factor) }
        function onWriteFinished(key, success, message) {
            if (key !== root.settingKey) return
            if (success) root.dirty = false
            else root.errorText = message
        }
    }
}
