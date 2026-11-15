import QtQuick
import QtQuick.Controls
import ".." as App
ToolTip {
    id: control
    delay: 500
    timeout: 7000
    padding: 10
    implicitWidth: Math.min(420, contentItem.implicitWidth + leftPadding + rightPadding)
    contentItem: Text {
        text: control.text; textFormat: Text.PlainText
        font.pixelSize: 11; color: App.Theme.text
        wrapMode: Text.WrapAnywhere
    }
    background: Rectangle { color: App.Theme.surface; radius: 8; border.color: App.Theme.border }
}
