import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LianwallGui 1.0
import "." as App

ApplicationWindow {
    id: root
    width: 800
    height: 600
    minimumWidth: 600
    minimumHeight: 400
    visible: true
    title: qsTr("LianWall")
    
    color: App.Theme.background

    // 初始化主题
    Component.onCompleted: {
        App.Theme.current = ConfigManager.theme
    }
    
    // 监听主题变化
    Connections {
        target: ConfigManager
        function onThemeChanged(newTheme) {
            App.Theme.current = newTheme
        }
    }

    // 占位内容
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16
        
        // 标题
        Text {
            text: "LianWall GUI"
            font.pixelSize: 28
            font.bold: true
            color: App.Theme.text
        }
        
        Text {
            text: qsTr("Clean slate - ready for your ideas")
            font.pixelSize: 16
            color: App.Theme.textSecondary
        }
        
        // 主题切换测试
        RowLayout {
            spacing: 12
            
            Text {
                text: qsTr("Theme:")
                color: App.Theme.text
            }
            
            Button {
                text: "Lian (Light)"
                onClicked: ConfigManager.setTheme("lian")
            }
            
            Button {
                text: "Lian-Dark"
                onClicked: ConfigManager.setTheme("lian-dark")
            }
        }
        
        // 填充
        Item { Layout.fillHeight: true }
        
        // 版本信息
        Text {
            text: "v" + AppVersion + " by " + AppAuthor
            font.pixelSize: 12
            color: App.Theme.textSecondary
        }
    }
}
