#include "TrayManager.h"
#include "LianwallClient.h"
#include "ProcessManager.h"

#include <QApplication>
#include <QIcon>
#include <QFile>
#include <QDebug>

TrayManager::TrayManager(LianwallClient *client, ProcessManager *processManager, QObject *parent)
    : QObject(parent)
    , m_tray(new QSystemTrayIcon(this))
    , m_menu(new QMenu())
    , m_client(client)
    , m_processManager(processManager)
{
    // 设置默认图标
    m_tray->setIcon(QIcon::fromTheme("lianwall", QIcon(":/icons/lianwall.svg")));
    m_tray->setToolTip(tr("LianWall - Wallpaper Manager"));
    
    createActions();
    createMenu();
    
    m_tray->setContextMenu(m_menu);
    
    // 双击打开面板
    connect(m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger) {
            emit openPanelRequested();
        }
    });
    
    // 监听连接状态变化
    connect(m_client, &LianwallClient::connectedChanged, this, [this]() {
        setIconState(m_client->isConnected() ? "active" : "disconnected");
    });
}

TrayManager::~TrayManager() {
    delete m_menu;
}

void TrayManager::createActions() {
    // 壁纸控制
    m_prevAction = new QAction(QIcon::fromTheme("media-skip-backward"), tr("Previous"), this);
    connect(m_prevAction, &QAction::triggered, m_client, &LianwallClient::previous);
    
    m_nextAction = new QAction(QIcon::fromTheme("media-skip-forward"), tr("Next"), this);
    connect(m_nextAction, &QAction::triggered, m_client, &LianwallClient::next);
    
    m_switchAction = new QAction(QIcon::fromTheme("view-refresh"), tr("Switch Mode"), this);
    connect(m_switchAction, &QAction::triggered, this, [this]() {
        // 切换模式 - 需要先获取当前模式
        // 简化处理：通过 CLI 调用
        QProcess::startDetached("lianwall", {"switch"});
    });
    
    // 生命周期
    m_reloadAction = new QAction(QIcon::fromTheme("view-refresh"), tr("Reload"), this);
    connect(m_reloadAction, &QAction::triggered, m_client, &LianwallClient::reload);
    
    m_restartAction = new QAction(QIcon::fromTheme("system-reboot"), tr("Restart"), this);
    connect(m_restartAction, &QAction::triggered, this, &TrayManager::restartRequested);
    
    m_shutdownAction = new QAction(QIcon::fromTheme("system-shutdown"), tr("Shutdown"), this);
    connect(m_shutdownAction, &QAction::triggered, this, &TrayManager::shutdownRequested);
    
    // GUI
    m_openPanelAction = new QAction(QIcon::fromTheme("preferences-system"), tr("Open Panel"), this);
    connect(m_openPanelAction, &QAction::triggered, this, &TrayManager::openPanelRequested);
}

void TrayManager::createMenu() {
    // 壁纸控制
    m_menu->addAction(m_prevAction);
    m_menu->addAction(m_nextAction);
    m_menu->addAction(m_switchAction);
    
    m_menu->addSeparator();
    
    // 生命周期
    m_menu->addAction(m_reloadAction);
    m_menu->addAction(m_restartAction);
    m_menu->addAction(m_shutdownAction);
    
    m_menu->addSeparator();
    
    // GUI
    m_menu->addAction(m_openPanelAction);
}

void TrayManager::show() {
    m_tray->show();
}

void TrayManager::hide() {
    m_tray->hide();
}

void TrayManager::updateTooltip(const QString &mode, const QString &currentWallpaper) {
    QString tooltip = tr("LianWall - %1 Mode\n%2")
                      .arg(mode)
                      .arg(currentWallpaper.isEmpty() ? tr("No wallpaper") : currentWallpaper);
    m_tray->setToolTip(tooltip);
}

void TrayManager::setIconState(const QString &state) {
    QString iconName = "lianwall";
    
    if (state == "disconnected") {
        iconName = "lianwall-disconnected";
    } else if (state == "error") {
        iconName = "lianwall-error";
    }
    
    // 尝试主题图标
    QIcon icon = QIcon::fromTheme(iconName);
    
    // 失败则尝试内置图标
    if (icon.isNull()) {
        QString resourcePath = QString(":/icons/%1.svg").arg(iconName);
        if (QFile::exists(resourcePath)) {
            icon = QIcon(resourcePath);
        }
    }
    
    // 如果仍然失败，使用默认图标
    if (icon.isNull() && iconName != "lianwall") {
        icon = QIcon::fromTheme("lianwall");
        if (icon.isNull()) {
            icon = QIcon(":/icons/lianwall.svg");
        }
    }
    
    if (!icon.isNull()) {
        m_tray->setIcon(icon);
    }
}
