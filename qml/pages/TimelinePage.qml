import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LianwallGui 1.0

Page {
    id: root
    
    required property TimelineModel timelineModel

    background: Rectangle { color: "#1a1b26" }

    // 文件操作模式
    property string fileOperation: "link"  // "link", "copy", "move"

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        // 左侧：壁纸列表
        Rectangle {
            SplitView.preferredWidth: 250
            SplitView.minimumWidth: 200
            color: "#1f2335"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                Text {
                    text: qsTr("Wallpapers")
                    font.pixelSize: 16
                    font.bold: true
                    color: "#a9b1d6"
                }

                // 模式切换
                RowLayout {
                    Layout.fillWidth: true
                    
                    Button {
                        text: "Video"
                        flat: true
                        highlighted: timelineModel.currentMode === "video"
                        onClicked: timelineModel.currentMode = "video"
                    }
                    Button {
                        text: "Image"
                        flat: true
                        highlighted: timelineModel.currentMode === "image"
                        onClicked: timelineModel.currentMode = "image"
                    }
                }

                // 统计信息
                Text {
                    text: qsTr("Scanned: %1 | Active: %2").arg(timelineModel.scannedCount).arg(timelineModel.activeCount)
                    font.pixelSize: 12
                    color: "#565f89"
                }

                // 壁纸列表
                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: timelineModel

                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 50
                        color: dragArea.containsMouse ? "#24283b" : "transparent"
                        radius: 4

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 5
                            spacing: 10

                            // 拖拽指示
                            Text {
                                text: "⠿"
                                font.pixelSize: 16
                                color: "#565f89"
                            }

                            // 文件名
                            Text {
                                Layout.fillWidth: true
                                text: model.filename
                                font.pixelSize: 12
                                color: "#a9b1d6"
                                elide: Text.ElideMiddle
                            }

                            // 时间标记
                            Text {
                                text: model.allDay ? "24h" : "⏰"
                                font.pixelSize: 12
                                color: model.allDay ? "#9ece6a" : "#7aa2f7"
                            }
                        }

                        MouseArea {
                            id: dragArea
                            anchors.fill: parent
                            hoverEnabled: true
                            drag.target: dragItem
                            
                            // TODO: 实现拖拽到时间轴
                        }
                    }
                }
            }
        }

        // 右侧：时间轴
        Rectangle {
            SplitView.fillWidth: true
            color: "#1a1b26"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 15

                // 标题栏
                RowLayout {
                    Layout.fillWidth: true

                    Text {
                        text: qsTr("Timeline Editor")
                        font.pixelSize: 18
                        font.bold: true
                        color: "#a9b1d6"
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: qsTr("Current: %1").arg(timelineModel.currentTime)
                        font.pixelSize: 14
                        color: "#7aa2f7"
                    }

                    Text {
                        text: qsTr("Next refresh: %1").arg(timelineModel.nextTimePoint || qsTr("None"))
                        font.pixelSize: 14
                        color: "#565f89"
                    }
                }

                // 时间轴视图
                TimelineView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    
                    timePoints: timelineModel.timePoints
                    currentTime: timelineModel.currentTime
                }

                // 操作栏
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 15

                    Text {
                        text: qsTr("File operation:")
                        color: "#a9b1d6"
                    }

                    RadioButton {
                        text: qsTr("Hard Link")
                        checked: root.fileOperation === "link"
                        onClicked: root.fileOperation = "link"

                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Create hard link (saves space, original stays)")
                    }

                    RadioButton {
                        text: qsTr("Copy")
                        checked: root.fileOperation === "copy"
                        onClicked: root.fileOperation = "copy"

                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Copy file (duplicates space, original stays)")
                    }

                    RadioButton {
                        text: qsTr("Move")
                        checked: root.fileOperation === "move"
                        onClicked: root.fileOperation = "move"

                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Move file (original is removed)")
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        text: qsTr("Save")
                        highlighted: true
                        // TODO: 保存时间轴更改
                    }

                    Button {
                        text: qsTr("Reset")
                        // TODO: 重置更改
                    }
                }
            }
        }
    }

    // 拖拽占位
    Item {
        id: dragItem
        visible: false
    }

    Component.onCompleted: {
        timelineModel.refresh()
    }
}
