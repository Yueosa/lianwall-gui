import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import LianwallGui 1.0

Page {
    id: root
    
    required property StatusModel statusModel

    background: Rectangle { color: "#1a1b26" }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        // 壁纸预览区域
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.maximumHeight: 400
            color: "#24283b"
            radius: 12

            // 视频播放器
            Video {
                id: videoPlayer
                anchors.fill: parent
                anchors.margins: 2
                
                source: statusModel.mode === "Video" ? "file://" + statusModel.current : ""
                fillMode: VideoOutput.PreserveAspectCrop
                loops: MediaPlayer.Infinite
                muted: true
                visible: statusModel.mode === "Video" && statusModel.current !== ""
                
                // 点击播放/暂停
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (videoPlayer.playbackState === MediaPlayer.PlayingState) {
                            videoPlayer.pause()
                        } else {
                            videoPlayer.play()
                        }
                    }
                }

                Component.onCompleted: play()
            }

            // 图片显示
            Image {
                anchors.fill: parent
                anchors.margins: 2
                source: statusModel.mode === "Image" ? "file://" + statusModel.current : ""
                fillMode: Image.PreserveAspectCrop
                visible: statusModel.mode === "Image" && statusModel.current !== ""
            }

            // 无壁纸提示
            Text {
                anchors.centerIn: parent
                text: qsTr("No wallpaper")
                font.pixelSize: 18
                color: "#565f89"
                visible: statusModel.current === ""
            }

            // 播放状态图标
            Rectangle {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 10
                width: 40
                height: 40
                radius: 20
                color: "#1a1b26"
                opacity: 0.8
                visible: statusModel.mode === "Video" && statusModel.current !== ""

                Text {
                    anchors.centerIn: parent
                    text: videoPlayer.playbackState === MediaPlayer.PlayingState ? "⏸" : "▶"
                    font.pixelSize: 20
                    color: "#a9b1d6"
                }
            }
        }

        // 信息区域
        RowLayout {
            Layout.fillWidth: true
            spacing: 20

            // 当前壁纸信息
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: qsTr("Mode: %1").arg(statusModel.mode)
                    font.pixelSize: 14
                    color: "#7aa2f7"
                }

                Text {
                    text: statusModel.currentFilename || qsTr("None")
                    font.pixelSize: 16
                    font.bold: true
                    color: "#a9b1d6"
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }

                Text {
                    text: qsTr("Available: %1 / %2").arg(statusModel.availableCount).arg(statusModel.totalWallpapers)
                    font.pixelSize: 12
                    color: "#565f89"
                }
            }

            // 控制按钮
            RowLayout {
                spacing: 15

                Button {
                    text: "◀ " + qsTr("Prev")
                    font.pixelSize: 14
                    implicitWidth: 100
                    implicitHeight: 45
                    
                    background: Rectangle {
                        color: parent.pressed ? "#3d59a1" : "#24283b"
                        radius: 8
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: "#a9b1d6"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: Client.previous()
                }

                Button {
                    text: "⟳ " + qsTr("Switch")
                    font.pixelSize: 14
                    implicitWidth: 110
                    implicitHeight: 45
                    
                    background: Rectangle {
                        color: parent.pressed ? "#3d59a1" : "#7aa2f7"
                        radius: 8
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: "#1a1b26"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: {
                        let newMode = statusModel.mode === "Video" ? "Image" : "Video"
                        Client.setMode(newMode)
                    }
                }

                Button {
                    text: qsTr("Next") + " ▶"
                    font.pixelSize: 14
                    implicitWidth: 100
                    implicitHeight: 45
                    
                    background: Rectangle {
                        color: parent.pressed ? "#3d59a1" : "#24283b"
                        radius: 8
                    }
                    
                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: "#a9b1d6"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: Client.next()
                }
            }
        }

        // 额外信息
        RowLayout {
            Layout.fillWidth: true
            spacing: 30

            Text {
                text: "🎬 " + statusModel.engine
                font.pixelSize: 12
                color: "#565f89"
            }

            Text {
                text: "🔒 " + qsTr("Locked: %1").arg(statusModel.lockedCount)
                font.pixelSize: 12
                color: "#565f89"
            }

            Text {
                text: "⏰ " + qsTr("Next refresh: %1").arg(statusModel.nextTimePoint || qsTr("None"))
                font.pixelSize: 12
                color: "#565f89"
                visible: statusModel.timePointsCount > 0
            }

            Item { Layout.fillWidth: true }

            Text {
                text: "💾 " + qsTr("VRAM: %1/%2 MB").arg(statusModel.vramUsedMb).arg(statusModel.vramTotalMb)
                font.pixelSize: 12
                color: "#565f89"
                visible: statusModel.vramTotalMb > 0
            }
        }
    }
}
