#include "DaemonClient.h"
#include "Constants.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QPointer>
#include <QDebug>
#include <QFileInfo>
#include <memory>
#include <utility>

// ============================================================================
// 构造 / 析构
// ============================================================================

DaemonClient::DaemonClient(QObject *parent, int requestTimeoutMs, QString daemonExecutable)
    : QObject(parent)
    , m_socket(new QLocalSocket(this))
    , m_socketPath(QStringLiteral("/tmp/lianwall.sock"))
    , m_daemonExecutable(std::move(daemonExecutable))
    , m_reconnectTimer(new QTimer(this))
    , m_requestTimer(new QTimer(this))
    , m_requestTimeoutMs(qMax(1, requestTimeoutMs))
{
    m_reconnectTimer->setSingleShot(true);
    m_clock.start();
    m_requestTimer->setInterval(qMin(250, m_requestTimeoutMs));
    connect(m_requestTimer, &QTimer::timeout, this, &DaemonClient::checkRequestTimeouts);

    m_socket->setReadBufferSize(64 * 1024);
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
    m_requestTimer->stop();
    // Owners may already be destructing: never invoke their callbacks here.
    m_socket->disconnect(this);
    m_socket->abort();
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
    resetConnection(QStringLiteral("connection_lost"), QStringLiteral("Disconnected from daemon"));
}

void DaemonClient::setAutoReconnect(bool enabled)
{
    m_autoReconnect = enabled;
    if (!enabled)
        m_reconnectTimer->stop();
}

// ============================================================================
// 槽函数
// ============================================================================

void DaemonClient::onConnected()
{
    qDebug() << "[DaemonClient] Connected to" << m_socketPath;
    resetBackoff();
    // Auto-start is only for the initial connection, never for a daemon the user stopped.
    m_daemonStartAttempted = true;
    emit connectionChanged(true);
}

void DaemonClient::onDisconnected()
{
    auto pending = takePendingRequests();
    emit connectionChanged(false);
    if (m_autoReconnect) {
        m_reconnectTimer->start(m_reconnectDelay);
        m_reconnectDelay = qMin(m_reconnectDelay * 2, kMaxReconnectDelay);
    }
    // Remove every entry before calling user code, which may send another request.
    failRequests(pending, QStringLiteral("connection_lost"), QStringLiteral("Connection to daemon lost"));
}

void DaemonClient::onReadyRead()
{
    // Bound each frame, while allowing many small frames in a single socket burst.
    constexpr qsizetype maxFrameBytes = LianwallGui::Protocol::MAX_RESPONSE_SIZE;
    while (m_socket->bytesAvailable() > 0) {
        m_readBuffer.append(m_socket->read(64 * 1024));
        int pos;
        while ((pos = m_readBuffer.indexOf('\n')) >= 0) {
            if (pos > maxFrameBytes) {
                resetConnection(QStringLiteral("protocol_error"), QStringLiteral("Response exceeds 64 MiB frame limit"));
                return;
            }
            const QByteArray line = m_readBuffer.left(pos);
            m_readBuffer.remove(0, pos + 1);
            const QPointer<DaemonClient> guard(this);
            if (!line.isEmpty())
                processLine(line);
            if (!guard || !guard->isConnected())
                return;
        }
        if (m_readBuffer.size() > maxFrameBytes) {
            resetConnection(QStringLiteral("protocol_error"), QStringLiteral("Response exceeds 64 MiB frame limit"));
            return;
        }
    }
}

QHash<quint64, DaemonClient::PendingRequest> DaemonClient::takePendingRequests()
{
    auto pending = std::move(m_pendingCallbacks);
    m_pendingCallbacks.clear();
    m_pendingOrder.clear();
    m_readBuffer.clear();
    m_expectedStatusCount = 0;
    m_syncDeadline = 0;
    m_requestTimer->stop();
    return pending;
}

void DaemonClient::failRequests(const QHash<quint64, PendingRequest> &pending,
                               const QString &code, const QString &message)
{
    Daemon::Response response;
    response.type = Daemon::ResponseType::Error;
    response.payload = {{QStringLiteral("code"), code}, {QStringLiteral("message"), message}};
    for (const auto &request : pending) {
        if (request.callback)
            request.callback(response);
    }
}

void DaemonClient::resetConnection(const QString &code, const QString &message)
{
    auto pending = takePendingRequests();
    // Abort before callbacks so reentrant requests cannot enter the failed session.
    // Reconnect also prevents late FIFO replies from being assigned to new requests.
    m_socket->abort();
    emit errorOccurred(message);
    failRequests(pending, code, message);
}

void DaemonClient::checkRequestTimeouts()
{
    const qint64 now = m_clock.elapsed();
    bool expired = m_expectedStatusCount > 0 && now >= m_syncDeadline;
    for (const auto &request : std::as_const(m_pendingCallbacks))
        expired = expired || now >= request.deadline;
    if (expired) {
        resetConnection(QStringLiteral("timeout"), QStringLiteral("Request timed out; operation result may be unknown"));
    } else if (m_pendingCallbacks.isEmpty() && m_expectedStatusCount == 0) {
        m_requestTimer->stop();
    }
}

