#include "ProcessManager.h"
#include "LianwallClient.h"
#include "Constants.h"

#include <QProcess>
#include <QThread>
#include <QFile>
#include <QDebug>

using namespace LianwallGui;

ProcessManager::ProcessManager(LianwallClient *client, QObject *parent)
    : QObject(parent)
    , m_client(client)
    , m_daemonRunning(false)
{
    // 监听客户端连接状态
    connect(m_client, &LianwallClient::connectedChanged, this, [this]() {
        bool running = m_client->isConnected();
        if (m_daemonRunning != running) {
            m_daemonRunning = running;
            emit daemonRunningChanged();
        }
    });
}

bool ProcessManager::isDaemonRunning() const {
    return m_daemonRunning;
}

bool ProcessManager::ensureDaemonRunning() {
    // 1. 检查 socket 文件是否存在
    if (QFile::exists(Protocol::DEFAULT_SOCKET_PATH)) {
        // 尝试连接
        m_client->connectToDaemon();
        if (m_client->isConnected()) {
            qDebug() << "[ProcessManager] Daemon already running";
            m_daemonRunning = true;
            emit daemonRunningChanged();
            return true;
        }
    }
    
    // 2. 启动 daemon
    qDebug() << "[ProcessManager] Starting daemon...";
    if (!startDaemon()) {
        emit errorOccurred(tr("Failed to start lianwall daemon"));
        return false;
    }
    
    // 3. 等待 socket 可用
    if (!waitForSocket()) {
        emit errorOccurred(tr("Daemon started but socket not available"));
        return false;
    }
    
    // 4. 连接
    m_client->connectToDaemon();
    m_daemonRunning = true;
    emit daemonRunningChanged();
    
    qDebug() << "[ProcessManager] Daemon started successfully";
    return true;
}

bool ProcessManager::startDaemon() {
    // 使用内嵌的 lianwall 二进制文件
    QString lianwallPath = Paths::embeddedLianwall();
    qDebug() << "[ProcessManager] Using lianwall:" << lianwallPath;
    
    // 使用 QProcess::startDetached 启动独立进程
    return QProcess::startDetached(lianwallPath, {"start"});
}

bool ProcessManager::waitForSocket(int timeoutMs) {
    int elapsed = 0;
    const int interval = 100;
    
    while (elapsed < timeoutMs) {
        if (QFile::exists(Protocol::DEFAULT_SOCKET_PATH)) {
            return true;
        }
        QThread::msleep(interval);
        elapsed += interval;
    }
    
    return false;
}

void ProcessManager::gracefulShutdown() {
    qDebug() << "[ProcessManager] Initiating graceful shutdown...";
    
    // 1. 发送 Shutdown 命令
    if (m_client->isConnected()) {
        m_client->shutdown();
        
        // 等待 socket 断开（表示 daemon 已退出）
        int timeout = 5000;
        int elapsed = 0;
        while (m_client->isConnected() && elapsed < timeout) {
            QThread::msleep(100);
            elapsed += 100;
        }
    }
    
    // 2. 确认 swww-daemon 已退出
    waitForProcessExit("swww-daemon");
    
    // 3. 确认 mpvpaper 已退出
    waitForProcessExit("mpvpaper");
    
    m_daemonRunning = false;
    emit daemonRunningChanged();
    emit shutdownComplete();
    
    qDebug() << "[ProcessManager] Graceful shutdown complete";
}

void ProcessManager::restartDaemon() {
    qDebug() << "[ProcessManager] Restarting daemon...";
    
    // 使用内嵌 CLI 的 restart 命令
    QString lianwallPath = Paths::embeddedLianwall();
    QProcess::execute(lianwallPath, {"restart"});
    
    // 等待新的 socket 可用
    QThread::msleep(500);
    waitForSocket();
    
    // 重新连接
    m_client->connectToDaemon();
}

bool ProcessManager::isProcessRunning(const QString &processName) {
    QProcess pgrep;
    pgrep.start("pgrep", {"-x", processName});
    pgrep.waitForFinished();
    return pgrep.exitCode() == 0;
}

void ProcessManager::waitForProcessExit(const QString &processName, int timeoutMs) {
    int elapsed = 0;
    const int interval = 100;
    
    while (elapsed < timeoutMs) {
        if (!isProcessRunning(processName)) {
            return;
        }
        QThread::msleep(interval);
        elapsed += interval;
    }
    
    qWarning() << "[ProcessManager]" << processName << "did not exit within timeout";
}
