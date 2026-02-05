#pragma once

#include <QObject>
#include <QApplication>
#include <QQmlApplicationEngine>

class LianwallClient;
class ProcessManager;
class TrayManager;
class ConfigManager;
class ThumbnailCache;

/**
 * @brief 应用程序主类
 * 
 * 管理所有核心组件的生命周期：
 * - LianwallClient: Socket 通信
 * - ProcessManager: 进程管理
 * - TrayManager: 系统托盘
 * - ConfigManager: 配置管理
 * - ThumbnailCache: 缩略图缓存
 */
class Application : public QObject {
    Q_OBJECT

public:
    explicit Application(int &argc, char **argv);
    ~Application();

    int exec();

    // 组件访问器
    LianwallClient* client() const { return m_client; }
    ProcessManager* processManager() const { return m_processManager; }
    TrayManager* trayManager() const { return m_trayManager; }
    ConfigManager* configManager() const { return m_configManager; }
    ThumbnailCache* thumbnailCache() const { return m_thumbnailCache; }

public slots:
    void showMainWindow();
    void hideMainWindow();
    void quit();
    void restart();

signals:
    void aboutToQuit();

private:
    void initComponents();
    void registerQmlTypes();
    void loadTranslations();
    void setupConnections();
    void handleExitRequest();

    QApplication *m_app;
    QQmlApplicationEngine *m_engine;

    LianwallClient *m_client;
    ProcessManager *m_processManager;
    TrayManager *m_trayManager;
    ConfigManager *m_configManager;
    ThumbnailCache *m_thumbnailCache;
};
