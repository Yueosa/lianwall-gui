import QtQuick
import ".." as App
Rectangle {
    id: root
    default property alias body: host.data
    property int padding: 16
    implicitHeight: host.childrenRect.height + 2 * padding
    color: App.Theme.surface
    radius: 12
    border.color: App.Theme.border
    data: Item { id: host; x: root.padding; y: root.padding; width: root.width - 2 * root.padding; height: root.height - 2 * root.padding }
}
