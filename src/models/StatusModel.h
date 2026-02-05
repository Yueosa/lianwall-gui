#pragma once

#include <QObject>
#include <QJsonObject>

class LianwallClient;

/**
 * @brief 状态数据模型
 * 
 * 封装 daemon 状态信息，提供 QML 绑定。
 */
class StatusModel : public QObject {
    Q_OBJECT
    
    Q_PROPERTY(QString mode READ mode NOTIFY statusChanged)
    Q_PROPERTY(QString current READ current NOTIFY statusChanged)
    Q_PROPERTY(QString currentFilename READ currentFilename NOTIFY statusChanged)
    Q_PROPERTY(QString engine READ engine NOTIFY statusChanged)
    Q_PROPERTY(int totalWallpapers READ totalWallpapers NOTIFY statusChanged)
    Q_PROPERTY(int lockedCount READ lockedCount NOTIFY statusChanged)
    Q_PROPERTY(int availableCount READ availableCount NOTIFY statusChanged)
    Q_PROPERTY(int scannedCount READ scannedCount NOTIFY statusChanged)
    Q_PROPERTY(int vramUsedMb READ vramUsedMb NOTIFY statusChanged)
    Q_PROPERTY(int vramTotalMb READ vramTotalMb NOTIFY statusChanged)
    Q_PROPERTY(int uptimeSecs READ uptimeSecs NOTIFY statusChanged)
    Q_PROPERTY(QString uptimeFormatted READ uptimeFormatted NOTIFY statusChanged)
    Q_PROPERTY(QString nextTimePoint READ nextTimePoint NOTIFY statusChanged)
    Q_PROPERTY(int timePointsCount READ timePointsCount NOTIFY statusChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)

public:
    explicit StatusModel(QObject *parent = nullptr);

    Q_INVOKABLE void setClient(LianwallClient *client);

    // Property getters
    QString mode() const { return m_mode; }
    QString current() const { return m_current; }
    QString currentFilename() const;
    QString engine() const { return m_engine; }
    int totalWallpapers() const { return m_totalWallpapers; }
    int lockedCount() const { return m_lockedCount; }
    int availableCount() const { return m_availableCount; }
    int scannedCount() const { return m_scannedCount; }
    int vramUsedMb() const { return m_vramUsedMb; }
    int vramTotalMb() const { return m_vramTotalMb; }
    int uptimeSecs() const { return m_uptimeSecs; }
    QString uptimeFormatted() const;
    QString nextTimePoint() const { return m_nextTimePoint; }
    int timePointsCount() const { return m_timePointsCount; }
    bool connected() const { return m_connected; }

    Q_INVOKABLE void refresh();

signals:
    void statusChanged();
    void connectedChanged();

private slots:
    void onStatusReceived(const QJsonObject &status);
    void onConnectedChanged();

private:
    LianwallClient *m_client;
    
    QString m_mode;
    QString m_current;
    QString m_engine;
    int m_totalWallpapers;
    int m_lockedCount;
    int m_availableCount;
    int m_scannedCount;
    int m_vramUsedMb;
    int m_vramTotalMb;
    int m_uptimeSecs;
    QString m_nextTimePoint;
    int m_timePointsCount;
    bool m_connected;
};
