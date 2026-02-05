import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LianwallGui 1.0

GridView {
    id: root
    
    property var wallpaperModel: null
    property int selectedIndex: -1
    property string thumbnailDir: ""
    
    signal wallpaperClicked(int index, var wallpaper)
    signal wallpaperRightClicked(int index, var wallpaper)
    signal wallpaperDoubleClicked(int index, var wallpaper)
    
    cellWidth: 180
    cellHeight: 140
    clip: true
    
    model: wallpaperModel
    
    delegate: WallpaperCard {
        width: root.cellWidth - 8
        height: root.cellHeight - 8
        
        wallpaperPath: model.path || ""
        wallpaperName: model.name || ""
        isVideo: model.isVideo || false
        isLocked: model.locked || false
        inCooldown: model.inCooldown || false
        isCurrent: model.isCurrent || false
        thumbnailSource: model.thumbnail || ""
        
        isSelected: root.selectedIndex === index
        
        onClicked: {
            root.selectedIndex = index
            root.wallpaperClicked(index, model)
        }
        
        onRightClicked: {
            root.selectedIndex = index
            root.wallpaperRightClicked(index, model)
        }
        
        onDoubleClicked: {
            root.wallpaperDoubleClicked(index, model)
        }
    }
    
    // 右键菜单
    Menu {
        id: contextMenu
        
        property var currentWallpaper: null
        property int currentIndex: -1
        
        MenuItem {
            text: qsTr("设为壁纸")
            icon.name: "image-x-generic"
            onTriggered: {
                if (contextMenu.currentWallpaper) {
                    Client.setWallpaper(contextMenu.currentWallpaper.path)
                }
            }
        }
        
        MenuSeparator {}
        
        MenuItem {
            text: contextMenu.currentWallpaper && contextMenu.currentWallpaper.locked 
                ? qsTr("解锁") : qsTr("锁定")
            icon.name: contextMenu.currentWallpaper && contextMenu.currentWallpaper.locked 
                ? "object-unlocked" : "object-locked"
            onTriggered: {
                if (contextMenu.currentWallpaper) {
                    if (contextMenu.currentWallpaper.locked) {
                        Client.unlock(contextMenu.currentWallpaper.path)
                    } else {
                        Client.lock(contextMenu.currentWallpaper.path)
                    }
                }
            }
        }
        
        MenuSeparator {}
        
        MenuItem {
            text: qsTr("在文件管理器中显示")
            icon.name: "folder-open"
            onTriggered: {
                if (contextMenu.currentWallpaper) {
                    var dir = contextMenu.currentWallpaper.path.substring(
                        0, contextMenu.currentWallpaper.path.lastIndexOf("/"))
                    Qt.openUrlExternally("file://" + dir)
                }
            }
        }
        
        MenuItem {
            text: qsTr("复制路径")
            icon.name: "edit-copy"
            onTriggered: {
                if (contextMenu.currentWallpaper) {
                    // TODO: Copy to clipboard
                }
            }
        }
    }
    
    onWallpaperRightClicked: function(index, wallpaper) {
        contextMenu.currentIndex = index
        contextMenu.currentWallpaper = wallpaper
        contextMenu.popup()
    }
    
    // 空状态
    Label {
        anchors.centerIn: parent
        text: qsTr("没有找到壁纸")
        color: "#6c7086"
        visible: root.count === 0
    }
    
    // 滚动条
    ScrollBar.vertical: ScrollBar {
        active: true
    }
}
