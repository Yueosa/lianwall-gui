import QtQuick
import QtQuick.Controls
import LianwallGui 1.0

Canvas {
    id: root
    
    property var wallpaperModel: null
    property var spaceData: null

    // 监听数据变化
    Connections {
        target: Client
        function onSpaceReceived(space) {
            root.spaceData = space
            root.requestPaint()
        }
    }

    onPaint: {
        var ctx = getContext("2d");
        ctx.reset();

        if (!spaceData || !spaceData.items) return;

        var centerX = width / 2;
        var centerY = height / 2;
        var maxRadius = Math.min(width, height) / 2 - 50;
        var items = spaceData.items;
        var currentIndex = spaceData.current_index;
        var pointerAngle = spaceData.pointer_angle;

        // 绘制背景圆
        ctx.strokeStyle = "#24283b";
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.arc(centerX, centerY, maxRadius, 0, 2 * Math.PI);
        ctx.stroke();

        // 绘制中心点
        ctx.fillStyle = "#414868";
        ctx.beginPath();
        ctx.arc(centerX, centerY, 5, 0, 2 * Math.PI);
        ctx.fill();

        // 绘制壁纸点
        for (var i = 0; i < items.length; i++) {
            var item = items[i];
            
            // 向日葵排列：半径与 sqrt(index) 成正比
            var r = maxRadius * Math.sqrt((i + 1) / items.length);
            var x = centerX + r * Math.cos(item.angle);
            var y = centerY + r * Math.sin(item.angle);

            // 根据状态选择颜色
            if (i === currentIndex) {
                ctx.fillStyle = "#f7768e";  // 当前 - 红色
            } else if (item.locked) {
                ctx.fillStyle = "#565f89";  // 锁定 - 灰色
            } else if (item.in_cooldown) {
                ctx.fillStyle = "#7aa2f7";  // 冷却 - 蓝色
            } else {
                ctx.fillStyle = "#9ece6a";  // 可用 - 绿色
            }

            var pointRadius = (i === currentIndex) ? 8 : 5;
            ctx.beginPath();
            ctx.arc(x, y, pointRadius, 0, 2 * Math.PI);
            ctx.fill();
        }

        // 绘制指针
        if (pointerAngle !== undefined) {
            var pointerLength = maxRadius + 20;
            var pointerX = centerX + pointerLength * Math.cos(pointerAngle);
            var pointerY = centerY + pointerLength * Math.sin(pointerAngle);

            ctx.strokeStyle = "#bb9af7";
            ctx.lineWidth = 2;
            ctx.beginPath();
            ctx.moveTo(centerX, centerY);
            ctx.lineTo(pointerX, pointerY);
            ctx.stroke();

            // 指针箭头
            ctx.fillStyle = "#bb9af7";
            ctx.beginPath();
            ctx.arc(pointerX, pointerY, 6, 0, 2 * Math.PI);
            ctx.fill();
        }

        // 绘制黄金角参考线
        ctx.strokeStyle = "#414868";
        ctx.lineWidth = 1;
        ctx.setLineDash([5, 5]);
        
        var goldenAngle = 2.399963;  // ≈ 137.508°
        for (var j = 0; j < 5; j++) {
            var angle = j * goldenAngle;
            var refX = centerX + maxRadius * Math.cos(angle);
            var refY = centerY + maxRadius * Math.sin(angle);
            
            ctx.beginPath();
            ctx.moveTo(centerX, centerY);
            ctx.lineTo(refX, refY);
            ctx.stroke();
        }
        ctx.setLineDash([]);
    }

    // 初始加载
    Component.onCompleted: {
        if (Client.connected) {
            Client.getSpace()
        }
    }
}
