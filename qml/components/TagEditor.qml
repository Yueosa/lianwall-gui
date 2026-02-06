import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".." as App

/// 数组参数编辑器
/// 用于编辑 mpvpaper_args / mpv_args / swww_args
/// 以 tag 标签列表显示，支持添加/删除
Item {
    id: tagRoot

    property string label: ""
    property var tags: []         // QStringList 绑定
    signal tagsEdited(var newTags)

    implicitHeight: tagCol.height

    ColumnLayout {
        id: tagCol
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: App.Theme.spacingSmall

        // 标签
        Text {
            text: tagRoot.label
            font.pixelSize: App.Theme.fontSizeMedium
            font.bold: true
            color: App.Theme.text
            visible: tagRoot.label.length > 0
        }

        // tag 列表
        Flow {
            Layout.fillWidth: true
            spacing: App.Theme.spacingTiny

            Repeater {
                model: tagRoot.tags

                Rectangle {
                    width: tagText.width + removeBtn.width + App.Theme.spacingSmall * 3
                    height: 28
                    radius: App.Theme.radiusSmall
                    color: App.Theme.surface
                    border.width: 1
                    border.color: App.Theme.border

                    Row {
                        anchors.centerIn: parent
                        spacing: App.Theme.spacingTiny

                        Text {
                            id: tagText
                            text: modelData
                            font.pixelSize: App.Theme.fontSizeSmall
                            font.family: "monospace"
                            color: App.Theme.text
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Text {
                            id: removeBtn
                            text: "✕"
                            font.pixelSize: App.Theme.fontSizeSmall
                            color: removeMouse.containsMouse ? App.Theme.error : App.Theme.textSecondary
                            anchors.verticalCenter: parent.verticalCenter

                            MouseArea {
                                id: removeMouse
                                anchors.fill: parent
                                anchors.margins: -4
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    var newTags = tagRoot.tags.slice()
                                    newTags.splice(index, 1)
                                    tagRoot.tagsEdited(newTags)
                                }
                            }
                        }
                    }
                }
            }
        }

        // 添加输入框
        RowLayout {
            Layout.fillWidth: true
            spacing: App.Theme.spacingSmall

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                radius: App.Theme.radiusSmall
                color: App.Theme.surface
                border.width: 1
                border.color: addInput.activeFocus ? App.Theme.accent : App.Theme.border

                TextInput {
                    id: addInput
                    anchors.fill: parent
                    anchors.margins: App.Theme.spacingSmall
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: App.Theme.fontSizeSmall
                    font.family: "monospace"
                    color: App.Theme.text
                    clip: true

                    Keys.onReturnPressed: addTag()
                    Keys.onEnterPressed: addTag()

                    Text {
                        anchors.fill: parent
                        verticalAlignment: Text.AlignVCenter
                        text: qsTr("输入参数后按 Enter 添加...")
                        font.pixelSize: App.Theme.fontSizeSmall
                        color: App.Theme.textSecondary
                        visible: !addInput.text && !addInput.activeFocus
                    }
                }
            }

            Rectangle {
                width: 32
                height: 32
                radius: App.Theme.radiusSmall
                color: addBtnMouse.pressed ? App.Theme.accentPressed
                       : addBtnMouse.containsMouse ? App.Theme.accentHover
                       : App.Theme.accent

                Text {
                    anchors.centerIn: parent
                    text: "+"
                    font.pixelSize: 16
                    font.bold: true
                    color: App.Theme.textOnAccent
                }

                MouseArea {
                    id: addBtnMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: addTag()
                }
            }
        }
    }

    function addTag() {
        var text = addInput.text.trim()
        if (text.length === 0) return
        var newTags = tagRoot.tags.slice()
        newTags.push(text)
        addInput.text = ""
        tagRoot.tagsEdited(newTags)
    }
}
