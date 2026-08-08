import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".." as App
import "../components" as Components

/// 关于页面
/// Logo + 版本 + 双仓库链接 + 主题色声明
Item {
    id: aboutRoot

    Flickable {
        id: aboutFlick
        anchors.fill: parent
        clip: true
        contentWidth: width
        contentHeight: aboutColumn.height + App.Theme.spacingLarge * 4
        boundsBehavior: Flickable.StopAtBounds
        flickableDirection: Flickable.VerticalFlick

        ScrollBar.vertical: Components.StyledScrollBar {}

        ColumnLayout {
            id: aboutColumn
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: App.Theme.spacingLarge * 2
            width: Math.min(aboutFlick.width - App.Theme.spacingLarge * 2, 480)
            spacing: App.Theme.spacingLarge

            // Logo
            Image {
                    Layout.alignment: Qt.AlignHCenter
                    source: "qrc:/icons/lianwall.png"
                    sourceSize: Qt.size(96, 96)
                    fillMode: Image.PreserveAspectFit
                }

                // 应用名
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "LianWall"
                    font.pixelSize: App.Theme.fontSizeHeader
                    font.bold: true
                    color: App.Theme.text
                }

                // 版本号
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "v" + AppVersion
                    font.pixelSize: App.Theme.fontSizeLarge
                    color: App.Theme.textSecondary
                }

                // 描述
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.maximumWidth: parent.width
                    text: AppDescription
                    font.pixelSize: App.Theme.fontSizeMedium
                    color: App.Theme.textSecondary
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                }

                // 作者
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("作者: %1").arg(AppAuthor)
                    font.pixelSize: App.Theme.fontSizeMedium
                    color: App.Theme.text
                }

                // 分隔线
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: App.Theme.divider
                }

                // GitHub 链接按钮
                ColumnLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: App.Theme.spacingSmall

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("源代码")
                        font.pixelSize: App.Theme.fontSizeMedium
                        font.bold: true
                        color: App.Theme.text
                    }

                    // GUI 仓库
                    LinkButton {
                        Layout.alignment: Qt.AlignHCenter
                        text: "🖥️  " + qsTr("GUI 客户端") + "  —  Yueosa/lianwall-gui"
                        url: AppGitHubGui
                    }

                    // Daemon 仓库
                    LinkButton {
                        Layout.alignment: Qt.AlignHCenter
                        text: "⚙️  " + qsTr("Daemon 内核") + "  —  Yueosa/lianwall"
                        url: AppGitHubDaemon
                    }
                }

                // 分隔线
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: App.Theme.divider
                }

                // 主题色声明
                ColumnLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: App.Theme.spacingSmall

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("关于主题色")
                        font.pixelSize: App.Theme.fontSizeMedium
                        font.bold: true
                        color: App.Theme.text
                    }

                    // 色彩展示
                    Row {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: App.Theme.spacingSmall

                        Repeater {
                            model: [App.Theme.mtfBlue, App.Theme.mtfWhite, App.Theme.mtfPink]
                            Rectangle {
                                width: 48; height: 24
                                radius: App.Theme.radiusSmall
                                color: modelData
                                border.width: 1
                                border.color: App.Theme.border
                            }
                        }
                    }

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.maximumWidth: aboutColumn.width
                        text: qsTr("本应用的主题色取自跨性别骄傲旗配色（蓝 #5BCEFA、粉 #F5A9B8、白 #FFFFFF），纯粹因为作者喜欢这些颜色的搭配。作者本人并非 MTF。")
                        font.pixelSize: App.Theme.fontSizeSmall
                        color: App.Theme.textSecondary
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        lineHeight: 1.4
                    }
                }

            // 底部间距
            Item { Layout.preferredHeight: App.Theme.spacingLarge }
        }
    }

    // ====================================================================
    // 内联组件：链接按钮
    // ====================================================================
    component LinkButton: Item {
        property string text: ""
        property string url: ""
        implicitWidth: linkRow.width + App.Theme.spacingMedium * 2
        implicitHeight: 36

        Rectangle {
            anchors.fill: parent
            radius: App.Theme.radiusMedium
            color: linkMouseArea.containsMouse ? App.Theme.cardHover : "transparent"
            border.width: 1
            border.color: linkMouseArea.containsMouse ? App.Theme.borderHover : App.Theme.border

            Row {
                id: linkRow
                anchors.centerIn: parent
                spacing: App.Theme.spacingTiny

                Text {
                    text: parent.parent.parent.text
                    font.pixelSize: App.Theme.fontSizeSmall
                    color: App.Theme.accent
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            MouseArea {
                id: linkMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: Qt.openUrlExternally(url)
            }
        }
    }
}
