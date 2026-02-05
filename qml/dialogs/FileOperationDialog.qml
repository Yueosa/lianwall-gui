import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Dialog {
    id: root
    
    title: qsTr("文件操作")
    modal: true
    standardButtons: Dialog.Ok | Dialog.Cancel
    
    property string sourceFile: ""
    property string targetDir: ""
    property string operation: "link"  // link, copy, move
    
    signal operationConfirmed(string source, string target, string op)
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 16
        
        // 源文件
        GroupBox {
            Layout.fillWidth: true
            title: qsTr("源文件")
            
            Label {
                width: parent.width
                text: sourceFile
                elide: Text.ElideMiddle
                wrapMode: Text.WrapAnywhere
            }
        }
        
        // 目标目录
        GroupBox {
            Layout.fillWidth: true
            title: qsTr("目标目录")
            
            RowLayout {
                width: parent.width
                
                TextField {
                    id: targetDirField
                    Layout.fillWidth: true
                    text: root.targetDir
                    placeholderText: qsTr("选择目标目录...")
                }
                
                Button {
                    text: qsTr("浏览")
                    onClicked: folderDialog.open()
                }
            }
        }
        
        // 操作类型
        GroupBox {
            Layout.fillWidth: true
            title: qsTr("操作类型")
            
            ColumnLayout {
                width: parent.width
                spacing: 8
                
                RadioButton {
                    id: linkRadio
                    text: qsTr("硬链接")
                    checked: operation === "link"
                    onCheckedChanged: if (checked) operation = "link"
                    
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("节省空间，原文件保留。文件共享相同数据块。")
                }
                
                RadioButton {
                    id: copyRadio
                    text: qsTr("复制")
                    checked: operation === "copy"
                    onCheckedChanged: if (checked) operation = "copy"
                    
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("占用双倍空间，原文件保留。创建独立副本。")
                }
                
                RadioButton {
                    id: moveRadio
                    text: qsTr("移动")
                    checked: operation === "move"
                    onCheckedChanged: if (checked) operation = "move"
                    
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("不占用额外空间，原文件被移除。")
                }
            }
        }
        
        // 预览
        Label {
            Layout.fillWidth: true
            text: {
                var fileName = sourceFile.split("/").pop()
                var op = ""
                switch (operation) {
                    case "link": op = "ln"; break
                    case "copy": op = "cp"; break
                    case "move": op = "mv"; break
                }
                return qsTr("命令预览: ") + op + " \"" + sourceFile + "\" \"" + targetDirField.text + "/" + fileName + "\""
            }
            color: "#7f849c"
            font.family: "monospace"
            wrapMode: Text.WrapAnywhere
        }
    }
    
    FolderDialog {
        id: folderDialog
        title: qsTr("选择目标目录")
        onAccepted: {
            targetDirField.text = selectedFolder.toString().replace("file://", "")
        }
    }
    
    onAccepted: {
        operationConfirmed(sourceFile, targetDirField.text, operation)
    }
}
