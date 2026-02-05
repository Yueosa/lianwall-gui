# LianWall Socket 通信协议文档

> 版本：Protocol v1  
> 适用于：LianWall 4.0.0+  
> 最后更新：2026-02-05

---

## 1. 连接信息

| 项目 | 值 |
|------|-----|
| **Socket 类型** | Unix Domain Socket |
| **默认路径** | `/tmp/lianwall.sock` |
| **协议版本** | `1` |
| **最大消息** | 1 MB |

---

## 2. 帧格式

```
+----------------+------------------+
| 长度 (4 bytes) | JSON 数据        |
| u32 big-endian | UTF-8 字符串     |
+----------------+------------------+
```

**Qt 读写示例**：
```cpp
// 发送
void send(QLocalSocket *socket, const QJsonObject &obj) {
    QByteArray json = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    quint32 len = json.size();
    QByteArray header(4, 0);
    qToBigEndian(len, header.data());
    socket->write(header + json);
}

// 接收
QJsonObject receive(QLocalSocket *socket) {
    QByteArray header = socket->read(4);
    quint32 len = qFromBigEndian<quint32>(header.constData());
    QByteArray json = socket->read(len);
    return QJsonDocument::fromJson(json).object();
}
```

---

## 3. 请求格式 (Request)

所有请求使用 **tagged JSON**，格式：
```json
{"cmd": "命令名", "data": {参数对象}}
```

无参数的命令省略 `data` 字段。

---

## 4. 请求命令一览

### 4.1 状态查询类

| 命令 | 参数 | 说明 |
|------|------|------|
| `Ping` | 无 | 心跳检测 |
| `Status` | 无 | 获取完整状态信息 |
| `GetSpace` | 无 | 获取向量空间快照（GUI 向日葵绘图） |
| `GetTimeInfo` | 无 | 获取时间调度信息（GUI 时间轴） |

### 4.2 壁纸控制类

| 命令 | 参数 | 说明 |
|------|------|------|
| `Next` | 无 | 切换到下一张壁纸 |
| `Previous` | 无 | 切换到上一张壁纸（历史栈） |
| `SetWallpaper` | `path: string` | 指定壁纸路径 |
| `SetMode` | `mode: "Video" \| "Image"` | 设置模式 |
| `Lock` | `path: string` | 锁定壁纸 |
| `Unlock` | `path: string` | 解锁壁纸 |
| `Reload` | 无 | 重新扫描目录并重载配置 |

### 4.3 生命周期类

| 命令 | 参数 | 说明 |
|------|------|------|
| `Shutdown` | 无 | 优雅关闭守护进程 |

---

## 5. 请求 JSON 示例

### 无参数命令
```json
{"cmd": "Next"}
{"cmd": "Previous"}
{"cmd": "Ping"}
{"cmd": "Status"}
{"cmd": "GetSpace"}
{"cmd": "GetTimeInfo"}
{"cmd": "Reload"}
{"cmd": "Shutdown"}
```

### 带参数命令
```json
// SetWallpaper
{"cmd": "SetWallpaper", "data": {"path": "/home/user/Videos/lianwall/sunset.mp4"}}

// SetMode
{"cmd": "SetMode", "data": {"mode": "Video"}}
{"cmd": "SetMode", "data": {"mode": "Image"}}

// Lock
{"cmd": "Lock", "data": {"path": "/home/user/Videos/lianwall/sunset.mp4"}}

// Unlock
{"cmd": "Unlock", "data": {"path": "/home/user/Videos/lianwall/sunset.mp4"}}
```

---

## 6. 响应格式 (Response)

所有响应结构：
```json
{
    "success": true | false,
    "data": { ... },      // 成功时
    "error": "message"    // 失败时
}
```

### 6.1 ResponseData 类型

| type | 说明 | 对应请求 |
|------|------|----------|
| `Ok` | 简单确认 | Next, Previous, SetWallpaper, SetMode, Lock, Unlock, Reload, Shutdown |
| `Pong` | 心跳响应 | Ping |
| `Status` | 状态信息 | Status |
| `Space` | 向量空间快照 | GetSpace |
| `TimeInfo` | 时间调度信息 | GetTimeInfo |

---

## 7. 响应数据结构

### 7.1 简单响应

**Ok** (操作成功)
```json
{
    "success": true,
    "data": {"type": "Ok"}
}
```

**Pong** (心跳)
```json
{
    "success": true,
    "data": {"type": "Pong"}
}
```

**Error** (失败)
```json
{
    "success": false,
    "error": "Wallpaper not found: /path/to/file.mp4"
}
```

---

### 7.2 StatusInfo (Status 响应)

```json
{
    "success": true,
    "data": {
        "type": "Status",
        "payload": {
            "mode": "Video",
            "current": "/home/user/Videos/lianwall/sunset.mp4",
            "engine": "mpvpaper",
            "total_wallpapers": 25,
            "locked_count": 3,
            "available_count": 15,
            "scanned_count": 50,
            "vram_used_mb": 1024,
            "vram_total_mb": 8192,
            "uptime_secs": 3600,
            "protocol_version": 1,
            "next_time_point": "18:00",
            "time_points_count": 4
        }
    }
}
```

