import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LianwallGui 1.0

Rectangle {
    id: root
    height: 50
    color: "#1a1b26"

    property int currentIndex: 0
    signal pageSelected(int index)

    // 页面定义
    readonly property var pages: [
        { icon: "🏠", label: qsTr("Home") },
        { icon: "📁", label: qsTr("Library") },
        { icon: "📅", label: qsTr("Timeline") },
        { icon: "📊", label: qsTr("Status") },
        { icon: "⚙", label: qsTr("Settings") },
        { icon: "ℹ", label: qsTr("About") }
    ]

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        spacing: 5

        // Logo
        Text {
            text: "🌻 LianWall"
            font.pixelSize: 18
            font.bold: true
            color: "#bb9af7"
        }

        Item { Layout.preferredWidth: 30 }

        // 导航按钮
        Repeater {
            model: root.pages

            Button {
                Layout.preferredWidth: 100
                Layout.preferredHeight: 40
                
                flat: true
                highlighted: root.currentIndex === index

                contentItem: RowLayout {
                    spacing: 6
                    Text {
                        text: modelData.icon
                        font.pixelSize: 16
                    }
                    Text {
                        text: modelData.label
                        font.pixelSize: 14
                        color: root.currentIndex === index ? "#7aa2f7" : "#a9b1d6"
                    }
                }

                background: Rectangle {
                    color: root.currentIndex === index ? "#24283b" : "transparent"
                    radius: 6
                }

                onClicked: root.pageSelected(index)
            }
        }

        Item { Layout.fillWidth: true }

        // 连接状态指示
        Rectangle {
            width: 10
            height: 10
            radius: 5
            color: Client.connected ? "#9ece6a" : "#f7768e"

            ToolTip.visible: mouseArea.containsMouse
            ToolTip.text: Client.connected ? qsTr("Connected") : qsTr("Disconnected")

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
            }
        }
    }

    // 底部分隔线
    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: "#414868"
    }
}
