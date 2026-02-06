#pragma once

/// @file ConfigManager.h
/// @brief 配置管理器（重写版）
///
/// 双重职责：
/// 1. Daemon 配置：通过 DaemonClient 的 GetConfig/SetConfig 读写
/// 2. 本地 GUI 设置：通过 QSettings（主题、语言等）
///
/// Daemon 配置以 Q_PROPERTY 缓存，由 ConfigChanged 事件驱动更新。
/// daemon 未连接时所有 daemon 配置写操作静默失败。

#include <QObject>
#include <QString>
#include <QJsonValue>
#include <QJsonObject>
#include <QStringList>

class DaemonClient;

class ConfigManager : public QObject
{
    Q_OBJECT

    // ========================================================================
    // Daemon 配置属性（来自 GetConfig）
    // ========================================================================

    // --- paths ---
    Q_PROPERTY(QString mode READ mode NOTIFY modeChanged)
    Q_PROPERTY(QString videoDir READ videoDir NOTIFY videoDirChanged)
    Q_PROPERTY(QString imageDir READ imageDir NOTIFY imageDirChanged)

    // --- video_engine ---
    Q_PROPERTY(int videoInterval READ videoInterval NOTIFY videoIntervalChanged)
    Q_PROPERTY(QString videoDisplay READ videoDisplay NOTIFY videoDisplayChanged)
    Q_PROPERTY(QStringList mpvpaperArgs READ mpvpaperArgs NOTIFY mpvpaperArgsChanged)
    Q_PROPERTY(QStringList mpvArgs READ mpvArgs NOTIFY mpvArgsChanged)

    // --- image_engine ---
    Q_PROPERTY(int imageInterval READ imageInterval NOTIFY imageIntervalChanged)
    Q_PROPERTY(QString imageOutputs READ imageOutputs NOTIFY imageOutputsChanged)
    Q_PROPERTY(QStringList swwwArgs READ swwwArgs NOTIFY swwwArgsChanged)

    // --- vram ---
    Q_PROPERTY(bool vramEnabled READ vramEnabled NOTIFY vramEnabledChanged)
    Q_PROPERTY(double vramThresholdPercent READ vramThresholdPercent NOTIFY vramThresholdChanged)
    Q_PROPERTY(double vramRecoveryPercent READ vramRecoveryPercent NOTIFY vramRecoveryChanged)
    Q_PROPERTY(int vramCheckInterval READ vramCheckInterval NOTIFY vramCheckIntervalChanged)
    Q_PROPERTY(int vramCooldownSeconds READ vramCooldownSeconds NOTIFY vramCooldownChanged)

    // --- daemon ---
    Q_PROPERTY(QString logLevel READ logLevel NOTIFY logLevelChanged)

    // --- 加载状态 ---
    Q_PROPERTY(bool configLoaded READ configLoaded NOTIFY configLoadedChanged)

    // ========================================================================
    // 本地 GUI 设置（QSettings）
    // ========================================================================
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)

public:
    explicit ConfigManager(DaemonClient *client, QObject *parent = nullptr);

    // --- Daemon 配置 Getters ---
    QString mode() const { return m_mode; }
    QString videoDir() const { return m_videoDir; }
    QString imageDir() const { return m_imageDir; }
    int videoInterval() const { return m_videoInterval; }
    QString videoDisplay() const { return m_videoDisplay; }
    QStringList mpvpaperArgs() const { return m_mpvpaperArgs; }
    QStringList mpvArgs() const { return m_mpvArgs; }
    int imageInterval() const { return m_imageInterval; }
    QString imageOutputs() const { return m_imageOutputs; }
    QStringList swwwArgs() const { return m_swwwArgs; }
    bool vramEnabled() const { return m_vramEnabled; }
    double vramThresholdPercent() const { return m_vramThreshold; }
    double vramRecoveryPercent() const { return m_vramRecovery; }
    int vramCheckInterval() const { return m_vramCheckInterval; }
    int vramCooldownSeconds() const { return m_vramCooldown; }
    QString logLevel() const { return m_logLevel; }
    bool configLoaded() const { return m_configLoaded; }

    // --- 本地设置 Getters ---
    QString theme() const;
    QString language() const;

    // --- 本地设置 Setters ---
    void setTheme(const QString &theme);
    void setLanguage(const QString &lang);

    // ========================================================================
    // QML 可调用的 Daemon 配置写操作
    // ========================================================================

    Q_INVOKABLE void setMode(const QString &mode);
    Q_INVOKABLE void setVideoInterval(int secs);
    Q_INVOKABLE void setVideoDisplay(const QString &display);
    Q_INVOKABLE void setMpvpaperArgs(const QStringList &args);
    Q_INVOKABLE void setMpvArgs(const QStringList &args);
    Q_INVOKABLE void setImageInterval(int secs);
    Q_INVOKABLE void setImageOutputs(const QString &outputs);
    Q_INVOKABLE void setSwwwArgs(const QStringList &args);
    Q_INVOKABLE void setVramEnabled(bool enabled);
    Q_INVOKABLE void setVramThresholdPercent(double percent);
    Q_INVOKABLE void setVramRecoveryPercent(double percent);
    Q_INVOKABLE void setVramCheckInterval(int secs);
    Q_INVOKABLE void setVramCooldownSeconds(int secs);
    Q_INVOKABLE void setLogLevel(const QString &level);

    /// 手动重新加载全部配置
    Q_INVOKABLE void reload();

    /// 打开壁纸目录（当前模式）
    Q_INVOKABLE void openCurrentWallpaperDir();

    /// 打开指定路径的目录
    Q_INVOKABLE void openDirectory(const QString &path);

signals:
    // Daemon 配置
    void modeChanged();
    void videoDirChanged();
    void imageDirChanged();
    void videoIntervalChanged();
    void videoDisplayChanged();
    void mpvpaperArgsChanged();
    void mpvArgsChanged();
    void imageIntervalChanged();
    void imageOutputsChanged();
    void swwwArgsChanged();
    void vramEnabledChanged();
    void vramThresholdChanged();
    void vramRecoveryChanged();
    void vramCheckIntervalChanged();
    void vramCooldownChanged();
    void logLevelChanged();
    void configLoadedChanged();

    // 本地设置
    void themeChanged(const QString &theme);
    void languageChanged(const QString &lang);

    /// 配置操作错误
    void errorOccurred(const QString &message);

private slots:
    void onDaemonConnected(bool connected);

private:
    /// 从完整 JSON 解析所有配置
    void applyFullConfig(const QJsonObject &config);

    /// 应用单个键值变更
    void applySingleKey(const QString &key, const QJsonValue &value);

    /// 发送 SetConfig 请求
    void sendSetConfig(const QString &key, const QJsonValue &value);

    DaemonClient *m_client;
    bool m_configLoaded = false;

    // Daemon 配置缓存
    QString m_mode = QStringLiteral("Video");
    QString m_videoDir;
    QString m_imageDir;
    int m_videoInterval = 600;
    QString m_videoDisplay = QStringLiteral("*");
    QStringList m_mpvpaperArgs;
    QStringList m_mpvArgs;
    int m_imageInterval = 600;
    QString m_imageOutputs;
    QStringList m_swwwArgs;
    bool m_vramEnabled = false;
    double m_vramThreshold = 25.0;
    double m_vramRecovery = 40.0;
    int m_vramCheckInterval = 2;
    int m_vramCooldown = 30;
    QString m_logLevel = QStringLiteral("info");
};
