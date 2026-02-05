import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    color: "#1f2335"
    radius: 8

    property var timePoints: []
    property string currentTime: "00:00"

    // 时间刻度
    Row {
        id: timeScale
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 10
        height: 30

        Repeater {
            model: 25  // 0-24

            Text {
                width: (root.width - 20) / 24
                text: index.toString().padStart(2, '0')
                font.pixelSize: 10
                color: "#565f89"
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

    // 主时间轴
    Canvas {
        id: canvas
        anchors.top: timeScale.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 10

        onPaint: {
            var ctx = getContext("2d");
            ctx.reset();

            var width = canvas.width;
            var height = canvas.height;

            // 背景网格
            ctx.strokeStyle = "#24283b";
            ctx.lineWidth = 1;

            // 垂直线（每小时）
            for (var i = 0; i <= 24; i++) {
                var x = (i / 24) * width;
                ctx.beginPath();
                ctx.moveTo(x, 0);
                ctx.lineTo(x, height);
                ctx.stroke();
            }

            // 水平线
            ctx.beginPath();
            ctx.moveTo(0, height / 2);
            ctx.lineTo(width, height / 2);
            ctx.stroke();

            // 时间点标记
            ctx.fillStyle = "#f7768e";
            for (var j = 0; j < root.timePoints.length; j++) {
                var time = root.timePoints[j];
                var parts = time.split(":");
                var hour = parseInt(parts[0]);
                var minute = parseInt(parts[1]);
                var pos = ((hour * 60 + minute) / (24 * 60)) * width;

                ctx.beginPath();
                ctx.arc(pos, 10, 5, 0, 2 * Math.PI);
                ctx.fill();

                // 垂直虚线
                ctx.setLineDash([3, 3]);
                ctx.strokeStyle = "#f7768e";
                ctx.beginPath();
                ctx.moveTo(pos, 15);
                ctx.lineTo(pos, height);
                ctx.stroke();
                ctx.setLineDash([]);
            }

            // 当前时间指示器
            if (root.currentTime) {
                var currentParts = root.currentTime.split(":");
                var currentHour = parseInt(currentParts[0]);
                var currentMinute = parseInt(currentParts[1]);
                var currentPos = ((currentHour * 60 + currentMinute) / (24 * 60)) * width;

                ctx.strokeStyle = "#9ece6a";
                ctx.lineWidth = 2;
                ctx.beginPath();
                ctx.moveTo(currentPos, 0);
                ctx.lineTo(currentPos, height);
                ctx.stroke();

                // 当前时间标签
                ctx.fillStyle = "#9ece6a";
                ctx.font = "12px sans-serif";
                ctx.textAlign = "center";
                ctx.fillText(root.currentTime, currentPos, height - 5);
            }
        }
    }

    // 重绘触发
    onTimePointsChanged: canvas.requestPaint()
    onCurrentTimeChanged: canvas.requestPaint()
    onWidthChanged: canvas.requestPaint()
    onHeightChanged: canvas.requestPaint()

    // 提示
    Text {
        anchors.centerIn: parent
        text: qsTr("Drag wallpapers here to create time segments")
        font.pixelSize: 14
        color: "#565f89"
        visible: root.timePoints.length === 0
    }
}
