#include "DaemonClient.h"
#include "Constants.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QDebug>

// ============================================================================
// 构造 / 析构
// ============================================================================

DaemonClient::DaemonClient(QObject *parent)
    : QObject(parent)
    , m_socket(new QLocalSocket(this))
    , m_socketPath(QStringLiteral("/tmp/lianwall.sock"))
    , m_reconnectTimer(new QTimer(this))
{
    m_reconnectTimer->setSingleShot(true);

    connect(m_socket, &QLocalSocket::connected,
            this, &DaemonClient::onConnected);
    connect(m_socket, &QLocalSocket::disconnected,
            this, &DaemonClient::onDisconnected);
    connect(m_socket, &QLocalSocket::readyRead,
            this, &DaemonClient::onReadyRead);
    connect(m_socket, &QLocalSocket::errorOccurred,
            this, &DaemonClient::onSocketError);
    connect(m_reconnectTimer, &QTimer::timeout,
            this, &DaemonClient::tryReconnect);
}

DaemonClient::~DaemonClient()
{
    m_autoReconnect = false;
    m_reconnectTimer->stop();
    if (m_socket->state() != QLocalSocket::UnconnectedState)
        m_socket->disconnectFromServer();
}

// ============================================================================
// 属性
// ============================================================================

bool DaemonClient::isConnected() const
{
    return m_socket->state() == QLocalSocket::ConnectedState;
}

void DaemonClient::setSocketPath(const QString &path)
{
    m_socketPath = path;
}

// ============================================================================
// 连接管理
// ============================================================================

void DaemonClient::connectToDaemon()
{
    m_autoReconnect = true;
    resetBackoff();

    if (m_socket->state() == QLocalSocket::UnconnectedState)
        m_socket->connectToServer(m_socketPath);
}

void DaemonClient::disconnectFromDaemon()
{
    m_autoReconnect = false;
    m_reconnectTimer->stop();
    m_pendingCallbacks.clear();
    m_readBuffer.clear();
    m_expectingImmediateStatus = false;

    if (m_socket->state() != QLocalSocket::UnconnectedState)
        m_socket->disconnectFromServer();
}

// ============================================================================
// 槽函数
// ============================================================================

void DaemonClient::onConnected()
{
    qDebug() << "[DaemonClient] Connected to" << m_socketPath;
    resetBackoff();
    emit connectionChanged(true);
}

void DaemonClient::onDisconnected()
{
    qDebug() << "[DaemonClient] Disconnected";

    // 清空未完成的回调
    while (!m_pendingCallbacks.isEmpty()) {
        auto cb = m_pendingCallbacks.dequeue();
        if (cb) {
            Daemon::Response errResp;
            errResp.type = Daemon::ResponseType::Error;
            cb(errResp);
        }
    }
    m_readBuffer.clear();
    m_expectingImmediateStatus = false;

    emit connectionChanged(false);

    // 自动重连
    if (m_autoReconnect) {
        qDebug() << "[DaemonClient] Reconnecting in" << m_reconnectDelay << "ms";
        m_reconnectTimer->start(m_reconnectDelay);
        // 指数退避
        m_reconnectDelay = qMin(m_reconnectDelay * 2, kMaxReconnectDelay);
    }
}

void DaemonClient::onReadyRead()
{
    m_readBuffer.append(m_socket->readAll());

    // 按行分割
    int pos;
    while ((pos = m_readBuffer.indexOf('\n')) >= 0) {
        QByteArray line = m_readBuffer.left(pos);
        m_readBuffer.remove(0, pos + 1);

        if (!line.isEmpty())
            processLine(line);
    }
}

void DaemonClient::onSocketError(QLocalSocket::LocalSocketError error)
{
    Q_UNUSED(error)
    QString msg = m_socket->errorString();
    qDebug() << "[DaemonClient] Socket error:" << msg;
    emit errorOccurred(msg);

    // 首次连接失败时尝试自动拉起 daemon
    if (!m_daemonStartAttempted && m_autoReconnect) {
        tryStartDaemon();
    }
}

void DaemonClient::tryReconnect()
{
    if (!m_autoReconnect)
        return;

    if (m_socket->state() == QLocalSocket::UnconnectedState)
        m_socket->connectToServer(m_socketPath);
}

// ============================================================================
// 请求发送
// ============================================================================

void DaemonClient::sendRequest(const QByteArray &data, ResponseCallback cb)
{
    if (!isConnected()) {
        if (cb) {
            Daemon::Response errResp;
            errResp.type = Daemon::ResponseType::Error;
            cb(errResp);
        }
        return;
    }

    m_pendingCallbacks.enqueue(cb);
    m_socket->write(data);
    m_socket->flush();
}

// ============================================================================
// 响应处理
// ============================================================================

