import QtQuick
import QtQuick.Controls
import ".." as App
ComboBox {
    id: control
    implicitWidth: 180
    implicitHeight: 34
    font.pixelSize: 12
    palette.text: App.Theme.text
    palette.buttonText: App.Theme.text
    palette.button: App.Theme.surface
    palette.base: App.Theme.surface
    palette.window: App.Theme.surface
    contentItem: Text {
        text: control.displayText; textFormat: Text.PlainText
        color: App.Theme.text; font: control.font
        verticalAlignment: Text.AlignVCenter; leftPadding: 11; rightPadding: 25; elide: Text.ElideRight
    }
    background: Rectangle { radius: 8; color: App.Theme.surface; border.color: control.activeFocus ? App.Theme.accent : App.Theme.border }
    indicator: UiIcon { name: "chevron"; color: App.Theme.textSecondary; width: 13; height: 13; rotation: 90; x: control.width - 22; y: (control.height - height) / 2 }
    delegate: ItemDelegate {
        id: option
        required property int index
        objectName: "comboOption_" + index
        width: control.width - 8; implicitHeight: 34
        text: control.textAt(index)
        highlighted: control.highlightedIndex === index
        contentItem: Text { text: option.text; textFormat: Text.PlainText; color: option.highlighted ? App.Theme.accent : App.Theme.text; font: control.font; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight }
        background: Rectangle { radius: 5; color: option.highlighted ? App.Theme.accentContainer : option.hovered ? App.Theme.cardHover : App.Theme.surface }
    }
    popup: Popup {
        y: control.height + 3; width: control.width
        implicitHeight: Math.min(contentItem.implicitHeight + 8, 280)
        padding: 4
        contentItem: ListView {
            clip: true; implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator {}
        }
        background: Rectangle { color: App.Theme.surface; radius: 8; border.color: App.Theme.border }
    }
}
