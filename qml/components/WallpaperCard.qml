import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    radius: 8
    color: isCurrent ? "#3d59a1" : (mouseArea.containsMouse ? "#24283b" : "#1f2335")
    border.color: locked ? "#f7768e" : (inCooldown ? "#7aa2f7" : "transparent")
    border.width: 2

    property string filename: ""
    property string thumbnail: ""
    property bool locked: false
    property bool inCooldown: false
    property bool isCurrent: false

    signal clicked()
    signal lockToggled()

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onClicked: function(mouse) {
            if (mouse.button === Qt.LeftButton) {
                root.clicked()
            } else if (mouse.button === Qt.RightButton) {
                contextMenu.popup()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 5
        spacing: 5

        // 缩略图
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#24283b"
            radius: 4
            clip: true

            Image {
                anchors.fill: parent
                source: root.thumbnail ? "file://" + root.thumbnail : ""
                fillMode: Image.PreserveAspectCrop
                visible: root.thumbnail !== ""
            }

            // 无缩略图时显示占位
            Text {
                anchors.centerIn: parent
                text: "🎬"
                font.pixelSize: 32
                color: "#565f89"
                visible: root.thumbnail === ""
            }

            // 状态图标
            Row {
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 5
                spacing: 3

                Rectangle {
                    width: 20
                    height: 20
                    radius: 10
                    color: "#f7768e"
                    visible: root.locked

                    Text {
                        anchors.centerIn: parent
                        text: "🔒"
                        font.pixelSize: 10
                    }
                }

                Rectangle {
                    width: 20
                    height: 20
                    radius: 10
                    color: "#7aa2f7"
                    visible: root.inCooldown

                    Text {
                        anchors.centerIn: parent
                        text: "❄"
                        font.pixelSize: 10
                    }
                }
            }

            // 当前壁纸指示
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.margins: 5
                width: 25
                height: 25
                radius: 12
                color: "#9ece6a"
                visible: root.isCurrent

                Text {
                    anchors.centerIn: parent
                    text: "▶"
                    font.pixelSize: 12
                    color: "#1a1b26"
                }
            }
        }

        // 文件名
        Text {
            Layout.fillWidth: true
            text: root.filename
            font.pixelSize: 11
            color: "#a9b1d6"
            elide: Text.ElideMiddle
            horizontalAlignment: Text.AlignHCenter
        }
    }

    // 右键菜单
    Menu {
        id: contextMenu

        MenuItem {
            text: root.locked ? qsTr("Unlock") : qsTr("Lock")
            onTriggered: root.lockToggled()
        }

        MenuItem {
            text: qsTr("Set as wallpaper")
            onTriggered: root.clicked()
        }

        MenuSeparator {}

        MenuItem {
            text: qsTr("Open in file manager")
            onTriggered: {
                // TODO: 打开文件管理器
            }
        }
    }
}
