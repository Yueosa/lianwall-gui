#pragma once

#include <QObject>
#include <QProcess>

class LianwallClient;

/**
 * @brief 进程管理器
 * 
 * 负责管理 lianwall daemon 的生命周期：
 * - 启动时检测并启动 daemon
 * - 关闭时优雅退出 daemon, swww-daemon, mpvpaper
 */
class ProcessManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool daemonRunning READ isDaemonRunning NOTIFY daemonRunningChanged)

public:
    explicit ProcessManager(LianwallClient *client, QObject *parent = nullptr);

    bool isDaemonRunning() const;

    /// 确保 daemon 正在运行（启动时调用）
    Q_INVOKABLE bool ensureDaemonRunning();

    /// 优雅关闭所有进程
    Q_INVOKABLE void gracefulShutdown();

    /// 重启 daemon
    Q_INVOKABLE void restartDaemon();

    /// 检查进程是否在运行
    Q_INVOKABLE bool isProcessRunning(const QString &processName);

signals:
    void daemonRunningChanged();
    void shutdownComplete();
    void errorOccurred(const QString &error);

private:
    bool startDaemon();
    bool waitForSocket(int timeoutMs = 3000);
    void waitForProcessExit(const QString &processName, int timeoutMs = 2000);

    LianwallClient *m_client;
    bool m_daemonRunning;
};
