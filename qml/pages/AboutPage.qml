import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LianwallGui 1.0

Page {
    id: root
    
    ScrollView {
        anchors.fill: parent
        anchors.margins: 20
        contentWidth: availableWidth
        
        ColumnLayout {
            width: parent.width
            spacing: 24
            
            // Logo 和标题
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 150
                Layout.alignment: Qt.AlignHCenter
                
                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 16
                    
                    Image {
                        source: "qrc:/icons/lianwall.svg"
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 80
                        Layout.preferredHeight: 80
                        fillMode: Image.PreserveAspectFit
                    }
                    
                    Label {
                        text: "Lianwall GUI"
                        font.pixelSize: 24
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter
                    }
                    
                    Label {
                        text: qsTr("Wayland 动态壁纸管理器")
                        color: "#7f849c"
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }
            
            // 版本信息
            GroupBox {
                Layout.fillWidth: true
                title: qsTr("版本信息")
                
                GridLayout {
                    columns: 2
                    rowSpacing: 8
                    columnSpacing: 16
                    width: parent.width
                    
                    Label { text: qsTr("GUI 版本"); color: "#7f849c" }
                    Label { text: Qt.application.version || "1.0.0" }
                    
                    Label { text: qsTr("守护进程版本"); color: "#7f849c" }
                    Label { 
                        id: daemonVersionLabel
                        text: "-"
                    }
                    
                    Label { text: qsTr("Qt 版本"); color: "#7f849c" }
                    Label { text: "6.x" }
                    
                    Label { text: qsTr("构建日期"); color: "#7f849c" }
                    Label { text: AppVersion }
                }
            }
            
            // 连接状态
            GroupBox {
                Layout.fillWidth: true
                title: qsTr("连接状态")
                
                GridLayout {
                    columns: 2
                    rowSpacing: 8
                    columnSpacing: 16
                    width: parent.width
                    
                    Label { text: qsTr("Socket"); color: "#7f849c" }
                    RowLayout {
                        Rectangle {
                            width: 10
                            height: 10
                            radius: 5
                            color: Client.connected ? "#a6e3a1" : "#f38ba8"
                        }
                        Label { 
                            text: Client.connected ? qsTr("已连接") : qsTr("未连接")
                        }
                    }
                    
                    Label { text: qsTr("守护进程"); color: "#7f849c" }
                    RowLayout {
                        Rectangle {
                            width: 10
                            height: 10
                            radius: 5
                            color: daemonRunning ? "#a6e3a1" : "#f38ba8"
                        }
                        Label { 
                            text: daemonRunning ? qsTr("运行中") : qsTr("未运行")
                        }
                    }
                    
                    Label { text: qsTr("运行时间"); color: "#7f849c" }
                    Label { 
                        id: uptimeLabel
                        text: "-"
                    }
                }
            }
            
            // 链接
            GroupBox {
                Layout.fillWidth: true
                title: qsTr("链接")
                
                ColumnLayout {
                    width: parent.width
                    spacing: 8
                    
                    Button {
                        text: "GitHub - lianwall"
                        flat: true
                        icon.name: "internet-web-browser"
                        onClicked: Qt.openUrlExternally("https://github.com/Yueosa/lianwall")
                    }
                    
                    Button {
                        text: "GitHub - lianwall-gui"
                        flat: true
                        icon.name: "internet-web-browser"
                        onClicked: Qt.openUrlExternally("https://github.com/Yueosa/lianwall-gui")
                    }
                    
                    Button {
                        text: qsTr("报告问题")
                        flat: true
                        icon.name: "dialog-warning"
                        onClicked: Qt.openUrlExternally("https://github.com/Yueosa/lianwall-gui/issues")
                    }
                }
            }
            
            // 作者信息
            GroupBox {
                Layout.fillWidth: true
                title: qsTr("作者")
                
                RowLayout {
                    width: parent.width
                    spacing: 16
                    
                    // Avatar placeholder
                    Rectangle {
                        width: 48
                        height: 48
                        radius: 24
                        color: "#45475a"
                        
                        Label {
                            anchors.centerIn: parent
                            text: "L"
                            font.pixelSize: 20
                            font.bold: true
                        }
                    }
                    
                    ColumnLayout {
                        spacing: 4
                        
                        Label {
                            text: "Lian"
                            font.bold: true
                        }
                        
                        Label {
                            text: "Yueosa"
                            color: "#7f849c"
                        }
                    }
                    
                    Item { Layout.fillWidth: true }
                    
                    Button {
                        text: "GitHub"
                        onClicked: Qt.openUrlExternally("https://github.com/Yueosa")
                    }
                }
            }
            
            // 许可证
            GroupBox {
                Layout.fillWidth: true
                title: qsTr("许可证")
                
                Label {
                    width: parent.width
                    text: "MIT License\n\n" +
                          "Copyright (c) 2024 Lian\n\n" +
                          qsTr("本软件按 \"原样\" 提供，不附带任何形式的明示或暗示担保。")
                    wrapMode: Text.WordWrap
                    color: "#7f849c"
                }
            }
            
            Item { Layout.fillHeight: true }
        }
    }
    
    property bool daemonRunning: false
    
    Connections {
        target: Client
        
        function onStatusReceived(status) {
            if (status.uptime_secs !== undefined) {
                var secs = status.uptime_secs
                var hours = Math.floor(secs / 3600)
                var mins = Math.floor((secs % 3600) / 60)
                uptimeLabel.text = hours + qsTr("小时") + " " + mins + qsTr("分钟")
            }
        }
        
        function onConnectedChanged() {
            daemonRunning = Client.connected
        }
    }
    
    Component.onCompleted: {
        daemonRunning = Client.connected
        if (Client.connected) {
            Client.getStatus()
        }
    }
}
