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
        spacing: 20

        Text {
            text: qsTr("Sunflower Visualization")
            font.pixelSize: 18
            font.bold: true
            color: "#a9b1d6"
        }

        // 向日葵画布
        SunflowerCanvas {
            Layout.fillWidth: true
            Layout.fillHeight: true
            wallpaperModel: root.wallpaperModel
        }

        // 图例
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 30

            Row {
                spacing: 5
                Rectangle { width: 12; height: 12; radius: 6; color: "#9ece6a" }
                Text { text: qsTr("Available"); color: "#a9b1d6"; font.pixelSize: 12 }
            }

            Row {
                spacing: 5
                Rectangle { width: 12; height: 12; radius: 6; color: "#f7768e" }
                Text { text: qsTr("Current"); color: "#a9b1d6"; font.pixelSize: 12 }
            }

            Row {
                spacing: 5
                Rectangle { width: 12; height: 12; radius: 6; color: "#7aa2f7" }
                Text { text: qsTr("Cooldown"); color: "#a9b1d6"; font.pixelSize: 12 }
            }

            Row {
                spacing: 5
                Rectangle { width: 12; height: 12; radius: 6; color: "#565f89" }
                Text { text: qsTr("Locked"); color: "#a9b1d6"; font.pixelSize: 12 }
            }
        }
    }
}
