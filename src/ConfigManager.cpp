#include "ConfigManager.h"
#include "DaemonClient.h"
#include "DaemonTypes.h"

#include <QSettings>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QJsonArray>
#include <QDebug>

// ============================================================================
// 构造
// ============================================================================

ConfigManager::ConfigManager(DaemonClient *client, QObject *parent)
    : QObject(parent)
    , m_client(client)
{
    // daemon 连接后自动拉取全部配置
    connect(m_client, &DaemonClient::connectionChanged,
            this, &ConfigManager::onDaemonConnected);

    // 监听 ConfigChanged 事件
    connect(m_client, &DaemonClient::eventReceived,
            this, [this](const Daemon::DaemonEvent &event) {
        if (event.type == Daemon::EventType::ConfigChanged) {
            auto d = event.asConfigChanged();
            if (d.key == QLatin1String("all")) {
                // ReloadConfig → 重新拉取全部
                reload();
            } else {
                applySingleKey(d.key, d.newValue);
            }
        }
    });
}

// ============================================================================
// 本地 GUI 设置（QSettings）
// ============================================================================

QString ConfigManager::theme() const
{
    QSettings settings;
    return settings.value("app/theme", "lian").toString();
}

void ConfigManager::setTheme(const QString &theme)
{
    QSettings settings;
    if (settings.value("app/theme").toString() == theme)
        return;
    settings.setValue("app/theme", theme);
    emit themeChanged(theme);
}

QString ConfigManager::language() const
{
    QSettings settings;
    return settings.value("app/language", "zh_CN").toString();
}

void ConfigManager::setLanguage(const QString &lang)
{
    QSettings settings;
    if (settings.value("app/language").toString() == lang)
        return;
    settings.setValue("app/language", lang);
    emit languageChanged(lang);
}

// ============================================================================
// Daemon 连接回调
// ============================================================================

void ConfigManager::onDaemonConnected(bool connected)
{
    if (connected) {
        reload();
    }
}

void ConfigManager::reload()
{
    if (!m_client->isConnected())
        return;

    m_client->getConfig(QString(), [this](const Daemon::Response &resp) {
        if (resp.type == Daemon::ResponseType::Config) {
            auto cfg = resp.asConfig();
            if (cfg.value.isObject()) {
                applyFullConfig(cfg.value.toObject());
                if (!m_configLoaded) {
                    m_configLoaded = true;
                    emit configLoadedChanged();
                }
                qDebug() << "[ConfigManager] Full config loaded";
            }
        } else if (resp.type == Daemon::ResponseType::Error) {
            auto err = resp.asError();
            qWarning() << "[ConfigManager] GetConfig error:" << err.message;
            emit errorOccurred(err.message);
        }
    });
}

// ============================================================================
// 配置解析
// ============================================================================

static QStringList jsonArrayToStringList(const QJsonValue &val) {
    QStringList result;
    if (val.isArray()) {
        for (const auto &v : val.toArray())
            result.append(v.toString());
    }
    return result;
}

