import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LianwallGui 1.0

Page {
    id: root
    
    required property WallpaperModel wallpaperModel

    background: Rectangle { color: "#1a1b26" }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15

        // 筛选栏
        RowLayout {
            Layout.fillWidth: true
            spacing: 15

            Text {
                text: qsTr("Filter:")
                font.pixelSize: 14
                color: "#a9b1d6"
            }

            ComboBox {
                id: modeFilter
                model: [qsTr("All"), qsTr("Video"), qsTr("Image")]
                implicitWidth: 100
                
                onCurrentIndexChanged: {
                    let filters = ["all", "video", "image"]
                    wallpaperModel.modeFilter = filters[currentIndex]
                }
            }

            ComboBox {
                id: statusFilter
                model: [qsTr("All"), qsTr("Locked"), qsTr("Unlocked"), qsTr("Cooldown")]
                implicitWidth: 120
                
                onCurrentIndexChanged: {
                    let filters = ["all", "locked", "unlocked", "cooldown"]
                    wallpaperModel.statusFilter = filters[currentIndex]
                }
            }

            CheckBox {
                id: activeOnly
                text: qsTr("Active now")
                
                onCheckedChanged: {
                    wallpaperModel.activeOnlyFilter = checked
                }
            }

            Item { Layout.fillWidth: true }

            Text {
                text: qsTr("%1 wallpapers").arg(wallpaperModel.count)
                font.pixelSize: 14
                color: "#565f89"
            }

            Button {
                text: "🔄 " + qsTr("Refresh")
                onClicked: wallpaperModel.refresh()
            }
        }

        // 壁纸网格
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            GridView {
                id: gridView
                anchors.fill: parent
                cellWidth: 200
                cellHeight: 150
                model: wallpaperModel

                delegate: WallpaperCard {
                    width: gridView.cellWidth - 10
                    height: gridView.cellHeight - 10
                    
                    filename: model.filename
                    thumbnail: model.thumbnail || ""
                    locked: model.locked
                    inCooldown: model.inCooldown
                    isCurrent: model.isCurrent

                    onClicked: {
                        Client.setWallpaper(model.path)
                    }

                    onLockToggled: {
                        if (model.locked) {
                            Client.unlock(model.path)
                        } else {
                            Client.lock(model.path)
                        }
                        // 延迟刷新
                        Qt.callLater(wallpaperModel.refresh)
                    }
                }
            }
        }
    }
}
