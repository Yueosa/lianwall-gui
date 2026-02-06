#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

/**
 * @brief 配置管理器
 * 
 * 通过调用 lianwall CLI 管理配置文件。
 * 不直接解析 TOML，而是使用 CLI 命令。
 */
class ConfigManager : public QObject {
    Q_OBJECT

    // GUI 应用设置属性
    Q_PROPERTY(QString exitBehavior READ exitBehavior WRITE setExitBehavior NOTIFY exitBehaviorChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(bool autostartEnabled READ isAutostartEnabled WRITE setAutostartEnabled NOTIFY autostartChanged)

public:
    explicit ConfigManager(QObject *parent = nullptr);

    /// 显示完整配置
    Q_INVOKABLE QString show();

    /// 获取配置项
    Q_INVOKABLE QString get(const QString &key);

    /// 设置配置项
    Q_INVOKABLE bool set(const QString &key, const QString &value);

    /// 重置配置
    Q_INVOKABLE bool reset();

    /// 在文件管理器中打开配置目录
    Q_INVOKABLE void openConfigDir();

    /// 在文件管理器中打开缓存目录
    Q_INVOKABLE void openCacheDir();

    /// 获取壁纸目录路径
    Q_INVOKABLE QString wallpaperDir();

    /// 添加壁纸（复制文件到壁纸目录）
    /// @param sourcePath 源文件路径
    /// @return 成功返回目标路径，失败返回空字符串
    Q_INVOKABLE QString addWallpaper(const QString &sourcePath);

    /// 在文件管理器中打开指定文件所在目录
    Q_INVOKABLE void openInFileManager(const QString &filePath);

    /// 复制文本到剪贴板
    Q_INVOKABLE void copyToClipboard(const QString &text);

    // === 应用设置 (存储在 QSettings) ===

    /// 退出行为: "ask", "minimize", "shutdown"
    Q_INVOKABLE QString exitBehavior();
    Q_INVOKABLE void setExitBehavior(const QString &behavior);

    /// 语言: "en", "zh_CN"
    Q_INVOKABLE QString language();
    Q_INVOKABLE void setLanguage(const QString &lang);

    /// 主题: "light", "dark", "system"
    Q_INVOKABLE QString theme();
    Q_INVOKABLE void setTheme(const QString &theme);

    // === 开机自启 ===

    /// 检查是否已启用开机自启
    Q_INVOKABLE bool isAutostartEnabled();

    /// 启用开机自启（会创建 systemd service 文件）
    Q_INVOKABLE bool enableAutostart();

    /// 禁用开机自启
    Q_INVOKABLE bool disableAutostart();
    
    /// 设置开机自启（用于 Q_PROPERTY）
    void setAutostartEnabled(bool enabled);

signals:
    void configChanged();
    void languageChanged(const QString &lang);
    void themeChanged(const QString &theme);
    void exitBehaviorChanged(const QString &behavior);
    void autostartChanged(bool enabled);

private:
    QString runCliCommand(const QStringList &args);
};
