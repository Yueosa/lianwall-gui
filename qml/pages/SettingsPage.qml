import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".." as App

/// Settings 设置页
/// 壁纸路径 + 界面设置
Item {
    id: settingsRoot

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        Flickable {
            contentHeight: mainCol.height + App.Theme.spacingLarge * 2

            ColumnLayout {
                id: mainCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: App.Theme.spacingLarge
                spacing: App.Theme.spacingLarge

                // 页面标题
                Text {
                    text: qsTr("⚙️ 设置")
                    font.pixelSize: 24
                    font.bold: true
                    color: App.Theme.text
                }

                // 未连接提示
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    radius: App.Theme.radiusMedium
                    color: Qt.rgba(App.Theme.warning.r, App.Theme.warning.g, App.Theme.warning.b, 0.15)
                    border.width: 1
                    border.color: App.Theme.warning
                    visible: !DaemonState.daemonConnected

                    Text {
                        anchors.centerIn: parent
                        text: qsTr("⚠️ 守护进程未连接，配置修改暂不可用")
                        font.pixelSize: App.Theme.fontSizeMedium
                        color: App.Theme.warning
                    }
                }

                // 未加载提示
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    radius: App.Theme.radiusMedium
                    color: App.Theme.surface
                    visible: DaemonState.daemonConnected && !ConfigManager.configLoaded

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: App.Theme.spacingSmall
                        BusyIndicator { width: 20; height: 20; running: true }
                        Text {
                            text: qsTr("正在加载配置...")
                            font.pixelSize: App.Theme.fontSizeMedium
                            color: App.Theme.textSecondary
                        }
                    }
                }

                // ============================================================
                // 分区 1：壁纸路径
                // ============================================================
                ConfigSection {
                    title: qsTr("📂 壁纸路径")

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: App.Theme.spacingMedium

                        // 视频壁纸目录
                        ConfigRow {
                            label: qsTr("视频目录")

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: App.Theme.spacingSmall

                                Text {
                                    Layout.fillWidth: true
                                    text: ConfigManager.videoDir || "-"
                                    font.pixelSize: App.Theme.fontSizeSmall
                                    font.family: "monospace"
                                    color: App.Theme.text
                                    elide: Text.ElideMiddle
                                }

                                SmallButton {
                                    text: qsTr("📂 打开")
                                    onClicked: ConfigManager.openDirectory(ConfigManager.videoDir)
                                }
                            }
                        }

                        // 图片壁纸目录
                        ConfigRow {
                            label: qsTr("图片目录")

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: App.Theme.spacingSmall

                                Text {
                                    Layout.fillWidth: true
                                    text: ConfigManager.imageDir || "-"
                                    font.pixelSize: App.Theme.fontSizeSmall
                                    font.family: "monospace"
                                    color: App.Theme.text
                                    elide: Text.ElideMiddle
                                }

                                SmallButton {
                                    text: qsTr("📂 打开")
                                    onClicked: ConfigManager.openDirectory(ConfigManager.imageDir)
                                }
                            }
                        }
                    }
                }

                // ============================================================
                // 分区 2：界面设置
                // ============================================================
                ConfigSection {
                    title: qsTr("🎨 界面设置")

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: App.Theme.spacingMedium

                        // 主题
                        ConfigRow {
                            label: qsTr("主题")

                            RowLayout {
                                spacing: App.Theme.spacingSmall

                                ModeChip {
                                    text: "☀️ " + qsTr("浅色")
                                    selected: ConfigManager.theme === "lian"
                                    onClicked: ConfigManager.setTheme("lian")
                                }
                                ModeChip {
                                    text: "🌙 " + qsTr("深色")
                                    selected: ConfigManager.theme === "lian-dark"
                                    onClicked: ConfigManager.setTheme("lian-dark")
                                }
                            }
                        }

                        // 语言
                        ConfigRow {
                            label: qsTr("语言")

                            ComboBox {
                                model: [
                                    { text: "中文", value: "zh_CN" },
                                    { text: "English", value: "en" }
                                ]
                                textRole: "text"
                                valueRole: "value"
                                currentIndex: ConfigManager.language === "en" ? 1 : 0
                                onActivated: {
                                    ConfigManager.setLanguage(model[currentIndex].value)
                                    LianwallApp.switchLanguage(model[currentIndex].value)
                                }

                                background: Rectangle {
                                    radius: App.Theme.radiusSmall
                                    color: App.Theme.surface
                                    border.width: 1
                                    border.color: App.Theme.border
                                    implicitWidth: 140
                                    implicitHeight: 32
                                }
                            }
                        }
                    }
                }

                // 底部间距
                Item { Layout.preferredHeight: App.Theme.spacingLarge }
            }
        }
    }

    // ========================================================================
    // 内联组件
    // ========================================================================

    /// 配置分区卡片
    component ConfigSection: Rectangle {
        property string title: ""
        default property alias content: sectionContent.data

        Layout.fillWidth: true
        implicitHeight: sectionCol.height + App.Theme.spacingMedium * 2
        radius: App.Theme.radiusLarge
        color: App.Theme.card
        border.width: 1
        border.color: App.Theme.border

        ColumnLayout {
            id: sectionCol
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: App.Theme.spacingMedium
            spacing: App.Theme.spacingMedium

            Text {
                text: title
                font.pixelSize: App.Theme.fontSizeLarge
                font.bold: true
                color: App.Theme.text
            }

            // 分隔线
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: App.Theme.divider
            }

            ColumnLayout {
                id: sectionContent
                Layout.fillWidth: true
                spacing: App.Theme.spacingMedium
            }
        }
    }

    /// 配置行：左标签 + 右控件
    component ConfigRow: RowLayout {
        property string label: ""
        property string hint: ""

        Layout.fillWidth: true
        spacing: App.Theme.spacingMedium

        ColumnLayout {
            Layout.preferredWidth: 130
            spacing: 2

            Text {
                text: label
                font.pixelSize: App.Theme.fontSizeMedium
                color: App.Theme.text
            }
            Text {
                text: hint
                font.pixelSize: App.Theme.fontSizeSmall
                color: App.Theme.textSecondary
                visible: hint.length > 0
            }
        }
    }

    /// 模式选择标签
    component ModeChip: Rectangle {
        property string text: ""
        property bool selected: false
        signal clicked()

        width: chipLabel.width + App.Theme.spacingSmall * 2
        height: 32
        radius: App.Theme.radiusMedium
        color: selected ? App.Theme.accent
               : chipMouse.containsMouse ? App.Theme.cardHover : "transparent"
        border.width: selected ? 0 : 1
        border.color: App.Theme.border

        Behavior on color { ColorAnimation { duration: 150 } }

        Text {
            id: chipLabel
            anchors.centerIn: parent
            text: parent.text
            font.pixelSize: App.Theme.fontSizeSmall
            color: parent.selected ? App.Theme.textOnAccent : App.Theme.text
        }

        MouseArea {
            id: chipMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
        }
    }

    /// 小按钮
    component SmallButton: Rectangle {
        property string text: ""
        signal clicked()

        width: btnLabel.width + App.Theme.spacingSmall * 2
        height: 28
        radius: App.Theme.radiusSmall
        color: btnMouse.pressed ? App.Theme.cardHover
               : btnMouse.containsMouse ? App.Theme.surface : "transparent"
        border.width: 1
        border.color: App.Theme.border

        Text {
            id: btnLabel
            anchors.centerIn: parent
            text: parent.text
            font.pixelSize: App.Theme.fontSizeSmall
            color: App.Theme.text
        }

        MouseArea {
            id: btnMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
        }
    }
}