**字段说明**：

| 字段 | 类型 | 说明 |
|------|------|------|
| `mode` | `"Video"` \| `"Image"` | 当前模式 |
| `current` | `string?` | 当前壁纸路径（null 表示无） |
| `engine` | `string` | 引擎名称 (`mpvpaper` / `swww`) |
| `total_wallpapers` | `int` | 活跃壁纸数（时间过滤后，向量空间中） |
| `locked_count` | `int` | 锁定数量 |
| `available_count` | `int` | 可选数量（未锁定、不在冷却中） |
| `scanned_count` | `int` | 扫描总数（含不活跃的） |
| `vram_used_mb` | `int` | 显存已用 (MB) |
| `vram_total_mb` | `int` | 显存总量 (MB)，0 表示无 GPU |
| `uptime_secs` | `int` | Daemon 运行时间（秒） |
| `protocol_version` | `int` | 协议版本号 |
| `next_time_point` | `string?` | 下个时间关键点 ("HH:MM")，null 表示无 |
| `time_points_count` | `int` | 时间关键点总数 |

---

### 7.3 SpaceSnapshot (GetSpace 响应)

```json
{
    "success": true,
    "data": {
        "type": "Space",
        "payload": {
            "items": [
                {
                    "index": 0,
                    "filename": "sunset.mp4",
                    "path": "/home/user/Videos/lianwall/sunset.mp4",
                    "angle": 0.0,
                    "locked": false,
                    "in_cooldown": false
                },
                {
                    "index": 1,
                    "filename": "ocean.mp4",
                    "path": "/home/user/Videos/lianwall/ocean.mp4",
                    "angle": 2.399963,
                    "locked": false,
                    "in_cooldown": true
                }
            ],
            "pointer_angle": 4.799926,
            "cooldown_size": 5,
            "current_index": 1
        }
    }
}
```

**字段说明**：

| 字段 | 类型 | 说明 |
|------|------|------|
| `items` | `WallpaperPoint[]` | 壁纸点列表 |
| `pointer_angle` | `float` | 当前指针角度 [0, 2π) |
| `cooldown_size` | `int` | 冷却队列大小 |
| `current_index` | `int?` | 当前壁纸在 items 中的索引 |

**WallpaperPoint 结构**：

| 字段 | 类型 | 说明 |
|------|------|------|
| `index` | `int` | 在向量空间中的索引 |
| `filename` | `string` | 文件名（不含路径） |
| `path` | `string` | 完整路径 |
| `angle` | `float` | 角度 [0, 2π)，黄金角 ≈ 2.399963 |
| `locked` | `bool` | 是否锁定 |
| `in_cooldown` | `bool` | 是否在冷却队列中 |

**GUI 绘图提示**：
```cpp
// 向日葵种子排列
for (auto &item : items) {
    float x = centerX + radius * cos(item.angle);
    float y = centerY + radius * sin(item.angle);
    // 根据 locked / in_cooldown 设置颜色
}
// 绘制指针
drawPointer(centerX, centerY, pointer_angle);
```

---

### 7.4 TimeScheduleInfo (GetTimeInfo 响应)

```json
{
    "success": true,
    "data": {
        "type": "TimeInfo",
        "payload": {
            "current_time": "14:30",
            "video_schedule": {
                "scanned_count": 50,
                "active_count": 20,
                "time_points": ["08:00", "12:00", "18:00", "22:00"],
                "next_time_point": "18:00",
                "wallpaper_segments": [
                    {
                        "filename": "morning.mp4",
                        "path": "/home/user/Videos/lianwall/08-12/morning.mp4",
                        "active_ranges": [
                            {"start": "08:00", "end": "12:00", "crosses_midnight": false}
                        ],
                        "all_day": false
                    },
                    {
                        "filename": "always.mp4",
                        "path": "/home/user/Videos/lianwall/always.mp4",
                        "active_ranges": [],
                        "all_day": true
                    },
                    {
                        "filename": "night.mp4",
                        "path": "/home/user/Videos/lianwall/22-06/night.mp4",
                        "active_ranges": [
                            {"start": "22:00", "end": "06:00", "crosses_midnight": true}
                        ],
                        "all_day": false
                    }
                ]
            },
            "image_schedule": {
                "scanned_count": 30,
                "active_count": 30,
                "time_points": [],
                "next_time_point": null,
                "wallpaper_segments": []
            }
        }
    }
}
```

**ModeSchedule 结构**：

| 字段 | 类型 | 说明 |
|------|------|------|
| `scanned_count` | `int` | 扫描总数 |
| `active_count` | `int` | 当前活跃数 |
| `time_points` | `string[]` | 关键时间点列表（已排序） |
| `next_time_point` | `string?` | 下个关键时间点 |
| `wallpaper_segments` | `WallpaperTimeSegment[]` | 壁纸时间分布 |

**WallpaperTimeSegment 结构**：