void ConfigManager::applyFullConfig(const QJsonObject &config)
{
    // --- paths ---
    auto paths = config["paths"].toObject();
    {
        auto v = paths["mode"].toString();
        if (!v.isEmpty() && m_mode != v) { m_mode = v; emit modeChanged(); }
    }
    {
        auto v = paths["video_dir"].toString();
        if (m_videoDir != v) { m_videoDir = v; emit videoDirChanged(); }
    }
    {
        auto v = paths["image_dir"].toString();
        if (m_imageDir != v) { m_imageDir = v; emit imageDirChanged(); }
    }

    // --- video_engine ---
    auto video = config["video_engine"].toObject();
    {
        auto v = video["interval"].toInt(600);
        if (m_videoInterval != v) { m_videoInterval = v; emit videoIntervalChanged(); }
    }
    {
        auto v = video["display"].toString("*");
        if (m_videoDisplay != v) { m_videoDisplay = v; emit videoDisplayChanged(); }
    }
    {
        auto v = jsonArrayToStringList(video["mpvpaper_args"]);
        if (m_mpvpaperArgs != v) { m_mpvpaperArgs = v; emit mpvpaperArgsChanged(); }
    }
    {
        auto v = jsonArrayToStringList(video["mpv_args"]);
        if (m_mpvArgs != v) { m_mpvArgs = v; emit mpvArgsChanged(); }
    }

    // --- image_engine ---
    auto image = config["image_engine"].toObject();
    {
        auto v = image["interval"].toInt(600);
        if (m_imageInterval != v) { m_imageInterval = v; emit imageIntervalChanged(); }
    }
    {
        auto v = image["outputs"].toString("");
        if (m_imageOutputs != v) { m_imageOutputs = v; emit imageOutputsChanged(); }
    }
    {
        auto v = jsonArrayToStringList(image["swww_args"]);
        if (m_swwwArgs != v) { m_swwwArgs = v; emit swwwArgsChanged(); }
    }

    // --- vram ---
    auto vram = config["vram"].toObject();
    {
        auto v = vram["enabled"].toBool(false);
        if (m_vramEnabled != v) { m_vramEnabled = v; emit vramEnabledChanged(); }
    }
    {
        auto v = vram["threshold_percent"].toDouble(25.0);
        if (qAbs(m_vramThreshold - v) > 0.01) { m_vramThreshold = v; emit vramThresholdChanged(); }
    }
    {
        auto v = vram["recovery_percent"].toDouble(40.0);
        if (qAbs(m_vramRecovery - v) > 0.01) { m_vramRecovery = v; emit vramRecoveryChanged(); }
    }
    {
        auto v = vram["check_interval"].toInt(2);
        if (m_vramCheckInterval != v) { m_vramCheckInterval = v; emit vramCheckIntervalChanged(); }
    }
    {
        auto v = vram["cooldown_seconds"].toInt(30);
        if (m_vramCooldown != v) { m_vramCooldown = v; emit vramCooldownChanged(); }
    }

    // --- daemon ---
    auto daemon = config["daemon"].toObject();
    {
        auto v = daemon["log_level"].toString("info");
        if (m_logLevel != v) { m_logLevel = v; emit logLevelChanged(); }
    }
}

void ConfigManager::applySingleKey(const QString &key, const QJsonValue &value)
{
    if      (key == QLatin1String("paths.mode"))              { auto v = value.toString(); if (m_mode != v) { m_mode = v; emit modeChanged(); } }
    else if (key == QLatin1String("paths.video_dir"))         { auto v = value.toString(); if (m_videoDir != v) { m_videoDir = v; emit videoDirChanged(); } }
    else if (key == QLatin1String("paths.image_dir"))         { auto v = value.toString(); if (m_imageDir != v) { m_imageDir = v; emit imageDirChanged(); } }
    else if (key == QLatin1String("video_engine.interval"))   { auto v = value.toInt(); if (m_videoInterval != v) { m_videoInterval = v; emit videoIntervalChanged(); } }
    else if (key == QLatin1String("video_engine.display"))    { auto v = value.toString(); if (m_videoDisplay != v) { m_videoDisplay = v; emit videoDisplayChanged(); } }
    else if (key == QLatin1String("video_engine.mpvpaper_args")) { auto v = jsonArrayToStringList(value); if (m_mpvpaperArgs != v) { m_mpvpaperArgs = v; emit mpvpaperArgsChanged(); } }
    else if (key == QLatin1String("video_engine.mpv_args"))   { auto v = jsonArrayToStringList(value); if (m_mpvArgs != v) { m_mpvArgs = v; emit mpvArgsChanged(); } }
    else if (key == QLatin1String("image_engine.interval"))   { auto v = value.toInt(); if (m_imageInterval != v) { m_imageInterval = v; emit imageIntervalChanged(); } }
    else if (key == QLatin1String("image_engine.outputs"))    { auto v = value.toString(); if (m_imageOutputs != v) { m_imageOutputs = v; emit imageOutputsChanged(); } }
    else if (key == QLatin1String("image_engine.swww_args"))  { auto v = jsonArrayToStringList(value); if (m_swwwArgs != v) { m_swwwArgs = v; emit swwwArgsChanged(); } }
    else if (key == QLatin1String("vram.enabled"))            { auto v = value.toBool(); if (m_vramEnabled != v) { m_vramEnabled = v; emit vramEnabledChanged(); } }
    else if (key == QLatin1String("vram.threshold_percent"))  { auto v = value.toDouble(); if (qAbs(m_vramThreshold - v) > 0.01) { m_vramThreshold = v; emit vramThresholdChanged(); } }
    else if (key == QLatin1String("vram.recovery_percent"))   { auto v = value.toDouble(); if (qAbs(m_vramRecovery - v) > 0.01) { m_vramRecovery = v; emit vramRecoveryChanged(); } }
    else if (key == QLatin1String("vram.check_interval"))     { auto v = value.toInt(); if (m_vramCheckInterval != v) { m_vramCheckInterval = v; emit vramCheckIntervalChanged(); } }
    else if (key == QLatin1String("vram.cooldown_seconds"))   { auto v = value.toInt(); if (m_vramCooldown != v) { m_vramCooldown = v; emit vramCooldownChanged(); } }
    else if (key == QLatin1String("daemon.log_level"))        { auto v = value.toString(); if (m_logLevel != v) { m_logLevel = v; emit logLevelChanged(); } }
    else {
        qDebug() << "[ConfigManager] Unknown config key:" << key;
    }
}

