#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>

class LianwallClient;
class ProcessManager;

/**
 * @brief 系统托盘管理器
 * 
 * 提供系统托盘图标和右键菜单：
 * - 壁纸控制: Previous, Next, Switch
 * - 生命周期: Reload, Restart, Shutdown
 * - GUI: Open Panel
 */
class TrayManager : public QObject {
    Q_OBJECT

public:
    explicit TrayManager(LianwallClient *client, ProcessManager *processManager, QObject *parent = nullptr);
    ~TrayManager();

    void show();
    void hide();

    /// 更新托盘提示文字
    void updateTooltip(const QString &mode, const QString &currentWallpaper);

    /// 设置托盘图标状态
    void setIconState(const QString &state);  // "active", "disconnected", "error"

signals:
    void openPanelRequested();
    void restartRequested();
    void shutdownRequested();

private:
    void createMenu();
    void createActions();

    QSystemTrayIcon *m_tray;
    QMenu *m_menu;
    LianwallClient *m_client;
    ProcessManager *m_processManager;

    // 菜单动作
    QAction *m_prevAction;
    QAction *m_nextAction;
    QAction *m_switchAction;
    QAction *m_reloadAction;
    QAction *m_restartAction;
    QAction *m_shutdownAction;
    QAction *m_openPanelAction;
};
