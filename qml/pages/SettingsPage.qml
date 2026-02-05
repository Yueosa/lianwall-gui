import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import LianwallGui 1.0

Page {
    id: root
    
    property var configData: ({})
    
    ScrollView {
        anchors.fill: parent
        anchors.margins: 20
        contentWidth: availableWidth
        
        ColumnLayout {
            width: parent.width
            spacing: 24
            
            // === 应用设置 ===
            GroupBox {
                Layout.fillWidth: true
                title: qsTr("应用设置")
                
                GridLayout {
                    columns: 2
                    rowSpacing: 12
                    columnSpacing: 16
                    width: parent.width
                    
                    Label { text: qsTr("语言") }
                    ComboBox {
                        id: languageCombo
                        model: [
                            { text: "English", value: "en" },
                            { text: "简体中文", value: "zh_CN" }
                        ]
                        textRole: "text"
                        valueRole: "value"
                        currentIndex: ConfigManager.language === "zh_CN" ? 1 : 0
                        onCurrentValueChanged: ConfigManager.language = currentValue
                    }
                    
                    Label { text: qsTr("退出行为") }
                    ComboBox {
                        id: exitBehaviorCombo
                        model: [
                            { text: qsTr("每次询问"), value: "ask" },
                            { text: qsTr("最小化到托盘"), value: "minimize" },
                            { text: qsTr("退出程序"), value: "quit" },
                            { text: qsTr("关闭守护进程"), value: "shutdown" }
                        ]
                        textRole: "text"
                        valueRole: "value"
                        currentIndex: {
                            switch(ConfigManager.exitBehavior) {
                                case "minimize": return 1
                                case "quit": return 2
                                case "shutdown": return 3
                                default: return 0
                            }
                        }
                        onCurrentValueChanged: ConfigManager.exitBehavior = currentValue
                    }
                    
                    Label { text: qsTr("开机自启") }
                    Switch {
                        id: autostartSwitch
                        checked: ConfigManager.autostartEnabled
                        onToggled: ConfigManager.autostartEnabled = checked
                    }
                }
            }
            
            // === lianwall 配置 ===
            GroupBox {
                Layout.fillWidth: true
                title: qsTr("lianwall 配置")
                
                ColumnLayout {
                    width: parent.width
                    spacing: 16
                    
                    // 壁纸目录
                    RowLayout {
                        Layout.fillWidth: true
                        Label { 
                            text: qsTr("壁纸目录")
                            Layout.preferredWidth: 100
                        }
                        TextField {
                            id: videoDirField
                            Layout.fillWidth: true
                            text: configData.video_dir || ""
                            placeholderText: "~/Videos/Wallpapers"
                        }
                        Button {
                            text: qsTr("浏览")
                            onClicked: folderDialog.open()
                        }
                    }
                    
                    // 模式
                    RowLayout {
                        Layout.fillWidth: true
                        Label { 
                            text: qsTr("默认模式")
                            Layout.preferredWidth: 100
                        }
                        ComboBox {
                            id: modeCombo
                            Layout.fillWidth: true
                            model: ["sunflower", "random", "sequential"]
                            currentIndex: {
                                var mode = configData.mode || "sunflower"
                                return model.indexOf(mode)
                            }
                        }
                    }
                    
                    // 间隔
                    RowLayout {
                        Layout.fillWidth: true
                        Label { 
                            text: qsTr("切换间隔")
                            Layout.preferredWidth: 100
                        }
                        SpinBox {
                            id: intervalSpin
                            from: 10
                            to: 86400
                            value: configData.interval || 300
                            editable: true
                        }
                        Label { text: qsTr("秒") }
                    }
                    
                    // 冷却时间
                    RowLayout {
                        Layout.fillWidth: true
                        Label { 
                            text: qsTr("冷却时间")
                            Layout.preferredWidth: 100
                        }
                        SpinBox {
                            id: cooldownSpin
                            from: 0
                            to: 86400
                            value: configData.cooldown || 600
                            editable: true
                        }
                        Label { text: qsTr("秒") }
                    }
                    
                    // VRAM 限制
                    RowLayout {
                        Layout.fillWidth: true
                        Label { 
                            text: qsTr("VRAM 限制")
                            Layout.preferredWidth: 100
                        }
                        SpinBox {
                            id: vramLimitSpin
                            from: 0
                            to: 65536
                            value: configData.vram_limit_mb || 0
                            editable: true
                        }
                        Label { text: qsTr("MB (0 = 无限制)") }
                    }
                    
                    // 引擎
                    RowLayout {
                        Layout.fillWidth: true
                        Label { 
                            text: qsTr("渲染引擎")
                            Layout.preferredWidth: 100
                        }
                        ComboBox {
                            id: engineCombo
                            Layout.fillWidth: true
                            model: ["auto", "mpvpaper", "swww"]
                            currentIndex: {
                                var engine = configData.engine || "auto"
                                return model.indexOf(engine)
                            }
                        }
                    }
                    
                    // 按钮行
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: 8
                        
                        Item { Layout.fillWidth: true }
                        
                        Button {
                            text: qsTr("重置为默认")
                            onClicked: {
                                ConfigManager.resetConfig()
                                loadConfig()
                            }
                        }
                        
                        Button {
                            text: qsTr("保存配置")
                            highlighted: true
                            onClicked: saveConfig()
                        }
                        
                        Button {
                            text: qsTr("重载配置")
                            onClicked: {
                                Client.reload()
                                showToast(qsTr("已发送重载命令"))
                            }
                        }
                    }
                }
            }
            
            // === 高级设置 ===
            GroupBox {
                Layout.fillWidth: true
                title: qsTr("高级设置")
                
                ColumnLayout {
                    width: parent.width
                    spacing: 12
                    
                    CheckBox {
                        id: debugCheck
                        text: qsTr("启用调试日志")
                        checked: configData.debug || false
                    }
                    
                    RowLayout {
                        Layout.fillWidth: true
                        Label { 
                            text: qsTr("Socket 路径")
                            Layout.preferredWidth: 100
                        }
                        TextField {
                            id: socketPathField
                            Layout.fillWidth: true
                            text: configData.socket_path || "/tmp/lianwall.sock"
                            placeholderText: "/tmp/lianwall.sock"
                        }
                    }
                    
                    RowLayout {
                        Layout.fillWidth: true
                        Label { 
                            text: qsTr("权重文件")
                            Layout.preferredWidth: 100
                        }
                        TextField {
                            id: weightFileField
                            Layout.fillWidth: true
                            text: configData.weight_file || ""
                            placeholderText: "~/.cache/lianwall/weights.json"
                        }
                    }
                }
            }
            
            Item { Layout.fillHeight: true }
        }
    }
    
    // 文件夹选择对话框
    FolderDialog {
        id: folderDialog
        title: qsTr("选择壁纸目录")
        onAccepted: {
            videoDirField.text = selectedFolder.toString().replace("file://", "")
        }
    }
    
    function loadConfig() {
        var result = ConfigManager.show()
        try {
            configData = JSON.parse(result)
        } catch (e) {
            console.error("Failed to parse config:", e)
        }
    }
    
    function saveConfig() {
        ConfigManager.set("video_dir", videoDirField.text)
        ConfigManager.set("mode", modeCombo.currentText)
        ConfigManager.set("interval", intervalSpin.value.toString())
        ConfigManager.set("cooldown", cooldownSpin.value.toString())
        ConfigManager.set("vram_limit_mb", vramLimitSpin.value.toString())
        ConfigManager.set("engine", engineCombo.currentText)
        ConfigManager.set("debug", debugCheck.checked ? "true" : "false")
        ConfigManager.set("socket_path", socketPathField.text)
        ConfigManager.set("weight_file", weightFileField.text)
        
        showToast(qsTr("配置已保存"))
    }
    
    function showToast(message) {
        // TODO: 实现 toast 提示
        console.log("Toast:", message)
    }
    
    Component.onCompleted: loadConfig()
}