| 字段 | 类型 | 说明 |
|------|------|------|
| `filename` | `string` | 文件名 |
| `path` | `string` | 完整路径 |
| `active_ranges` | `TimeRangeInfo[]` | 活跃时间段列表 |
| `all_day` | `bool` | 是否全天可用 |

**TimeRangeInfo 结构**：

| 字段 | 类型 | 说明 |
|------|------|------|
| `start` | `string` | 开始时间 ("HH:MM") |
| `end` | `string` | 结束时间 ("HH:MM") |
| `crosses_midnight` | `bool` | 是否跨天 |

---

## 8. 错误响应示例

```json
// 壁纸不存在
{
    "success": false,
    "error": "Wallpaper not found: /path/to/nonexistent.mp4"
}

// 无效模式
{
    "success": false,
    "error": "Invalid mode"
}

// 引擎错误
{
    "success": false,
    "error": "Failed to start mpvpaper: No such file or directory"
}

// 无活跃壁纸
{
    "success": false,
    "error": "No active wallpapers available"
}
```

---

## 9. Qt 集成示例

### 9.1 LianwallClient 类

```cpp
// LianwallClient.h
#pragma once
#include <QObject>
#include <QLocalSocket>
#include <QJsonObject>

class LianwallClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)

public:
    explicit LianwallClient(QObject *parent = nullptr);

    bool isConnected() const;

    // 命令方法
    Q_INVOKABLE void ping();
    Q_INVOKABLE void getStatus();
    Q_INVOKABLE void getSpace();
    Q_INVOKABLE void getTimeInfo();
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void setWallpaper(const QString &path);
    Q_INVOKABLE void setMode(const QString &mode);
    Q_INVOKABLE void lock(const QString &path);
    Q_INVOKABLE void unlock(const QString &path);
    Q_INVOKABLE void reload();
    Q_INVOKABLE void shutdown();

public slots:
    void connectToDaemon(const QString &socketPath = "/tmp/lianwall.sock");
    void disconnectFromDaemon();

signals:
    void connectedChanged();
    void responseReceived(const QJsonObject &response);
    void errorOccurred(const QString &error);

    // 具体响应信号
    void pongReceived();
    void statusReceived(const QJsonObject &status);
    void spaceReceived(const QJsonObject &space);
    void timeInfoReceived(const QJsonObject &timeInfo);
    void operationSucceeded();
    void operationFailed(const QString &error);

private slots:
    void onReadyRead();
    void onError(QLocalSocket::LocalSocketError error);

private:
    void sendRequest(const QJsonObject &request);
    void processResponse(const QJsonObject &response);

    QLocalSocket *m_socket;
    QByteArray m_buffer;
};
```

### 9.2 使用示例

```cpp
// main.cpp
LianwallClient client;
client.connectToDaemon();

// 获取状态
connect(&client, &LianwallClient::statusReceived, [](const QJsonObject &status) {
    qDebug() << "Mode:" << status["mode"].toString();
    qDebug() << "Current:" << status["current"].toString();
    qDebug() << "Available:" << status["available_count"].toInt();
});
client.getStatus();

// 切换壁纸
client.next();

// 设置模式
client.setMode("Video");
```

### 9.3 QML 绑定

```qml
// main.qml
import QtQuick
import LianwallGui

Window {
    LianwallClient {
        id: client
        onStatusReceived: (status) => {
            modeLabel.text = status.mode
            currentLabel.text = status.current || "None"
        }
    }

    Button {
        text: "Next"
        onClicked: client.next()
    }

    Button {
        text: "Previous"
        onClicked: client.previous()
    }

    Component.onCompleted: {
        client.connectToDaemon()
        client.getStatus()
    }
}
```

---

## 10. 连接管理

### 10.1 自动启动 Daemon

```cpp
void ensureDaemonRunning() {
    QLocalSocket test;
    test.connectToServer("/tmp/lianwall.sock");
    if (!test.waitForConnected(100)) {
        // Daemon 未运行，启动它
        QProcess::startDetached("lianwall", {"start"});
        QThread::msleep(500);
    }
}
```

### 10.2 自动重连

```cpp
void LianwallClient::onDisconnected() {
    QTimer::singleShot(1000, this, [this]() {
        connectToDaemon();
    });
}
```

### 10.3 轮询状态（可选）

```cpp
// 每 5 秒更新状态
QTimer *timer = new QTimer(this);
connect(timer, &QTimer::timeout, &client, &LianwallClient::getStatus);
timer->start(5000);
```

---

## 11. 常量定义

```cpp
// Constants.h
namespace LianwallProtocol {
    constexpr const char* DEFAULT_SOCKET_PATH = "/tmp/lianwall.sock";
    constexpr quint32 PROTOCOL_VERSION = 1;
    constexpr qint64 MAX_MESSAGE_SIZE = 1024 * 1024;  // 1 MB
    constexpr int CONNECT_TIMEOUT_MS = 1000;
    constexpr int READ_TIMEOUT_MS = 5000;
}
```

---

## 12. 附录：WallMode 枚举

| 值 | 说明 |
|----|------|
| `"Video"` | 动态壁纸模式（mpvpaper） |
| `"Image"` | 静态壁纸模式（swww） |

---

**文档结束**
