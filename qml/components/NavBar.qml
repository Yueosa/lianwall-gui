import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".." as App

/// 左侧导航栏
/// 图标按钮竖排，底部显示 daemon 连接状态
Item {
    id: navRoot
    implicitWidth: 56

    property int currentIndex: 0
    property bool daemonConnected: false

    signal navigated(int index)

    // 页面定义
    readonly property var pages: [
        { icon: "🏠", label: qsTr("仪表盘") },
        { icon: "📚", label: qsTr("壁纸库") },
        { icon: "⚙️", label: qsTr("设置") },
        { icon: "ℹ️", label: qsTr("关于") }
    ]

    Rectangle {
        anchors.fill: parent
        color: App.Theme.navBar

        ColumnLayout {
            anchors.fill: parent
            anchors.topMargin: App.Theme.spacingMedium
            anchors.bottomMargin: App.Theme.spacingMedium
            spacing: App.Theme.spacingTiny

            // 导航按钮
            Repeater {
                model: navRoot.pages

                delegate: Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 48

                    readonly property bool isSelected: navRoot.currentIndex === index

                    Rectangle {
                        anchors.fill: parent
                        anchors.leftMargin: 6
                        anchors.rightMargin: 6
                        radius: App.Theme.radiusMedium
                        color: isSelected ? App.Theme.navBarSelected
                             : navMouseArea.containsMouse ? App.Theme.cardHover
                             : "transparent"

                        // 选中指示条
                        Rectangle {
                            visible: isSelected
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            width: 3
                            height: 20
                            radius: 2
                            color: App.Theme.accent
                        }

                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: 2

                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: modelData.icon
                                font.pixelSize: 20
                            }

                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: modelData.label
                                font.pixelSize: 9
                                color: isSelected ? App.Theme.accent : App.Theme.textSecondary
                            }
                        }

                        MouseArea {
                            id: navMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: navRoot.navigated(index)
                        }
                    }
                }
            }

            // 弹性空间
            Item { Layout.fillHeight: true }

            // 主题切换按钮
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 40

                Rectangle {
                    anchors.fill: parent
                    anchors.leftMargin: 6
                    anchors.rightMargin: 6
                    radius: App.Theme.radiusMedium
                    color: themeMouseArea.containsMouse ? App.Theme.cardHover : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: App.Theme.isDark ? "🌙" : "☀️"
                        font.pixelSize: 18
                    }

                    MouseArea {
                        id: themeMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            ConfigManager.setTheme(App.Theme.isDark ? "lian" : "lian-dark")
                        }
                    }
                }
            }

            // 连接状态指示
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 30

                Row {
                    anchors.centerIn: parent
                    spacing: 6

                    Rectangle {
                        width: 8; height: 8
                        radius: 4
                        anchors.verticalCenter: parent.verticalCenter
                        color: navRoot.daemonConnected ? App.Theme.success : App.Theme.error

                        // 连接时脉冲动画
                        SequentialAnimation on opacity {
                            running: !navRoot.daemonConnected
                            loops: Animation.Infinite
                            NumberAnimation { to: 0.3; duration: 800; easing.type: Easing.InOutQuad }
                            NumberAnimation { to: 1.0; duration: 800; easing.type: Easing.InOutQuad }
                        }
                    }

                    Text {
                        text: navRoot.daemonConnected ? "ON" : "OFF"
                        font.pixelSize: 9
                        font.bold: true
                        color: App.Theme.textSecondary
                    }
                }
            }
        }
    }
}
