import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import LianwallGui 1.0

ApplicationWindow {
    id: root
    width: 1000
    height: 700
    minimumWidth: 800
    minimumHeight: 600
    visible: false  // 初始隐藏，托盘控制显示
    title: qsTr("LianWall")

    // 当前页面索引
    property int currentPage: 0

    // 状态模型
    StatusModel {
        id: statusModel
        Component.onCompleted: {
            setClient(Client)
            refresh()
        }
    }

    // 壁纸模型
    WallpaperModel {
        id: wallpaperModel
        Component.onCompleted: {
            setClient(Client)
            setThumbnailProvider(ThumbnailCache)
        }
    }

    // 时间轴模型
    TimelineModel {
        id: timelineModel
        Component.onCompleted: {
            setClient(Client)
        }
    }

    // 定时刷新状态
    Timer {
        interval: 5000
        running: true
        repeat: true
        onTriggered: statusModel.refresh()
    }

    // 窗口关闭事件
    onClosing: function(close) {
        let behavior = ConfigManager.exitBehavior()
        
        if (behavior === "minimize") {
            close.accepted = false
            root.hide()
        } else if (behavior === "shutdown") {
            ProcessManager.gracefulShutdown()
        } else {
            // "ask" - 弹窗
            close.accepted = false
            exitDialog.open()
        }
    }

    // 主布局
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // 导航栏
        NavBar {
            id: navBar
            Layout.fillWidth: true
            currentIndex: root.currentPage
            onPageSelected: function(index) {
                root.currentPage = index
            }
        }

        // 页面栈
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: root.currentPage

            HomePage {
                statusModel: statusModel
            }

            LibraryPage {
                wallpaperModel: wallpaperModel
            }

            TimelinePage {
                timelineModel: timelineModel
            }

            StatusPage {
                wallpaperModel: wallpaperModel
            }

            SettingsPage {}

            AboutPage {}
        }
    }

    // 退出确认对话框
    ExitDialog {
        id: exitDialog
        onMinimizeRequested: root.hide()
        onShutdownRequested: ProcessManager.gracefulShutdown()
        onQuitRequested: Qt.quit()
    }

    // 显示窗口的函数
    function show() {
        root.visible = true
        root.raise()
        root.requestActivate()
    }
}
