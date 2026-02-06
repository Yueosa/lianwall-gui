#pragma once

/// @file DaemonState.h
/// @brief 响应式状态聚合（QML 绑定层）
///
/// 持有从 daemon 获取的所有运行时数据。
/// 监听 DaemonClient 事件自动更新（零轮询）。
/// 所有属性对 QML 只读，由 daemon 事件驱动变化。

#include "DaemonTypes.h"

#include <QObject>
#include <QString>

class DaemonClient;

class DaemonState : public QObject
{
    Q_OBJECT

    // === 连接状态 ===
    Q_PROPERTY(bool daemonConnected READ daemonConnected NOTIFY daemonConnectedChanged)

    // === 基本状态 ===
    Q_PROPERTY(QString mode READ mode NOTIFY modeChanged)
    Q_PROPERTY(QString currentPath READ currentPath NOTIFY currentPathChanged)
    Q_PROPERTY(QString currentFilename READ currentFilename NOTIFY currentFilenameChanged)
    Q_PROPERTY(QString engine READ engine NOTIFY engineChanged)

    // === 壁纸统计 ===
    Q_PROPERTY(int totalWallpapers READ totalWallpapers NOTIFY totalWallpapersChanged)
    Q_PROPERTY(int lockedCount READ lockedCount NOTIFY lockedCountChanged)
    Q_PROPERTY(int availableCount READ availableCount NOTIFY availableCountChanged)
    Q_PROPERTY(int scannedCount READ scannedCount NOTIFY scannedCountChanged)

    // === 显存 ===
    Q_PROPERTY(quint64 vramUsedMb READ vramUsedMb NOTIFY vramChanged)
    Q_PROPERTY(quint64 vramTotalMb READ vramTotalMb NOTIFY vramChanged)
    Q_PROPERTY(bool vramDegraded READ vramDegraded NOTIFY vramDegradedChanged)

    // === 时间 ===
    Q_PROPERTY(quint64 uptimeSecs READ uptimeSecs NOTIFY uptimeChanged)
    Q_PROPERTY(int nextSwitchSecs READ nextSwitchSecs NOTIFY nextSwitchSecsChanged)
    Q_PROPERTY(QString nextTimePoint READ nextTimePoint NOTIFY nextTimePointChanged)
    Q_PROPERTY(int timePointsCount READ timePointsCount NOTIFY timePointsCountChanged)

    // === 协议 ===
    Q_PROPERTY(quint32 protocolVersion READ protocolVersion NOTIFY protocolVersionChanged)

public:
    explicit DaemonState(DaemonClient *client, QObject *parent = nullptr);

    // --- Getters ---
    bool daemonConnected() const { return m_connected; }
    QString mode() const { return m_mode; }
    QString currentPath() const { return m_currentPath; }
    QString currentFilename() const { return m_currentFilename; }
    QString engine() const { return m_engine; }
    int totalWallpapers() const { return m_totalWallpapers; }
    int lockedCount() const { return m_lockedCount; }
    int availableCount() const { return m_availableCount; }
    int scannedCount() const { return m_scannedCount; }
    quint64 vramUsedMb() const { return m_vramUsedMb; }
    quint64 vramTotalMb() const { return m_vramTotalMb; }
    bool vramDegraded() const { return m_vramDegraded; }
    quint64 uptimeSecs() const { return m_uptimeSecs; }
    int nextSwitchSecs() const { return m_nextSwitchSecs; }
    QString nextTimePoint() const { return m_nextTimePoint; }
    int timePointsCount() const { return m_timePointsCount; }
    quint32 protocolVersion() const { return m_protocolVersion; }

    // === QML 可调用方法 ===

    /// 请求刷新完整状态
    Q_INVOKABLE void refresh();

signals:
    void daemonConnectedChanged();
    void modeChanged();
    void currentPathChanged();
    void currentFilenameChanged();
    void engineChanged();
    void totalWallpapersChanged();
    void lockedCountChanged();
    void availableCountChanged();
    void scannedCountChanged();
    void vramChanged();
    void vramDegradedChanged();
    void uptimeChanged();
    void nextSwitchSecsChanged();
    void nextTimePointChanged();
    void timePointsCountChanged();
    void protocolVersionChanged();

    /// 壁纸切换事件（UI 可用于动画/通知）
    void wallpaperChanged(const QString &path, const QString &filename,
                          const QString &trigger);

    /// 扫描进度
    void scanProgressUpdated(const QString &mode, int filesFound, bool completed);

    /// daemon 错误
    void daemonError(const QString &code, const QString &message, bool recoverable);

private slots:
    void onConnectionChanged(bool connected);
    void onInitialStatus(const Daemon::StatusPayload &status);
    void onEvent(const Daemon::DaemonEvent &event);

private:
    /// 从完整 Status 快照更新所有字段
    void applyStatus(const Daemon::StatusPayload &status);

    DaemonClient *m_client;

    // 缓存的状态值
    bool m_connected = false;
    QString m_mode = QStringLiteral("Video");
    QString m_currentPath;
    QString m_currentFilename;
    QString m_engine = QStringLiteral("none");
    int m_totalWallpapers = 0;
    int m_lockedCount = 0;
    int m_availableCount = 0;
    int m_scannedCount = 0;
    quint64 m_vramUsedMb = 0;
    quint64 m_vramTotalMb = 0;
    bool m_vramDegraded = false;
    quint64 m_uptimeSecs = 0;
    int m_nextSwitchSecs = -1;
    QString m_nextTimePoint;
    int m_timePointsCount = 0;
    quint32 m_protocolVersion = 0;
};
