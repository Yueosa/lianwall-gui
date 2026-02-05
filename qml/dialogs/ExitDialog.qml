import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root
    
    title: qsTr("退出确认")
    modal: true
    standardButtons: Dialog.NoButton
    
    signal minimizeRequested()
    signal quitRequested()
    signal shutdownRequested()
    
    property bool rememberChoice: false
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 16
        
        Label {
            text: qsTr("您想要如何退出？")
            font.pixelSize: 14
        }
        
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8
            
            Button {
                Layout.fillWidth: true
                text: qsTr("最小化到托盘")
                icon.name: "window-minimize"
                onClicked: {
                    if (rememberChoice) {
                        ConfigManager.exitBehavior = "minimize"
                    }
                    root.minimizeRequested()
                    root.close()
                }
            }
            
            Button {
                Layout.fillWidth: true
                text: qsTr("退出 GUI（守护进程继续运行）")
                icon.name: "application-exit"
                onClicked: {
                    if (rememberChoice) {
                        ConfigManager.exitBehavior = "quit"
                    }
                    root.quitRequested()
                    root.close()
                }
            }
            
            Button {
                Layout.fillWidth: true
                text: qsTr("关闭守护进程并退出")
                icon.name: "system-shutdown"
                highlighted: true
                onClicked: {
                    if (rememberChoice) {
                        ConfigManager.exitBehavior = "shutdown"
                    }
                    root.shutdownRequested()
                    root.close()
                }
            }
        }
        
        CheckBox {
            id: rememberCheckbox
            text: qsTr("记住我的选择")
            checked: root.rememberChoice
            onCheckedChanged: root.rememberChoice = checked
        }
        
        Button {
            Layout.alignment: Qt.AlignRight
            text: qsTr("取消")
            flat: true
            onClicked: root.close()
        }
    }
}