// ============================================================================
// SetConfig 发送
// ============================================================================

void ConfigManager::sendSetConfig(const QString &key, const QJsonValue &value)
{
    if (!m_client->isConnected()) {
        emit errorOccurred(tr("守护进程未连接"));
        return;
    }

    m_client->setConfig(key, value, [this, key](const Daemon::Response &resp) {
        if (resp.type == Daemon::ResponseType::Error) {
            auto err = resp.asError();
            qWarning() << "[ConfigManager] SetConfig failed:" << key << err.message;
            emit errorOccurred(err.message);
        }
    });
}

// ============================================================================
// QML 可调用的写操作（每个都先乐观更新本地缓存，然后发送请求）
// ============================================================================

void ConfigManager::setMode(const QString &mode)
{
    sendSetConfig("paths.mode", mode);
}

void ConfigManager::setVideoInterval(int secs)
{
    secs = qBound(10, secs, 86400);
    sendSetConfig("video_engine.interval", secs);
}

void ConfigManager::setVideoDisplay(const QString &display)
{
    sendSetConfig("video_engine.display", display);
}

void ConfigManager::setMpvpaperArgs(const QStringList &args)
{
    QJsonArray arr;
    for (const auto &a : args) arr.append(a);
    sendSetConfig("video_engine.mpvpaper_args", arr);
}

void ConfigManager::setMpvArgs(const QStringList &args)
{
    QJsonArray arr;
    for (const auto &a : args) arr.append(a);
    sendSetConfig("video_engine.mpv_args", arr);
}

void ConfigManager::setImageInterval(int secs)
{
    secs = qBound(10, secs, 86400);
    sendSetConfig("image_engine.interval", secs);
}

void ConfigManager::setImageOutputs(const QString &outputs)
{
    sendSetConfig("image_engine.outputs", outputs);
}

void ConfigManager::setSwwwArgs(const QStringList &args)
{
    QJsonArray arr;
    for (const auto &a : args) arr.append(a);
    sendSetConfig("image_engine.swww_args", arr);
}

void ConfigManager::setVramEnabled(bool enabled)
{
    sendSetConfig("vram.enabled", enabled);
}

void ConfigManager::setVramThresholdPercent(double percent)
{
    percent = qBound(5.0, percent, 50.0);
    sendSetConfig("vram.threshold_percent", percent);
}

void ConfigManager::setVramRecoveryPercent(double percent)
{
    percent = qBound(20.0, percent, 80.0);
    sendSetConfig("vram.recovery_percent", percent);
}

void ConfigManager::setVramCheckInterval(int secs)
{
    secs = qBound(1, secs, 60);
    sendSetConfig("vram.check_interval", secs);
}

void ConfigManager::setVramCooldownSeconds(int secs)
{
    secs = qBound(10, secs, 600);
    sendSetConfig("vram.cooldown_seconds", secs);
}

void ConfigManager::setLogLevel(const QString &level)
{
    sendSetConfig("daemon.log_level", level);
}

// ============================================================================
// 目录操作
// ============================================================================

void ConfigManager::openCurrentWallpaperDir()
{
    QString dir = (m_mode == QLatin1String("Video")) ? m_videoDir : m_imageDir;
    if (!dir.isEmpty()) {
        // 展开 ~
        if (dir.startsWith("~/"))
            dir = QDir::homePath() + dir.mid(1);
        QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
    }
}

void ConfigManager::openDirectory(const QString &path)
{
    QString dir = path;
    if (dir.startsWith("~/"))
        dir = QDir::homePath() + dir.mid(1);
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
}