void DaemonClient::processLine(const QByteArray &line)
{
    auto resp = Daemon::Response::fromLine(line);
    if (!resp.has_value()) {
        qWarning() << "[DaemonClient] Failed to parse response:" << line.left(200);
        return;
    }

    const auto &r = *resp;

    // 事件推送（不消耗 pending callback）
    if (r.type == Daemon::ResponseType::Event) {
        auto event = r.asEvent();
        emit eventReceived(event);
        return;
    }

    // immediate_sync 的 Status 推送（subscribe 后额外推送的一条 Status）
    if (m_expectingImmediateStatus && r.type == Daemon::ResponseType::Status) {
        m_expectingImmediateStatus = false;
        emit initialStatusReceived(r.asStatus());
        return;
    }

    // 普通请求-响应匹配
    if (!m_pendingCallbacks.isEmpty()) {
        auto cb = m_pendingCallbacks.dequeue();

        // 如果是 Subscribed 响应且 immediate_sync，标记等待后续 Status
        if (r.type == Daemon::ResponseType::Subscribed) {
            m_expectingImmediateStatus = true;
        }

        if (cb)
            cb(r);
    } else {
        qWarning() << "[DaemonClient] Unexpected response (no pending callback):"
                    << static_cast<int>(r.type);
    }
}

void DaemonClient::resetBackoff()
{
    m_reconnectDelay = kMinReconnectDelay;
}

void DaemonClient::tryStartDaemon()
{
    m_daemonStartAttempted = true;

    using namespace LianwallGui;
    const QString exe = Paths::findLianwalld();
    qDebug() << "[DaemonClient] Daemon not running, attempting to start:" << exe;

    // 以分离进程启动 daemon（daemon 自身会 daemonize）
    bool ok = QProcess::startDetached(exe, {});
    if (ok) {
        qDebug() << "[DaemonClient] lianwalld launched, waiting for socket...";
    } else {
        qWarning() << "[DaemonClient] Failed to start lianwalld at" << exe;
    }
}

// ============================================================================
// Query API
// ============================================================================

void DaemonClient::ping(ResponseCallback cb)
{
    sendRequest(Daemon::Request::ping(), std::move(cb));
}

void DaemonClient::getStatus(ResponseCallback cb)
{
    sendRequest(Daemon::Request::getStatus(), std::move(cb));
}

void DaemonClient::getSpace(std::optional<Daemon::WallMode> mode, ResponseCallback cb)
{
    sendRequest(Daemon::Request::getSpace(mode), std::move(cb));
}

void DaemonClient::getTimeInfo(ResponseCallback cb)
{
    sendRequest(Daemon::Request::getTimeInfo(), std::move(cb));
}

void DaemonClient::getConfig(const QString &key, ResponseCallback cb)
{
    sendRequest(Daemon::Request::getConfig(key), std::move(cb));
}

// ============================================================================
// Command API
// ============================================================================

void DaemonClient::next(ResponseCallback cb)
{
    sendRequest(Daemon::Request::next(), std::move(cb));
}

void DaemonClient::prev(ResponseCallback cb)
{
    sendRequest(Daemon::Request::prev(), std::move(cb));
}

void DaemonClient::setWallpaper(const QString &path, ResponseCallback cb)
{
    sendRequest(Daemon::Request::setWallpaper(path), std::move(cb));
}

void DaemonClient::setMode(Daemon::WallMode mode, ResponseCallback cb)
{
    sendRequest(Daemon::Request::setMode(mode), std::move(cb));
}

void DaemonClient::lock(const QString &path, ResponseCallback cb)
{
    sendRequest(Daemon::Request::lock(path), std::move(cb));
}

void DaemonClient::unlock(const QString &path, ResponseCallback cb)
{
    sendRequest(Daemon::Request::unlock(path), std::move(cb));
}

void DaemonClient::toggleLock(const QString &path, ResponseCallback cb)
{
    sendRequest(Daemon::Request::toggleLock(path), std::move(cb));
}

void DaemonClient::setConfig(const QString &key, const QJsonValue &value,
                              ResponseCallback cb)
{
    sendRequest(Daemon::Request::setConfig(key, value), std::move(cb));
}

void DaemonClient::rescan(ResponseCallback cb)
{
    sendRequest(Daemon::Request::rescan(), std::move(cb));
}

void DaemonClient::reloadConfig(ResponseCallback cb)
{
    sendRequest(Daemon::Request::reloadConfig(), std::move(cb));
}

void DaemonClient::shutdown(ResponseCallback cb)
{
    sendRequest(Daemon::Request::shutdown(), std::move(cb));
}

// ============================================================================
// Subscribe API
// ============================================================================

void DaemonClient::subscribe(const QVector<Daemon::EventType> &events,
                              bool immediateSync,
                              ResponseCallback cb)
{
    // immediate_sync 标记将在收到 Subscribed 响应时设置
    sendRequest(Daemon::Request::subscribe(events, immediateSync), std::move(cb));
}

void DaemonClient::unsubscribe(ResponseCallback cb)
{
    sendRequest(Daemon::Request::unsubscribe(), std::move(cb));
}
