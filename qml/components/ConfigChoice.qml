import QtQuick
import QtQuick.Controls
import ".." as App
UiComboBox {
    id: control
    property string settingKey: ""
    property var values: []
    property var labels: values
    property var fallbackValue: ""
    readonly property var savedValue: { ConfigManager.document; return ConfigManager.value(settingKey, fallbackValue) }
    model: labels
    currentIndex: values.indexOf(savedValue)
    displayText: currentIndex >= 0 ? currentText : String(savedValue)
    enabled: DaemonState.daemonConnected && ConfigManager.pendingKeys.indexOf(settingKey) < 0
    onActivated: ConfigManager.setValue(settingKey, values[index])
}
