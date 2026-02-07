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

                        // 配置文件目录
                        ConfigRow {
                            label: qsTr("配置目录")

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: App.Theme.spacingSmall

                                Text {
                                    Layout.fillWidth: true
                                    text: ConfigManager.configDirPath()
                                    font.pixelSize: App.Theme.fontSizeSmall
                                    font.family: "monospace"
                                    color: App.Theme.text
                                    elide: Text.ElideMiddle
                                }

                                SmallButton {
                                    text: qsTr("📂 打开")
                                    onClicked: ConfigManager.openDirectory(ConfigManager.configDirPath())
                                }
                            }
                        }

                        // 缓存目录
                        ConfigRow {
                            label: qsTr("缓存目录")

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: App.Theme.spacingSmall

                                Text {
                                    Layout.fillWidth: true
                                    text: ConfigManager.cacheDirPath()
                                    font.pixelSize: App.Theme.fontSizeSmall
                                    font.family: "monospace"
                                    color: App.Theme.text
                                    elide: Text.ElideMiddle
                                }

                                SmallButton {
                                    text: qsTr("📂 打开")
                                    onClicked: ConfigManager.openDirectory(ConfigManager.cacheDirPath())
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

                        // 主色调
                        ConfigRow {
                            label: qsTr("主色调")

                            RowLayout {
                                spacing: App.Theme.spacingSmall

                                ColorChip {
                                    chipColor: "#5BCEFA"
                                    label: qsTr("MTF蓝")
                                    selected: ConfigManager.accentColor === "blue"
                                    onClicked: ConfigManager.setAccentColor("blue")
                                }
                                ColorChip {
                                    chipColor: "#F5A9B8"
                                    label: qsTr("MTF粉")
                                    selected: ConfigManager.accentColor === "pink"
                                    onClicked: ConfigManager.setAccentColor("pink")
                                }
                            }
                        }

                        // 语言
                        ConfigRow {
                            label: qsTr("语言")

                            StyledSelect {
                                id: langSelect
                                selectWidth: 140
                                model: [
                                    { text: "中文", value: "zh_CN" },
                                    { text: "English", value: "en" }
                                ]
                                currentIndex: ConfigManager.language === "en" ? 1 : 0
                                onSelected: function(index) {
                                    ConfigManager.setLanguage(model[index].value)
                                    LianwallApp.switchLanguage(model[index].value)
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

    /// 色彩选择标签（带色点）
    component ColorChip: Rectangle {
        property color chipColor: "#5BCEFA"
        property string label: ""
        property bool selected: false
        signal clicked()

        width: colorChipRow.width + App.Theme.spacingSmall * 2
        height: 32
        radius: App.Theme.radiusMedium
        color: selected ? Qt.rgba(chipColor.r, chipColor.g, chipColor.b, 0.25)
               : colorChipMouse.containsMouse ? App.Theme.cardHover : "transparent"
        border.width: selected ? 2 : 1
        border.color: selected ? chipColor : App.Theme.border

        Behavior on color { ColorAnimation { duration: 150 } }
        Behavior on border.color { ColorAnimation { duration: 150 } }

        Row {
            id: colorChipRow
            anchors.centerIn: parent
            spacing: 6

            Rectangle {
                width: 14; height: 14
                radius: 7
                color: chipColor
                anchors.verticalCenter: parent.verticalCenter
                border.width: selected ? 2 : 0
                border.color: Qt.darker(chipColor, 1.3)
            }

            Text {
                text: label
                font.pixelSize: App.Theme.fontSizeSmall
                color: selected ? chipColor : App.Theme.text
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        MouseArea {
            id: colorChipMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
        }
    }

    /// 自定义下拉选择器（替代默认 ComboBox）
    component StyledSelect: Item {
        id: selectRoot
        property int selectWidth: 140
        property var model: []
        property int currentIndex: 0
        signal selected(int index)

        width: selectWidth
        height: 32

        // 当前选中项按钮
        Rectangle {
            id: selectButton
            anchors.fill: parent
            radius: App.Theme.radiusMedium
            color: selectMouse.pressed ? App.Theme.cardHover
                   : selectMouse.containsMouse ? App.Theme.surface : App.Theme.input
            border.width: 1
            border.color: selectPopup.visible ? App.Theme.accent : App.Theme.border

            Behavior on border.color { ColorAnimation { duration: 150 } }

            Row {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 8
                spacing: 4

                Text {
                    width: parent.width - dropdownArrow.width - 4
                    anchors.verticalCenter: parent.verticalCenter
                    text: selectRoot.model[selectRoot.currentIndex]?.text ?? ""
                    font.pixelSize: App.Theme.fontSizeSmall
                    color: App.Theme.text
                    elide: Text.ElideRight
                }

                Text {
                    id: dropdownArrow
                    anchors.verticalCenter: parent.verticalCenter
                    text: selectPopup.visible ? "▲" : "▼"
                    font.pixelSize: 8
                    color: App.Theme.textSecondary
                }
            }

            MouseArea {
                id: selectMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: selectPopup.visible = !selectPopup.visible
            }
        }

        // 下拉面板（使用 Popup 自动处理点击外部关闭）
        Popup {
            id: selectPopup
            y: selectButton.height + 4
            width: selectRoot.selectWidth
            padding: 4
            closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

            background: Rectangle {
                radius: App.Theme.radiusMedium
                color: App.Theme.popup
                border.width: 1
                border.color: App.Theme.border
            }

            contentItem: Column {
                id: dropdownCol
                spacing: 0

                Repeater {
                    model: selectRoot.model

                    Rectangle {
                        width: selectRoot.selectWidth - 8
                        height: 32
                        radius: App.Theme.radiusSmall
                        color: index === selectRoot.currentIndex ? App.Theme.accent
                               : optMouse.containsMouse ? App.Theme.cardHover : "transparent"

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 8
                            text: modelData.text
                            font.pixelSize: App.Theme.fontSizeSmall
                            color: index === selectRoot.currentIndex ? App.Theme.textOnAccent
                                   : App.Theme.text
                        }

                        MouseArea {
                            id: optMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                selectRoot.currentIndex = index
                                selectRoot.selected(index)
                                selectPopup.close()
                            }
                        }
                    }
                }
            }
        }
    }
}
