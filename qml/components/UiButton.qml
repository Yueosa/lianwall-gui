import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".." as App
Button {
    id: control
    property string iconName: ""
    property string kind: "outline"
    property string hint: ""
    property bool selected: false
    implicitHeight: 34
    implicitWidth: contentItem.implicitWidth + (text.length ? 24 : 16)
    padding: 8
    hoverEnabled: true
    opacity: enabled ? 1 : 0.45
    readonly property color ink: kind === "primary" ? App.Theme.textOnAccent : selected || kind === "tonal" ? App.Theme.accent : App.Theme.text
    background: Rectangle {
        radius: control.text.length ? 9 : 16
        color: control.kind === "primary" ? App.Theme.accent
             : control.kind === "tonal" || control.selected ? App.Theme.accentContainer
             : control.hovered ? App.Theme.cardHover : "transparent"
        border.color: control.activeFocus ? App.Theme.accent : control.kind === "outline" ? App.Theme.border : "transparent"
        border.width: 1
    }
    contentItem: RowLayout {
        spacing: control.text.length && control.iconName.length ? 7 : 0
        UiIcon { visible: control.iconName.length > 0; name: control.iconName; color: control.ink; Layout.preferredWidth: 17; Layout.preferredHeight: 17 }
        Text { textFormat: Text.PlainText; visible: control.text.length > 0; text: control.text; color: control.ink; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true }
    }
    UiToolTip { visible: control.hovered && control.hint.length > 0; text: control.hint }
}
