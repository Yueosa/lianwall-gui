#pragma once

/// @file Application.h
/// @brief 应用程序主类
///
/// 管理 DaemonClient / DaemonState / ConfigManager 生命周期。
/// 系统托盘（QSystemTrayIcon）+ 托盘菜单。
/// 默认静默启动（只有托盘，不弹窗口）。
/// 关闭窗口 = 隐藏到托盘（不退出进程）。

#include <QObject>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QTranslator>
#include <QSystemTrayIcon>
#include <QMenu>

class DaemonClient;
class DaemonState;
class ConfigManager;

class Application : public QObject {
    Q_OBJECT

public:
    explicit Application(int &argc, char **argv);
    ~Application();

    int exec();

    DaemonClient* daemonClient() const { return m_daemonClient; }
    DaemonState*  daemonState()  const { return m_daemonState; }
    ConfigManager* configManager() const { return m_configManager; }

public slots:
    /// 显示主窗口（从托盘恢复）
    void showMainWindow();
    /// 隐藏主窗口到托盘
    void hideMainWindow();
    /// 切换窗口可见性
    void toggleMainWindow();
    /// 真正退出应用（非隐藏）
    void quit();
    /// 切换语言
    void switchLanguage(const QString &lang);

signals:
    void aboutToQuit();

private:
    void initComponents();
    void initSystemTray();
    void registerQmlTypes();
    void loadTranslations();

    // 托盘图标点击处理
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);

    QApplication *m_app;
    QQmlApplicationEngine *m_engine;
    QTranslator *m_translator;

    // 核心组件
    DaemonClient *m_daemonClient;
    DaemonState  *m_daemonState;
    ConfigManager *m_configManager;

    // 系统托盘
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;
};
