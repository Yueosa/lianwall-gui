#pragma once

/// @file DaemonClient.h
/// @brief 异步 Unix Domain Socket 通信客户端
///
/// 基于 QLocalSocket，信号驱动，行分隔 JSON 协议。
/// 自动重连（指数退避）、请求-响应匹配、事件分发。

#include "DaemonTypes.h"

#include <QObject>
#include <QLocalSocket>
#include <QTimer>
#include <QQueue>
#include <functional>

class DaemonClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectionChanged)

public:
    explicit DaemonClient(QObject *parent = nullptr);
    ~DaemonClient() override;

    /// 连接状态
    bool isConnected() const;

    /// 设置 socket 路径（默认 /tmp/lianwall.sock）
    void setSocketPath(const QString &path);

    // ========================================================================
    // 连接管理
    // ========================================================================

    /// 开始连接（自动重连）
    void connectToDaemon();

    /// 断开连接（停止重连）
    void disconnectFromDaemon();

    /// 设置自动重连开关
    void setAutoReconnect(bool enabled);

    // ========================================================================
    // Query 请求
    // ========================================================================

    /// 回调类型：响应到达时调用
    using ResponseCallback = std::function<void(const Daemon::Response &)>;

    void ping(ResponseCallback cb = nullptr);
    void getStatus(ResponseCallback cb = nullptr);
    void getSpace(std::optional<Daemon::WallMode> mode = std::nullopt,
                  ResponseCallback cb = nullptr);
    void getTimeInfo(ResponseCallback cb = nullptr);
    void getConfig(const QString &key = QString(), ResponseCallback cb = nullptr);

    // ========================================================================
    // Command 请求
    // ========================================================================

    void next(ResponseCallback cb = nullptr);
    void prev(ResponseCallback cb = nullptr);
    void setWallpaper(const QString &path, ResponseCallback cb = nullptr);
    void setMode(Daemon::WallMode mode, ResponseCallback cb = nullptr);
    void lock(const QString &path, ResponseCallback cb = nullptr);
    void unlock(const QString &path, ResponseCallback cb = nullptr);
    void toggleLock(const QString &path, ResponseCallback cb = nullptr);
    void setConfig(const QString &key, const QJsonValue &value,
                   ResponseCallback cb = nullptr);
    void rescan(ResponseCallback cb = nullptr);
    void reloadConfig(ResponseCallback cb = nullptr);
    void shutdown(ResponseCallback cb = nullptr);

    // ========================================================================
    // Subscribe
    // ========================================================================

    void subscribe(const QVector<Daemon::EventType> &events,
                   bool immediateSync = true,
                   ResponseCallback cb = nullptr);
    void unsubscribe(ResponseCallback cb = nullptr);

signals:
    /// 连接状态变化
    void connectionChanged(bool connected);

    /// 收到事件推送
    void eventReceived(const Daemon::DaemonEvent &event);

    /// 收到 immediate_sync 的初始 Status
    void initialStatusReceived(const Daemon::StatusPayload &status);

    /// 连接错误
    void errorOccurred(const QString &message);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onSocketError(QLocalSocket::LocalSocketError error);
    void tryReconnect();

private:
    /// 发送原始请求并注册回调
    void sendRequest(const QByteArray &data, ResponseCallback cb);

    /// 处理一行完整的 JSON 响应
    void processLine(const QByteArray &line);

    /// 重置重连退避
    void resetBackoff();

    /// 尝试启动 daemon 进程（仅一次）
    void tryStartDaemon();

    QLocalSocket *m_socket = nullptr;
    QString m_socketPath;
    QByteArray m_readBuffer;

    // 重连
    QTimer *m_reconnectTimer = nullptr;
    int m_reconnectDelay = 1000;      // 当前重连延迟 (ms)
    static constexpr int kMinReconnectDelay  = 1000;   // 1s
    static constexpr int kMaxReconnectDelay  = 30000;  // 30s
    bool m_autoReconnect = false;

    // daemon 自动拉起
    bool m_daemonStartAttempted = false;  // 每次生命周期只尝试启动一次

    // 请求-响应匹配：FIFO 队列
    // daemon 按顺序处理请求，所以响应也是有序的
    QQueue<ResponseCallback> m_pendingCallbacks;

    // 订阅后等待 immediate_sync 的 Status 响应
    bool m_expectingImmediateStatus = false;
};
