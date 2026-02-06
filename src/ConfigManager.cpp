#include "ConfigManager.h"
#include "Constants.h"

#include <QProcess>
#include <QSettings>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QDebug>

using namespace LianwallGui;

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
{
}

QString ConfigManager::runCliCommand(const QStringList &args) {
    QProcess process;
    QString lianwallPath = Paths::embeddedLianwall();
    process.start(lianwallPath, args);
    process.waitForFinished(5000);
    
    if (process.exitCode() != 0) {
        qWarning() << "[ConfigManager] CLI command failed:" << args;
        return QString();
    }
    
    return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
}

// ============================================================================
// 配置文件操作 (通过 CLI)
// ============================================================================

QString ConfigManager::show() {
    return runCliCommand({"config", "show"});
}

QString ConfigManager::get(const QString &key) {
    return runCliCommand({"config", "get", key});
}

bool ConfigManager::set(const QString &key, const QString &value) {
    QProcess process;
    QString lianwallPath = Paths::embeddedLianwall();
    process.start(lianwallPath, {"config", "set", key, value});
    process.waitForFinished(5000);
    
    if (process.exitCode() == 0) {
        emit configChanged();
        return true;
    }
    return false;
}

bool ConfigManager::reset() {
    QProcess process;
    QString lianwallPath = Paths::embeddedLianwall();
    process.start(lianwallPath, {"config", "reset"});
    process.waitForFinished(5000);
    
    if (process.exitCode() == 0) {
        emit configChanged();
        return true;
    }
    return false;
}

void ConfigManager::openConfigDir() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(Paths::configDir()));
}

void ConfigManager::openCacheDir() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(Paths::cacheDir()));
}

QString ConfigManager::wallpaperDir() {
    // 从配置获取壁纸目录
    QString dir = get("video_dir");
    if (dir.isEmpty()) {
        // 默认目录
        dir = QDir::homePath() + "/Videos/Wallpapers";
    }
    // 展开 ~
    if (dir.startsWith("~/")) {
        dir = QDir::homePath() + dir.mid(1);
    }
    return dir;
}

QString ConfigManager::addWallpaper(const QString &sourcePath) {
    QFileInfo sourceInfo(sourcePath);
    if (!sourceInfo.exists()) {
        qWarning() << "[ConfigManager] Source file does not exist:" << sourcePath;
        return QString();
    }
    
    QString destDir = wallpaperDir();
    QDir().mkpath(destDir);  // 确保目录存在
    
    QString destPath = destDir + "/" + sourceInfo.fileName();
    
    // 如果目标已存在，添加数字后缀
    if (QFile::exists(destPath)) {
        QString baseName = sourceInfo.completeBaseName();
        QString suffix = sourceInfo.suffix();
        int counter = 1;
        do {
            destPath = QString("%1/%2_%3.%4").arg(destDir, baseName).arg(counter++).arg(suffix);
        } while (QFile::exists(destPath));
    }
    
    if (QFile::copy(sourcePath, destPath)) {
        qDebug() << "[ConfigManager] Copied wallpaper to:" << destPath;
        return destPath;
    } else {
        qWarning() << "[ConfigManager] Failed to copy wallpaper:" << sourcePath << "->" << destPath;
        return QString();
    }
}

void ConfigManager::openInFileManager(const QString &filePath) {
    QFileInfo info(filePath);
    QString dirPath = info.isDir() ? filePath : info.absolutePath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(dirPath));
}

void ConfigManager::copyToClipboard(const QString &text) {
    QGuiApplication::clipboard()->setText(text);
}

// ============================================================================
// 应用设置 (QSettings)
// ============================================================================

QString ConfigManager::exitBehavior() {
    QSettings settings;
    return settings.value("app/exitBehavior", "ask").toString();
}

void ConfigManager::setExitBehavior(const QString &behavior) {
    QSettings settings;
    settings.setValue("app/exitBehavior", behavior);
    emit exitBehaviorChanged(behavior);
}

QString ConfigManager::language() {
    QSettings settings;
    return settings.value("app/language", "zh_CN").toString();
}

void ConfigManager::setLanguage(const QString &lang) {
    QSettings settings;
    settings.setValue("app/language", lang);
    emit languageChanged(lang);
}

QString ConfigManager::theme() {
    QSettings settings;
    return settings.value("app/theme", "lian").toString();
}

void ConfigManager::setTheme(const QString &theme) {
    QSettings settings;
    settings.setValue("app/theme", theme);
    emit themeChanged(theme);
}

// ============================================================================
// 开机自启
// ============================================================================

bool ConfigManager::isAutostartEnabled() {
    return QFile::exists(Paths::systemdServicePath());
}

bool ConfigManager::enableAutostart() {
    QString servicePath = Paths::systemdServicePath();
    QString execPath = QCoreApplication::applicationFilePath();
    
    // 构建 service 文件内容
    QString content = QString(
        "[Unit]\n"
        "Description=LianWall GUI - Wallpaper Manager\n"
        "After=graphical-session.target\n"
        "\n"
        "[Service]\n"
        "ExecStart=%1\n"
        "Restart=on-failure\n"
        "RestartSec=5\n"
        "\n"
        "[Install]\n"
        "WantedBy=default.target\n"
    ).arg(execPath);
    
    // 确保目录存在
    QDir().mkpath(QFileInfo(servicePath).absolutePath());
    
    // 写入文件
    QFile file(servicePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[ConfigManager] Failed to create service file:" << servicePath;
        return false;
    }
    file.write(content.toUtf8());
    file.close();
    
    // 启用服务
    QProcess enable;
    enable.start("systemctl", {"--user", "enable", "lianwall-gui"});
    enable.waitForFinished(5000);
    
    if (enable.exitCode() != 0) {
        qWarning() << "[ConfigManager] Failed to enable service";
        return false;
    }
    
    qDebug() << "[ConfigManager] Autostart enabled";
    return true;
}

bool ConfigManager::disableAutostart() {
    // 禁用服务
    QProcess disable;
    disable.start("systemctl", {"--user", "disable", "lianwall-gui"});
    disable.waitForFinished(5000);
    
    // 删除 service 文件
    QString servicePath = Paths::systemdServicePath();
    if (QFile::exists(servicePath)) {
        QFile::remove(servicePath);
    }
    
    qDebug() << "[ConfigManager] Autostart disabled";
    return true;
}

void ConfigManager::setAutostartEnabled(bool enabled) {
    if (enabled) {
        if (enableAutostart()) {
            emit autostartChanged(true);
        }
    } else {
        if (disableAutostart()) {
            emit autostartChanged(false);
        }
    }
}
