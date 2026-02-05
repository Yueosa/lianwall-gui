#include "LianwallClient.h"
#include "Constants.h"

#include <QJsonDocument>
#include <QTimer>
#include <QtEndian>
#include <QDebug>

using namespace LianwallGui;

LianwallClient::LianwallClient(QObject *parent)
    : QObject(parent)
    , m_socket(new QLocalSocket(this))
    , m_socketPath(Protocol::DEFAULT_SOCKET_PATH)
    , m_autoReconnect(true)
{
    connect(m_socket, &QLocalSocket::connected, this, &LianwallClient::onConnected);
    connect(m_socket, &QLocalSocket::disconnected, this, &LianwallClient::onDisconnected);
    connect(m_socket, &QLocalSocket::readyRead, this, &LianwallClient::onReadyRead);
    connect(m_socket, &QLocalSocket::errorOccurred, this, &LianwallClient::onError);
}

LianwallClient::~LianwallClient() {
    disconnectFromDaemon();
}

bool LianwallClient::isConnected() const {
    return m_socket->state() == QLocalSocket::ConnectedState;
}

// ============================================================================
// 连接管理
// ============================================================================

void LianwallClient::connectToDaemon(const QString &socketPath) {
    if (!socketPath.isEmpty()) {
        m_socketPath = socketPath;
    }
    
    if (m_socket->state() != QLocalSocket::UnconnectedState) {
        m_socket->disconnectFromServer();
    }
    
    m_socket->connectToServer(m_socketPath);
}

void LianwallClient::disconnectFromDaemon() {
    m_autoReconnect = false;
    if (m_socket->state() != QLocalSocket::UnconnectedState) {
        m_socket->disconnectFromServer();
    }
}

void LianwallClient::onConnected() {
    qDebug() << "[LianwallClient] Connected to daemon";
    m_autoReconnect = true;
    emit connectedChanged();
}

void LianwallClient::onDisconnected() {
    qDebug() << "[LianwallClient] Disconnected from daemon";
    emit connectedChanged();
    
    if (m_autoReconnect) {
        tryReconnect();
    }
}

void LianwallClient::onError(QLocalSocket::LocalSocketError error) {
    QString errorStr;
    switch (error) {
        case QLocalSocket::ConnectionRefusedError:
            errorStr = tr("Connection refused - daemon not running?");
            break;
        case QLocalSocket::ServerNotFoundError:
            errorStr = tr("Server not found - socket file missing");
            break;
        case QLocalSocket::SocketTimeoutError:
            errorStr = tr("Connection timeout");
            break;
        default:
            errorStr = m_socket->errorString();
    }
    
    qWarning() << "[LianwallClient] Socket error:" << errorStr;
    emit errorOccurred(errorStr);
    
    if (m_autoReconnect && error != QLocalSocket::ConnectionRefusedError) {
        tryReconnect();
    }
}

void LianwallClient::tryReconnect() {
    QTimer::singleShot(Protocol::CONNECT_TIMEOUT_MS, this, [this]() {
        if (!isConnected() && m_autoReconnect) {
            qDebug() << "[LianwallClient] Attempting reconnect...";
            connectToDaemon();
        }
    });
}

// ============================================================================
// 消息收发
// ============================================================================

void LianwallClient::sendRequest(const QString &cmd) {
    QJsonObject request;
    request["cmd"] = cmd;
    sendRequest(request);
}

void LianwallClient::sendRequest(const QString &cmd, const QJsonObject &data) {
    QJsonObject request;
    request["cmd"] = cmd;
    request["data"] = data;
    sendRequest(request);
}

void LianwallClient::sendRequest(const QJsonObject &request) {
    if (!isConnected()) {
        emit errorOccurred(tr("Not connected to daemon"));
        return;
    }
    
    m_lastCommand = request["cmd"].toString();
    
    // 序列化 JSON
    QByteArray json = QJsonDocument(request).toJson(QJsonDocument::Compact);
    
    // 构建帧: 4字节长度前缀 (big-endian) + JSON
    quint32 len = static_cast<quint32>(json.size());
    QByteArray header(4, 0);
    qToBigEndian(len, header.data());
    
    m_socket->write(header + json);
    m_socket->flush();
    
    qDebug() << "[LianwallClient] Sent:" << m_lastCommand;
}

void LianwallClient::onReadyRead() {
    m_buffer.append(m_socket->readAll());
    
    // 尝试解析完整消息
    while (m_buffer.size() >= 4) {
        // 读取长度前缀
        quint32 len = qFromBigEndian<quint32>(m_buffer.constData());
        
        if (len > Protocol::MAX_MESSAGE_SIZE) {
            qWarning() << "[LianwallClient] Message too large:" << len;
            m_buffer.clear();
            return;
        }
        
        // 检查是否有完整消息
        if (m_buffer.size() < static_cast<qsizetype>(4 + len)) {
            return;  // 等待更多数据
        }
        
        // 提取 JSON 数据
        QByteArray json = m_buffer.mid(4, len);
        m_buffer.remove(0, 4 + len);
        
        processResponse(json);
    }
}

void LianwallClient::processResponse(const QByteArray &data) {
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "[LianwallClient] JSON parse error:" << error.errorString();
        emit errorOccurred(tr("Invalid response from daemon"));
        return;
    }
    
    QJsonObject response = doc.object();
    bool success = response["success"].toBool();
    
    if (!success) {
        QString errorMsg = response["error"].toString();
        qWarning() << "[LianwallClient] Operation failed:" << errorMsg;
        emit operationFailed(m_lastCommand, errorMsg);
        return;
    }
    
    // 解析响应数据
    QJsonObject dataObj = response["data"].toObject();
    QString type = dataObj["type"].toString();
    QJsonObject payload = dataObj["payload"].toObject();
    
    if (type == "Pong") {
        emit pongReceived();
    } else if (type == "Status") {
        emit statusReceived(payload);
    } else if (type == "Space") {
        emit spaceReceived(payload);
    } else if (type == "TimeInfo") {
        emit timeInfoReceived(payload);
    } else if (type == "Ok") {
        emit operationSucceeded(m_lastCommand);
    }
    
    qDebug() << "[LianwallClient] Received:" << type;
}

// ============================================================================
// 命令方法
// ============================================================================

void LianwallClient::ping() {
    sendRequest("Ping");
}

void LianwallClient::getStatus() {
    sendRequest("Status");
}

void LianwallClient::getSpace() {
    sendRequest("GetSpace");
}

void LianwallClient::getTimeInfo() {
    sendRequest("GetTimeInfo");
}

void LianwallClient::next() {
    sendRequest("Next");
}

void LianwallClient::previous() {
    sendRequest("Previous");
}

void LianwallClient::setWallpaper(const QString &path) {
    QJsonObject data;
    data["path"] = path;
    sendRequest("SetWallpaper", data);
}

void LianwallClient::setMode(const QString &mode) {
    QJsonObject data;
    data["mode"] = mode;
    sendRequest("SetMode", data);
}

void LianwallClient::lock(const QString &path) {
    QJsonObject data;
    data["path"] = path;
    sendRequest("Lock", data);
}

void LianwallClient::unlock(const QString &path) {
    QJsonObject data;
    data["path"] = path;
    sendRequest("Unlock", data);
}

void LianwallClient::reload() {
    sendRequest("Reload");
}

void LianwallClient::shutdown() {
    m_autoReconnect = false;
    sendRequest("Shutdown");
}