void DaemonClient::onSocketError(QLocalSocket::LocalSocketError error)
{
    Q_UNUSED(error)
    QString msg = m_socket->errorString();
    qDebug() << "[DaemonClient] Socket error:" << msg
             << "path=" << m_socketPath
             << "state=" << m_socket->state();
    emit errorOccurred(msg);

    // 首次连接失败时尝试自动拉起 daemon
    if (!m_daemonStartAttempted && m_autoReconnect) {
        tryStartDaemon();
    }

    // 连接失败后安排重连（onDisconnected 只在已连接后断开时触发）
    if (m_autoReconnect && !m_reconnectTimer->isActive()) {
        qDebug() << "[DaemonClient] Scheduling reconnect in" << m_reconnectDelay << "ms";
        m_reconnectTimer->start(m_reconnectDelay);
        m_reconnectDelay = qMin(m_reconnectDelay * 2, kMaxReconnectDelay);
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
            errResp.payload = QJsonObject{
                {QStringLiteral("code"), QStringLiteral("connection_lost")},
                {QStringLiteral("message"), QStringLiteral("Not connected to daemon")},
            };
            cb(errResp);
        }
        return;
    }

    quint64 id = nextRequestId();
    auto obj = QJsonDocument::fromJson(data).object();
    obj.insert(QStringLiteral("id"), static_cast<double>(id));

    if (m_pendingCallbacks.size() >= 64) {
        PendingRequest rejected{std::move(cb), 0, false};
        failRequests({{id, rejected}}, QStringLiteral("client_busy"), QStringLiteral("Too many pending requests; request was not sent"));
        return;
    }
    const QByteArray frame = QJsonDocument(obj).toJson(QJsonDocument::Compact) + '\n';
    if (frame.size() - 1 > LianwallGui::Protocol::MAX_REQUEST_SIZE) {
        PendingRequest rejected{std::move(cb), 0, false};
        failRequests({{id, rejected}}, QStringLiteral("request_too_large"), QStringLiteral("Request exceeds 1 MiB frame limit; request was not sent"));
        return;
    }
    const QString command = obj.value(QStringLiteral("cmd")).toString();
    const bool wallpaper = command == QLatin1String("Next") || command == QLatin1String("Prev")
        || command == QLatin1String("SetMode") || command == QLatin1String("SetWallpaper");
    const qint64 timeout = static_cast<qint64>(m_requestTimeoutMs) * (wallpaper ? 6 : 1);
    const bool sync = command == QLatin1String("Subscribe")
        && obj.value(QStringLiteral("immediate_sync")).toBool();
    static const QHash<QString, Daemon::ResponseType> queryTypes = {
        {QStringLiteral("Ping"), Daemon::ResponseType::Pong},
        {QStringLiteral("GetStatus"), Daemon::ResponseType::Status},
        {QStringLiteral("GetSpace"), Daemon::ResponseType::Space},
        {QStringLiteral("GetTimeInfo"), Daemon::ResponseType::TimeInfo},
        {QStringLiteral("GetConfig"), Daemon::ResponseType::Config},
        {QStringLiteral("GetRotationStatus"), Daemon::ResponseType::RotationStatus},
        {QStringLiteral("Subscribe"), Daemon::ResponseType::Subscribed},
        {QStringLiteral("Unsubscribe"), Daemon::ResponseType::Unsubscribed},
    };
    const auto expected = queryTypes.value(command, Daemon::ResponseType::Ok);
    m_pendingCallbacks.insert(id, PendingRequest{std::move(cb), m_clock.elapsed() + timeout, sync, expected});
    m_pendingOrder.enqueue(id);
    if (!m_requestTimer->isActive())
        m_requestTimer->start();
    if (m_socket->write(frame) != frame.size()) {
        resetConnection(QStringLiteral("connection_lost"), QStringLiteral("Failed to send request"));
        return;
    }
    m_socket->flush();
}

// ============================================================================
// 响应处理
// ============================================================================

