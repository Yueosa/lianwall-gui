#pragma once

#include <QObject>
#include <QLocalSocket>
#include <QJsonObject>
#include <QByteArray>

/**
 * @brief LianWall Daemon Socket 客户端
 * 
 * 负责与 lianwall daemon 通过 Unix Socket 通信。
 * 协议格式: 4字节长度前缀 (big-endian) + JSON 数据
 */
class LianwallClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)

public:
    explicit LianwallClient(QObject *parent = nullptr);
    ~LianwallClient();

    bool isConnected() const;

    // === 状态查询 ===
    Q_INVOKABLE void ping();
    Q_INVOKABLE void getStatus();
    Q_INVOKABLE void getSpace();
    Q_INVOKABLE void getTimeInfo();

    // === 壁纸控制 ===
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void setWallpaper(const QString &path);
    Q_INVOKABLE void setMode(const QString &mode);
    Q_INVOKABLE void lock(const QString &path);
    Q_INVOKABLE void unlock(const QString &path);
    Q_INVOKABLE void reload();

    // === 生命周期 ===
    Q_INVOKABLE void shutdown();

public slots:
    void connectToDaemon(const QString &socketPath = QString());
    void disconnectFromDaemon();

signals:
    void connectedChanged();
    void errorOccurred(const QString &error);

    // 具体响应信号
    void pongReceived();
    void statusReceived(const QJsonObject &status);
    void spaceReceived(const QJsonObject &space);
    void timeInfoReceived(const QJsonObject &timeInfo);
    void operationSucceeded(const QString &command);
    void operationFailed(const QString &command, const QString &error);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QLocalSocket::LocalSocketError error);

private:
    void sendRequest(const QJsonObject &request);
    void sendRequest(const QString &cmd);
    void sendRequest(const QString &cmd, const QJsonObject &data);
    void processResponse(const QByteArray &data);
    void tryReconnect();

    QLocalSocket *m_socket;
    QByteArray m_buffer;
    QString m_socketPath;
    QString m_lastCommand;
    bool m_autoReconnect;
};
