import QtQuick
import QtQuick.Shapes
import ".." as App
Item {
    id: root
    property string name: "info"
    property color color: App.Theme.accent
    property bool filled: false
    implicitWidth: 18
    implicitHeight: 18
    readonly property var paths: ({
        "player": "M6 4H18Q21 4 21 7V15Q21 18 18 18H6Q3 18 3 15V7Q3 4 6 4ZM8 22H16M12 18V22M10 8L16 11L10 14Z",
        "library": "M3 3H10V10H3ZM14 3H21V10H14ZM3 14H10V21H3ZM14 14H21V21H14Z",
        "engine": "M8 6H16Q18 6 18 8V16Q18 18 16 18H8Q6 18 6 16V8Q6 6 8 6ZM9 9H15V15H9ZM9 3V6M15 3V6M9 18V21M15 18V21M3 9H6M3 15H6M18 9H21M18 15H21",
        "automation": "M8 5L3 12L8 19M16 5L21 12L16 19M13 2L11 22",
        "settings": "M3 7H7M13 7H21M3 17H11M17 17H21M10 4A3 3 0 1 1 10 10A3 3 0 1 1 10 4M14 14A3 3 0 1 1 14 20A3 3 0 1 1 14 14",
        "next": "M5 5L15 12L5 19ZM19 5V19", "prev": "M19 5L9 12L19 19ZM5 5V19",
        "pause": "M9 6V18M15 6V18", "play": "M8 5L19 12L8 19Z",
        "refresh": "M20 8A8 8 0 0 0 6 6L3 9M3 4V9H8M4 16A8 8 0 0 0 18 18L21 15M21 20V15H16",
        "heart": "M20.5 5.5A5 5 0 0 0 13.5 5.5L12 7L10.5 5.5A5 5 0 0 0 3.5 12.5L12 21L20.5 12.5A5 5 0 0 0 20.5 5.5Z",
        "search": "M10.5 4A6.5 6.5 0 1 1 10.5 17A6.5 6.5 0 1 1 10.5 4M16 16L21 21",
        "plus": "M12 4V20M4 12H20",
        "check": "M5 12L9 16L19 6", "close": "M6 6L18 18M6 18L18 6",
        "folder": "M3 7Q3 5 5 5H10L12 7H19Q21 7 21 9V19Q21 21 19 21H5Q3 21 3 19Z",
        "moon": "M20 15A9 9 0 0 1 9 4A9 9 0 1 0 20 15Z",
        "sun": "M12 8A4 4 0 1 1 12 16A4 4 0 1 1 12 8M12 2V4M12 20V22M2 12H4M20 12H22M5 5L6 6M18 18L19 19M5 19L6 18M18 6L19 5",
        "lock": "M7 10H17Q19 10 19 12V19Q19 21 17 21H7Q5 21 5 19V12Q5 10 7 10ZM8 10V7A4 4 0 0 1 16 7V10M12 14V17",
        "list": "M8 5H21M8 12H21M8 19H21M3 5H3.2M3 12H3.2M3 19H3.2",
        "chevron": "M9 5L16 12L9 19", "more": "M4 12H5M11 12H12M18 12H19",
        "log": "M5 3H15L19 7V21H5ZM14 3V8H19M8 12H16M8 16H13",
        "info": "M12 3A9 9 0 1 1 12 21A9 9 0 1 1 12 3M12 11V17M12 7V7.2",
        "bolt": "M13 2L4 14H11L10 22L20 9H12Z"
    })
    Shape {
        width: 24; height: 24
        scale: Math.min(root.width, root.height) / 24
        transformOrigin: Item.TopLeft
        ShapePath {
            strokeColor: root.color
            strokeWidth: 1.65
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            fillColor: root.filled ? root.color : "transparent"
            PathSvg { path: root.paths[root.name] || root.paths.info }
        }
    }
}
