import QtQuick
import QtQuick.Controls
import ".." as App

/// 轻量滚动条：细滑块、无厚轨道，避免默认 ScrollView 双层滚动的怪异观感
ScrollBar {
    id: control

    policy: ScrollBar.AsNeeded
    padding: 1
    implicitWidth: orientation === Qt.Vertical ? 10 : parent.width
    implicitHeight: orientation === Qt.Horizontal ? 10 : parent.height

    contentItem: Rectangle {
        implicitWidth: 6
        implicitHeight: 6
        radius: 3
        color: control.pressed || control.hovered
               ? App.Theme.scrollbarHover
               : App.Theme.scrollbar
        opacity: control.size < 1.0 && (control.active || control.hovered) ? 0.85 : 0.0

        Behavior on opacity {
            NumberAnimation { duration: 120 }
        }
    }

    background: Item {
        implicitWidth: control.implicitWidth
        implicitHeight: control.implicitHeight
    }
}
