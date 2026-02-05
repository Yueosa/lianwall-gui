import QtQuick
import QtQuick.Controls
import QtMultimedia

Item {
    id: root
    
    property string source: ""
    property string mediaType: "video"  // video, image
    property bool autoPlay: true
    property bool showControls: true
    
    // 视频播放器
    Video {
        id: videoPlayer
        anchors.fill: parent
        source: root.mediaType === "video" ? root.source : ""
        fillMode: VideoOutput.PreserveAspectFit
        loops: MediaPlayer.Infinite
        
        visible: root.mediaType === "video" && source !== ""
        
        Component.onCompleted: {
            if (root.autoPlay && source !== "") {
                play()
            }
        }
        
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
    }
    
    // 图片显示
    Image {
        id: imageViewer
        anchors.fill: parent
        source: root.mediaType === "image" ? root.source : ""
        fillMode: Image.PreserveAspectFit
        
        visible: root.mediaType === "image" && source !== ""
    }
    
    // 占位符
    Rectangle {
        anchors.fill: parent
        color: "#1e1e2e"
        visible: root.source === ""
        
        Label {
            anchors.centerIn: parent
            text: qsTr("无预览")
            color: "#6c7086"
        }
    }
    
    // 控制覆盖层
    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 40
        color: "#80000000"
        visible: showControls && root.mediaType === "video" && root.source !== ""
        
        Row {
            anchors.centerIn: parent
            spacing: 16
            
            ToolButton {
                icon.name: videoPlayer.playbackState === MediaPlayer.PlayingState 
                    ? "media-playback-pause" : "media-playback-start"
                onClicked: {
                    if (videoPlayer.playbackState === MediaPlayer.PlayingState) {
                        videoPlayer.pause()
                    } else {
                        videoPlayer.play()
                    }
                }
            }
            
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: formatTime(videoPlayer.position) + " / " + formatTime(videoPlayer.duration)
                font.family: "monospace"
            }
            
            ToolButton {
                icon.name: videoPlayer.muted ? "audio-volume-muted" : "audio-volume-high"
                onClicked: videoPlayer.muted = !videoPlayer.muted
            }
        }
    }
    
    // 类型指示器
    Rectangle {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 8
        width: typeLabel.width + 12
        height: 24
        radius: 4
        color: "#80000000"
        visible: root.source !== ""
        
        Label {
            id: typeLabel
            anchors.centerIn: parent
            text: root.mediaType === "video" ? "VIDEO" : "IMAGE"
            font.pixelSize: 10
            font.bold: true
        }
    }
    
    function formatTime(ms) {
        var secs = Math.floor(ms / 1000)
        var mins = Math.floor(secs / 60)
        secs = secs % 60
        return mins + ":" + (secs < 10 ? "0" : "") + secs
    }
    
    function play() {
        if (mediaType === "video") {
            videoPlayer.play()
        }
    }
    
    function pause() {
        if (mediaType === "video") {
            videoPlayer.pause()
        }
    }
    
    function stop() {
        if (mediaType === "video") {
            videoPlayer.stop()
        }
    }
    
    onSourceChanged: {
        if (mediaType === "video" && autoPlay && source !== "") {
            videoPlayer.play()
        }
    }
}
