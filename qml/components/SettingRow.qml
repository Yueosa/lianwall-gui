import QtQuick
import QtQuick.Layouts
import ".." as App
Item {
    id: root
    default property alias controls: right.data
    property string title: ""
    property string description: ""
    property string errorText: ""
    property bool divider: true
    implicitHeight: Math.max(labels.implicitHeight, right.implicitHeight) + 24
    RowLayout {
        anchors.fill: parent
        spacing: 16
        ColumnLayout {
            id: labels
            Layout.fillWidth: true
            spacing: 4
            Text { textFormat: Text.PlainText; text: root.title; font.pixelSize: 12; color: App.Theme.text; Layout.fillWidth: true; wrapMode: Text.Wrap }
            Text { textFormat: Text.PlainText; text: root.description; visible: text.length > 0; font.pixelSize: 10; color: App.Theme.textSecondary; Layout.fillWidth: true; wrapMode: Text.Wrap }
            Text { textFormat: Text.PlainText; text: root.errorText; visible: text.length > 0; font.pixelSize: 11; color: App.Theme.error; Layout.fillWidth: true; wrapMode: Text.Wrap }
        }
        RowLayout { id: right; spacing: 7 }
    }
    Rectangle { visible: root.divider; anchors.bottom: parent.bottom; width: parent.width; height: 1; color: App.Theme.border }
}
