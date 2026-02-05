import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    
    property string trackName: ""
    property string timeRange: ""  // "08:00-18:00"
    property var wallpapers: []
    property bool isExpanded: true
    property bool isActive: false
    
    signal wallpaperDropped(string wallpaperPath)
    signal wallpaperRemoved(int index)
    signal trackRemoved()
    signal trackEdited()
    
    height: isExpanded ? headerHeight + contentHeight : headerHeight
    
    property int headerHeight: 48
    property int contentHeight: Math.max(100, Math.ceil(wallpapers.length / 4) * 90 + 20)
    
    // 轨道头部
    Rectangle {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: headerHeight
        color: isActive ? "#313244" : "#1e1e2e"
        border.color: isActive ? "#89b4fa" : "#313244"
        border.width: 1
        radius: 4
        
        RowLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 8
            
            // 展开/折叠按钮
            ToolButton {
                icon.name: isExpanded ? "go-down" : "go-next"
                onClicked: isExpanded = !isExpanded
            }
            
            // 时间范围
            Rectangle {
                width: timeLabel.width + 16
                height: 28
                radius: 4
                color: isActive ? "#89b4fa" : "#45475a"
                
                Label {
                    id: timeLabel
                    anchors.centerIn: parent
                    text: timeRange || qsTr("默认")
                    color: isActive ? "#1e1e2e" : "#cdd6f4"
                    font.bold: true
                    font.family: "monospace"
                }
            }
            
            // 轨道名称
            Label {
                text: trackName
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
            
            // 壁纸数量
            Label {
                text: wallpapers.length + qsTr(" 个壁纸")
                color: "#6c7086"
            }
            
            // 编辑按钮
            ToolButton {
                icon.name: "document-edit"
                onClicked: root.trackEdited()
                
                ToolTip.visible: hovered
                ToolTip.text: qsTr("编辑时间范围")
            }
            
            // 删除按钮
            ToolButton {
                icon.name: "edit-delete"
                onClicked: root.trackRemoved()
                
                ToolTip.visible: hovered
                ToolTip.text: qsTr("删除此轨道")
            }
        }
    }
    
    // 轨道内容
    Rectangle {
        id: content
        anchors.top: header.bottom
        anchors.topMargin: 4
        anchors.left: parent.left
        anchors.right: parent.right
        height: contentHeight
        color: "#181825"
        border.color: "#313244"
        border.width: 1
        radius: 4
        visible: isExpanded
        
        // 拖放区域
        DropArea {
            anchors.fill: parent
            keys: ["wallpaper"]
            
            onEntered: function(drag) {
                content.border.color = "#89b4fa"
            }
            
            onExited: {
                content.border.color = "#313244"
            }
            
            onDropped: function(drop) {
                content.border.color = "#313244"
                if (drop.hasText) {
                    root.wallpaperDropped(drop.text)
                }
            }
        }
        
        // 壁纸网格
        GridView {
            anchors.fill: parent
            anchors.margins: 8
            cellWidth: 80
            cellHeight: 80
            clip: true
            
            model: wallpapers
            
            delegate: Rectangle {
                width: 72
                height: 72
                radius: 4
                color: "#313244"
                
                Image {
                    anchors.fill: parent
                    anchors.margins: 2
                    source: modelData.thumbnail || ""
                    fillMode: Image.PreserveAspectCrop
                    
                    Rectangle {
                        anchors.fill: parent
                        color: "transparent"
                        radius: 2
                    }
                }
                
                // 删除按钮
                Rectangle {
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.margins: 2
                    width: 20
                    height: 20
                    radius: 10
                    color: "#f38ba8"
                    visible: mouseArea.containsMouse
                    
                    Label {
                        anchors.centerIn: parent
                        text: "×"
                        color: "#1e1e2e"
                        font.bold: true
                    }
                    
                    MouseArea {
                        anchors.fill: parent
                        onClicked: root.wallpaperRemoved(index)
                    }
                }
                
                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.NoButton
                }
                
                // 拖动支持
                Drag.active: dragArea.drag.active
                Drag.keys: ["wallpaper"]
                Drag.mimeData: { "text/plain": modelData.path }
                
                MouseArea {
                    id: dragArea
                    anchors.fill: parent
                    drag.target: parent
                    
                    onReleased: {
                        parent.x = 0
                        parent.y = 0
                    }
                }
            }
        }
        
        // 空状态提示
        Label {
            anchors.centerIn: parent
            text: qsTr("拖放壁纸到此处")
            color: "#6c7086"
            visible: wallpapers.length === 0
        }
    }
    
    Behavior on height {
        NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
    }
}
