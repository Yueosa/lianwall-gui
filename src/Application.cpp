#include "Application.h"
#include "ConfigManager.h"
#include "Constants.h"
#include "DaemonClient.h"
#include "DaemonState.h"
#include "WallpaperListModel.h"
#include "ThumbnailProvider.h"

#include <QQmlContext>
#include <QIcon>
#include <QDebug>
#include <QAction>
#include <QWindow>

using namespace LianwallGui;

// ============================================================================
// 构造 / 析构
// ============================================================================

Application::Application(int &argc, char **argv)
    : QObject(nullptr)
    , m_app(new QApplication(argc, argv))
    , m_engine(new QQmlApplicationEngine(this))
    , m_translator(nullptr)
    , m_daemonClient(nullptr)
    , m_daemonState(nullptr)
    , m_configManager(nullptr)
    , m_trayIcon(nullptr)
    , m_trayMenu(nullptr)
{
    // 设置应用信息
    m_app->setApplicationName(APP_NAME);
    m_app->setApplicationVersion(APP_VERSION);
    m_app->setOrganizationName(APP_AUTHOR);
    m_app->setOrganizationDomain("github.com/Yueosa");
    m_app->setWindowIcon(QIcon(QStringLiteral(":/icons/lianwall.svg")));

    // 关闭窗口 ≠ 退出进程（托盘驻留）
    m_app->setQuitOnLastWindowClosed(false);

    initComponents();
    initSystemTray();
    loadTranslations();
    registerQmlTypes();
}

Application::~Application()
{
    delete m_trayMenu;   // trayIcon 通过 parent 自动删除
    delete m_app;
}

// ============================================================================
// 组件初始化
// ============================================================================

void Application::initComponents()
{
    // 通信层
    m_daemonClient = new DaemonClient(this);
    m_daemonState  = new DaemonState(m_daemonClient, this);

    // 壁纸模型
    m_wallpaperModel = new WallpaperListModel(m_daemonClient, this);
    m_wallpaperFilterModel = new WallpaperFilterModel(this);
    m_wallpaperFilterModel->setSourceModel(m_wallpaperModel);

    // 配置管理（暂保留旧 ConfigManager，Phase 6 重写）
    m_configManager = new ConfigManager(this);

    // 启动连接
    m_daemonClient->connectToDaemon();
}

void Application::initSystemTray()
{
    // --- 托盘菜单 ---
    m_trayMenu = new QMenu();

    auto *showAction = m_trayMenu->addAction(tr("显示/隐藏"));
    connect(showAction, &QAction::triggered, this, &Application::toggleMainWindow);

    m_trayMenu->addSeparator();

    auto *nextAction = m_trayMenu->addAction(tr("下一张"));
    connect(nextAction, &QAction::triggered, this, [this]() {
        m_daemonClient->next();
    });

    auto *prevAction = m_trayMenu->addAction(tr("上一张"));
    connect(prevAction, &QAction::triggered, this, [this]() {
        m_daemonClient->prev();
    });

    m_trayMenu->addSeparator();

    auto *reloadAction = m_trayMenu->addAction(tr("重载配置"));
    connect(reloadAction, &QAction::triggered, this, [this]() {
        m_daemonClient->reloadConfig();
    });

    m_trayMenu->addSeparator();

    auto *quitAction = m_trayMenu->addAction(tr("退出"));
    connect(quitAction, &QAction::triggered, this, &Application::quit);

    // --- 托盘图标 ---
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon(QStringLiteral(":/icons/lianwall.svg")));
    m_trayIcon->setToolTip(QStringLiteral("%1 %2").arg(APP_NAME, APP_VERSION));
    m_trayIcon->setContextMenu(m_trayMenu);

    connect(m_trayIcon, &QSystemTrayIcon::activated,
            this, &Application::onTrayActivated);

    m_trayIcon->show();
}

void Application::loadTranslations()
{
    QString lang = m_configManager->language();
    switchLanguage(lang);
}

void Application::switchLanguage(const QString &lang)
{
    if (m_translator) {
        m_app->removeTranslator(m_translator);
        delete m_translator;
        m_translator = nullptr;
    }

    m_translator = new QTranslator(this);
    if (m_translator->load(QStringLiteral(":/translations/lianwall-gui_%1.qm").arg(lang))) {
        m_app->installTranslator(m_translator);
        qDebug() << "[Application] Loaded translation:" << lang;
    } else {
        qWarning() << "[Application] Failed to load translation:" << lang;
        delete m_translator;
        m_translator = nullptr;
    }

    if (m_engine)
        m_engine->retranslate();
}

