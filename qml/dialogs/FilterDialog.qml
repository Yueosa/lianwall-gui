import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root
    
    title: qsTr("筛选壁纸")
    modal: true
    standardButtons: Dialog.Ok | Dialog.Cancel
    
    property string modeFilter: "all"      // all, video, image
    property string statusFilter: "all"    // all, locked, unlocked, cooldown
    property bool activeOnly: false
    
    signal filtersApplied(string mode, string status, bool activeOnly)
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 16
        
        // 类型筛选
        GroupBox {
            Layout.fillWidth: true
            title: qsTr("壁纸类型")
            
            RowLayout {
                width: parent.width
                
                RadioButton {
                    text: qsTr("全部")
                    checked: modeFilter === "all"
                    onCheckedChanged: if (checked) modeFilter = "all"
                }
                
                RadioButton {
                    text: qsTr("视频")
                    checked: modeFilter === "video"
                    onCheckedChanged: if (checked) modeFilter = "video"
                }
                
                RadioButton {
                    text: qsTr("图片")
                    checked: modeFilter === "image"
                    onCheckedChanged: if (checked) modeFilter = "image"
                }
            }
        }
        
        // 状态筛选
        GroupBox {
            Layout.fillWidth: true
            title: qsTr("壁纸状态")
            
            GridLayout {
                columns: 2
                width: parent.width
                
                RadioButton {
                    text: qsTr("全部")
                    checked: statusFilter === "all"
                    onCheckedChanged: if (checked) statusFilter = "all"
                }
                
                RadioButton {
                    text: qsTr("已锁定")
                    checked: statusFilter === "locked"
                    onCheckedChanged: if (checked) statusFilter = "locked"
                    
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("不会自动切换到的壁纸")
                }
                
                RadioButton {
                    text: qsTr("未锁定")
                    checked: statusFilter === "unlocked"
                    onCheckedChanged: if (checked) statusFilter = "unlocked"
                }
                
                RadioButton {
                    text: qsTr("冷却中")
                    checked: statusFilter === "cooldown"
                    onCheckedChanged: if (checked) statusFilter = "cooldown"
                    
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("最近播放过，暂时不会再次播放")
                }
            }
        }
        
        // 其他选项
        GroupBox {
            Layout.fillWidth: true
            title: qsTr("其他")
            
            CheckBox {
                text: qsTr("仅显示当前时段生效的壁纸")
                checked: activeOnly
                onCheckedChanged: activeOnly = checked
                
                ToolTip.visible: hovered
                ToolTip.text: qsTr("如果启用了时间段功能，只显示当前时间段内的壁纸")
            }
        }
    }
    
    onAccepted: {
        filtersApplied(modeFilter, statusFilter, activeOnly)
    }
}