void DaemonClient::processLine(const QByteArray &line)
{
    const auto parsed = Daemon::Response::fromLine(line);
    if (!parsed || parsed->type == Daemon::ResponseType::Unknown) {
        resetConnection(QStringLiteral("protocol_error"), QStringLiteral("Invalid daemon response"));
        return;
    }
    const auto &response = *parsed;
    const auto root = QJsonDocument::fromJson(line).object();
    if (response.type == Daemon::ResponseType::Event) {
        emit eventReceived(response.asEvent());
        return;
    }
    const bool hasId = root.contains(QStringLiteral("id"));
    const qint64 wireId = hasId ? root.value(QStringLiteral("id")).toInteger(-1) : 0;
    if (wireId < 0) {
        resetConnection(QStringLiteral("protocol_error"), QStringLiteral("Invalid response id"));
        return;
    }
    // A tagged GetStatus response must never be consumed as initial subscription sync.
    if (response.type == Daemon::ResponseType::Status && wireId == 0
        && (hasId || m_expectedStatusCount > 0)) {
        if (m_expectedStatusCount > 0)
            --m_expectedStatusCount;
        emit initialStatusReceived(response.asStatus());
        return;
    }
    quint64 id = static_cast<quint64>(wireId);
    if (hasId) {
        if (id == 0) {
            resetConnection(QStringLiteral("protocol_error"), QStringLiteral("Unmatched daemon response"));
            return;
        }
        if (!m_pendingCallbacks.contains(id)) {
            qWarning() << "[DaemonClient] Ignoring late or unknown response id:" << id;
            return;
        }
        m_pendingOrder.removeAll(id);
    } else {
        // Legacy daemons omit ids; reset the session on timeout to keep FIFO safe.
        if (m_pendingOrder.isEmpty())
            return;
        id = m_pendingOrder.dequeue();
    }
    const PendingRequest request = m_pendingCallbacks.value(id);
    if (response.type != Daemon::ResponseType::Error && response.type != request.expected) {
        resetConnection(QStringLiteral("protocol_error"), QStringLiteral("Unexpected daemon response type"));
        return;
    }
    if (response.type == Daemon::ResponseType::Error
        && response.payload.value(QStringLiteral("code")).toString() == QLatin1String("timeout")) {
        resetConnection(QStringLiteral("timeout"), response.asError().message);
        return;
    }
    m_pendingCallbacks.remove(id);
    if (response.type == Daemon::ResponseType::Subscribed && request.immediateSync) {
        ++m_expectedStatusCount;
        m_syncDeadline = m_clock.elapsed() + m_requestTimeoutMs;
    }
    if (request.callback)
        request.callback(response);
}

void DaemonClient::resetBackoff()
{
    m_reconnectDelay = kMinReconnectDelay;
}

void DaemonClient::resetStartAttempt()
{
    m_daemonStartAttempted = false;
}

bool DaemonClient::tryStartDaemon()
{
    m_daemonStartAttempted = true;

    using namespace LianwallGui;
    const QString exe = m_daemonExecutable.isEmpty() ? Paths::findLianwalld() : m_daemonExecutable;
    qDebug() << "[DaemonClient] Daemon not running, attempting to start:" << exe;

    // 以分离进程启动 daemon（daemon 自身会 daemonize）
    bool ok = QProcess::startDetached(exe, {});
    if (ok) {
        qDebug() << "[DaemonClient] lianwalld launched, waiting for socket...";
    } else {
        qWarning() << "[DaemonClient] Failed to start lianwalld at" << exe;
    }
    return ok;
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

void DaemonClient::getRotationStatus(ResponseCallback cb)
{
    sendRequest(Daemon::Request::getRotationStatus(), std::move(cb));
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

void DaemonClient::pauseRotation(ResponseCallback cb)
{
    sendRequest(Daemon::Request::pauseRotation(), std::move(cb));
}

void DaemonClient::resumeRotation(ResponseCallback cb)
{
    sendRequest(Daemon::Request::resumeRotation(), std::move(cb));
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

void DaemonClient::restartDaemon(ResponseCallback cb)
{
    if (m_restarting) {
        Daemon::Response response;
        response.type = Daemon::ResponseType::Error;
        response.payload = {{"code", "client_busy"}, {"message", "Daemon restart already in progress"}};
        if (cb) cb(response);
        return;
    }
    m_restarting = true;
    const QPointer<DaemonClient> guard(this);
    shutdown([guard, cb = std::move(cb)](const Daemon::Response &response) {
        if (!guard) return;
        if (response.type != Daemon::ResponseType::Ok) {
            guard->m_restarting = false;
            if (cb) cb(response);
            return;
        }
        guard->m_autoReconnect = false;
        guard->m_reconnectTimer->stop();
        guard->m_socket->abort();
        if (!guard) return;
        auto *timer = new QTimer(guard);
        timer->setInterval(100);
        auto elapsed = std::make_shared<QElapsedTimer>();
        elapsed->start();
        QObject::connect(timer, &QTimer::timeout, guard, [guard, timer, elapsed, cb]() {
            if (!guard) return;
            const bool stopped = !QFileInfo::exists(guard->m_socketPath);
            if (!stopped && elapsed->elapsed() < 15000) return;
            timer->stop();
            timer->deleteLater();
            guard->m_restarting = false;
            Daemon::Response result;
            result.type = Daemon::ResponseType::Error;
            result.payload = {{"code", "restart_failed"}, {"message", "Daemon did not finish shutdown within 15 seconds"}};
            if (stopped) {
                guard->resetStartAttempt();
                guard->resetBackoff();
                if (guard->tryStartDaemon()) {
                    result.type = Daemon::ResponseType::Ok;
                    result.payload = {};
                } else {
                    result.payload = {{"code", "restart_failed"}, {"message", "Failed to launch daemon"}};
                }
            }
            guard->connectToDaemon();
            if (cb) cb(result);
        });
        timer->start();
    });
}