void Application::registerQmlTypes()
{
    // 注册旧的 ConfigManager（Phase 6 会替换）
    qmlRegisterSingletonInstance("LianwallGui", 1, 0, "ConfigManager", m_configManager);

    auto *ctx = m_engine->rootContext();

    // DaemonState 暴露给 QML
    ctx->setContextProperty("DaemonState", m_daemonState);

    // 壁纸模型暴露给 QML
    ctx->setContextProperty("WallpaperModel", m_wallpaperModel);
    ctx->setContextProperty("WallpaperFilterModel", m_wallpaperFilterModel);

    // 注册缩略图提供器
    m_engine->addImageProvider(QStringLiteral("thumbnail"), new ThumbnailProvider());

    // Application 自身暴露给 QML（窗口控制）
    ctx->setContextProperty("App", this);

    // 常量
    ctx->setContextProperty("AppVersion",      APP_VERSION);
    ctx->setContextProperty("AppAuthor",       APP_AUTHOR);
    ctx->setContextProperty("AppGitHubGui",    APP_GITHUB_GUI);
    ctx->setContextProperty("AppGitHubDaemon", APP_GITHUB_DAEMON);
    ctx->setContextProperty("AppDescription",  APP_DESCRIPTION);
}

// ============================================================================
// 执行
// ============================================================================

int Application::exec()
{
    qDebug() << "[Application] Starting (silent mode — tray only)";

    const QUrl url(QStringLiteral("qrc:/qt/qml/LianwallGui/qml/main.qml"));

    QObject::connect(m_engine, &QQmlApplicationEngine::objectCreated, m_app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) {
                qCritical() << "[Application] Failed to load QML";
                QCoreApplication::exit(-1);
            }
        }, Qt::QueuedConnection);

    m_engine->load(url);

    // 静默启动：加载 QML 后立即隐藏窗口
    // 用户通过托盘图标打开窗口
    hideMainWindow();

    qDebug() << "[Application] Ready";
    return m_app->exec();
}

// ============================================================================
// 窗口控制
// ============================================================================

void Application::showMainWindow()
{
    if (m_engine->rootObjects().isEmpty())
        return;

    auto *root = m_engine->rootObjects().first();
    if (auto *window = qobject_cast<QWindow *>(root)) {
        window->show();
        window->raise();
        window->requestActivate();
    } else {
        QMetaObject::invokeMethod(root, "show");
        QMetaObject::invokeMethod(root, "raise");
        QMetaObject::invokeMethod(root, "requestActivate");
    }
}

void Application::hideMainWindow()
{
    if (m_engine->rootObjects().isEmpty())
        return;

    auto *root = m_engine->rootObjects().first();
    if (auto *window = qobject_cast<QWindow *>(root)) {
        window->hide();
    } else {
        QMetaObject::invokeMethod(root, "hide");
    }
}

void Application::toggleMainWindow()
{
    if (m_engine->rootObjects().isEmpty())
        return;

    auto *root = m_engine->rootObjects().first();
    if (auto *window = qobject_cast<QWindow *>(root)) {
        window->isVisible() ? hideMainWindow() : showMainWindow();
    } else {
        // QML Window 的 visible 属性
        bool visible = root->property("visible").toBool();
        visible ? hideMainWindow() : showMainWindow();
    }
}

void Application::quit()
{
    qDebug() << "[Application] Quitting...";
    emit aboutToQuit();
    m_daemonClient->disconnectFromDaemon();
    m_trayIcon->hide();
    m_app->quit();
}

// ============================================================================
// QML 可调用的 Daemon 命令
// ============================================================================

void Application::daemonNext()
{
    if (m_daemonClient->isConnected())
        m_daemonClient->next();
}

void Application::daemonPrev()
{
    if (m_daemonClient->isConnected())
        m_daemonClient->prev();
}

void Application::daemonToggleLock()
{
    if (m_daemonClient->isConnected()) {
        auto path = m_daemonState->currentPath();
        if (!path.isEmpty())
            m_daemonClient->toggleLock(path);
    }
}

void Application::daemonRescan()
{
    if (m_daemonClient->isConnected())
        m_daemonClient->rescan();
}

void Application::daemonReloadConfig()
{
    if (m_daemonClient->isConnected())
        m_daemonClient->reloadConfig();
}

// ============================================================================
// 托盘
// ============================================================================

void Application::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:       // 左键单击
    case QSystemTrayIcon::DoubleClick:   // 双击
        toggleMainWindow();
        break;
    default:
        break;
    }
}

