#include "Application.h"
#include "ConfigManager.h"
#include "Constants.h"
#include "DaemonClient.h"
#include "DaemonState.h"
#include "DaemonTypes.h"
#include "WallpaperListModel.h"
#include "ThumbnailProvider.h"

#include <QQmlContext>
#include <QIcon>
#include <QDir>
#include <QDebug>
#include <QAction>
#include <QWindow>
#include <QTimer>
#include <QProcess>

using namespace LianwallGui;

// ============================================================================
// 构造 / 析构
// ============================================================================

Application::Application(int &argc, char **argv)
    : QObject(nullptr)
    , m_app(new QApplication(argc, argv))
    , m_engine(nullptr)
    , m_translator(nullptr)
    , m_daemonClient(nullptr)
    , m_daemonState(nullptr)
    , m_configManager(nullptr)
    , m_trayIcon(nullptr)
    , m_trayMenu(nullptr)
{
    // 设置应用信息（轻量级，单实例检测前即可完成）
    m_app->setApplicationName(APP_NAME);
    m_app->setApplicationVersion(APP_VERSION);
    m_app->setOrganizationName(APP_AUTHOR);
    m_app->setOrganizationDomain("github.com/Yueosa");
    m_app->setDesktopFileName(QStringLiteral("lianwall-gui"));
    m_app->setWindowIcon(QIcon(QStringLiteral(":/icons/lianwall.png")));

    // 关闭窗口 ≠ 退出进程（托盘驻留）
    m_app->setQuitOnLastWindowClosed(false);
}

void Application::init()
{
    m_engine = new QQmlApplicationEngine(this);

    initComponents();
    initSystemTray();
    loadTranslations();
    registerQmlTypes();
}

Application::~Application()
{
    // 必须先销毁 QML engine，再销毁它引用的 C++ 对象
    // 否则 engine 析构时 QML 绑定可能访问已释放的 contextProperty
    delete m_engine;
    m_engine = nullptr;

    delete m_trayMenu;
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

    // 配置管理（通过 DaemonClient 读写 daemon 配置）
    m_configManager = new ConfigManager(m_daemonClient, this);

    // 壁纸库 / 配置错误统一上屏（此前只打日志，用户看不到）
    connect(m_wallpaperModel, &WallpaperListModel::errorOccurred,
            this, [this](const QString &message) {
        // 旧 daemon 可能仍回 Timeout；业务上以 WallpaperChanged 为准，不当失败
        if (message.contains(QLatin1String("timeout"), Qt::CaseInsensitive)) {
            qWarning() << "[Application] Wallpaper op slow/timeout (ignored as business error):"
                       << message;
            return;
        }
        emit m_daemonState->daemonError(QStringLiteral("wallpaper"), message, true);
    });
    connect(m_configManager, &ConfigManager::errorOccurred,
            this, [this](const QString &message) {
        emit m_daemonState->daemonError(QStringLiteral("config"), message, true);
    });

    // 权威完成信号：壁纸真的变了 → 结束「切换中」
    connect(m_daemonState, &DaemonState::wallpaperChanged,
            this, [this](const QString &, const QString &, const QString &) {
        finishWallpaperCommand();
    });
    connect(m_daemonState, &DaemonState::modeChanged,
            this, [this]() {
        // SetMode 空间为空时可能只改 mode、不发 WallpaperChanged
        finishWallpaperCommand();
    });

    // 启动连接
    m_daemonClient->connectToDaemon();
}

void Application::initSystemTray()
{
    // --- 托盘菜单 ---
    m_trayMenu = new QMenu();

    auto *showAction = m_trayMenu->addAction(tr("🌀 显示/隐藏"));
    connect(showAction, &QAction::triggered, this, &Application::toggleMainWindow);

    m_trayMenu->addSeparator();

    auto *nextAction = m_trayMenu->addAction(tr("▶️ 下一张"));
    connect(nextAction, &QAction::triggered, this, [this]() {
        qDebug() << "[Tray] Next clicked, connected:" << m_daemonClient->isConnected();
        if (m_daemonClient->isConnected())
            m_daemonClient->next();
    });

    auto *prevAction = m_trayMenu->addAction(tr("◀️ 上一张"));
    connect(prevAction, &QAction::triggered, this, [this]() {
        qDebug() << "[Tray] Prev clicked, connected:" << m_daemonClient->isConnected();
        if (m_daemonClient->isConnected())
            m_daemonClient->prev();
    });

    m_trayMenu->addSeparator();

    // 模式切换（单按键切换）
    m_trayModeAction = m_trayMenu->addAction(tr("切换模式"));
    updateTrayModeAction();
    connect(m_trayModeAction, &QAction::triggered, this, [this]() {
        auto current = m_daemonState->mode();
        auto target = (current == QStringLiteral("Video")) ? QStringLiteral("Image") : QStringLiteral("Video");
        qDebug() << "[Tray] Toggle mode:" << current << "->" << target;
        daemonSetMode(target);
    });
    // DaemonState mode 变化时更新托盘文字
    connect(m_daemonState, &DaemonState::modeChanged, this, &Application::updateTrayModeAction);

    m_trayMenu->addSeparator();

    auto *restartAction = m_trayMenu->addAction(tr("🔄 重启 Daemon"));
    connect(restartAction, &QAction::triggered, this, [this]() {
        qDebug() << "[Tray] Restart daemon requested";
        if (m_daemonClient->isConnected()) {
            m_daemonClient->shutdown([this](const Daemon::Response &) {
                // shutdown 响应到达后立即重置并拉起新 daemon
                m_daemonClient->resetStartAttempt();
                m_daemonClient->resetBackoff();
                // 短延迟后启动新 daemon（等旧进程释放 socket）
                QTimer::singleShot(200, this, [this]() {
                    m_daemonClient->tryStartDaemon();
                });
            });
        }
    });

    m_trayMenu->addSeparator();

    auto *quitAction = m_trayMenu->addAction(tr("♥️ 退出"));
    connect(quitAction, &QAction::triggered, this, &Application::quit);

    // --- 托盘图标 ---
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon(QStringLiteral(":/icons/lianwall.png")));
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
    auto *ctx = m_engine->rootContext();

    // 核心组件暴露给 QML
    ctx->setContextProperty("DaemonState", m_daemonState);
    ctx->setContextProperty("ConfigManager", m_configManager);

    // 壁纸模型暴露给 QML
    ctx->setContextProperty("WallpaperModel", m_wallpaperModel);
    ctx->setContextProperty("WallpaperFilterModel", m_wallpaperFilterModel);

    // 注册缩略图提供器
    m_engine->addImageProvider(QStringLiteral("thumbnail"), new ThumbnailProvider());

    // Application 自身暴露给 QML（窗口控制）
    // 注意：不能叫 "App"，会与 QML 的 import "." as App 别名冲突
    ctx->setContextProperty("LianwallApp", this);

    // 常量
    ctx->setContextProperty("AppVersion",      APP_VERSION);
    ctx->setContextProperty("AppAuthor",       APP_AUTHOR);
    ctx->setContextProperty("AppGitHubGui",    APP_GITHUB_GUI);
    ctx->setContextProperty("AppGitHubDaemon", APP_GITHUB_DAEMON);
    ctx->setContextProperty("AppDescription",  APP_DESCRIPTION);
    ctx->setContextProperty("HomeDir", QDir::homePath());
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
    if (m_quitting) return;    // 防止重入
    m_quitting = true;
    qDebug() << "[Application] Quitting...";
    emit aboutToQuit();
    m_trayIcon->hide();

    // 发送 Shutdown 命令关闭 daemon
    // daemon 收到后会清理 mpvpaper/swww，然后关闭 socket
    if (m_daemonClient->isConnected()) {
        // 禁止自动重连（daemon 关闭后不要尝试重新连接）
        m_daemonClient->setAutoReconnect(false);

        // daemon 关闭时 socket 会断开，触发 doFinalQuit
        connect(m_daemonClient, &DaemonClient::connectionChanged,
                this, [this](bool connected) {
            if (!connected && m_quitting) {
                qDebug() << "[Application] Daemon shut down, exiting GUI";
                doFinalQuit();
            }
        });

        // 安全超时：daemon 如果 3 秒内没关掉就强制退出
        QTimer::singleShot(3000, this, [this]() {
            if (m_quitting) {
                qWarning() << "[Application] Daemon shutdown timeout, force quit";
                doFinalQuit();
            }
        });

        m_daemonClient->shutdown();
    } else {
        // daemon 本身没连上，直接退出
        doFinalQuit();
    }
}

void Application::doFinalQuit()
{
    m_daemonClient->disconnectFromDaemon();
    m_app->quit();
}

// ============================================================================
// QML 可调用的 Daemon 命令
// ============================================================================

void Application::finishWallpaperCommand()
{
    if (!m_wallpaperCommandInFlight)
        return;
    m_wallpaperCommandInFlight = false;
    emit wallpaperSwitchingChanged();
}

void Application::handleWallpaperCommandResponse(const Daemon::Response &r, const char *op)
{
    if (r.type != Daemon::ResponseType::Error) {
        // Ok：引擎侧已完成。若事件已先到，finish 为 no-op；否则此处收尾
        // （兼容 SetMode 同模式 / 空空间等可能不发 WallpaperChanged 的情况）
        finishWallpaperCommand();
        return;
    }

    auto err = r.asError();
    qWarning() << "[Application]" << op << "failed:" << err.message
               << "code:" << Daemon::errorCodeToString(err.code);

    // Timeout 不是业务失败：保持「切换中」，等 WallpaperChanged / modeChanged
    if (err.code == Daemon::ErrorCode::Timeout) {
        qWarning() << "[Application]" << op
                   << "timed out at IPC layer — waiting for wallpaper/mode event";
        return;
    }

    finishWallpaperCommand();
    emit m_daemonState->daemonError(
        Daemon::errorCodeToString(err.code), err.message, true);
}

void Application::armWallpaperCommandGuard()
{
    if (!m_wallpaperCommandInFlight) {
        m_wallpaperCommandInFlight = true;
        emit wallpaperSwitchingChanged();
    }
    // 极端兜底：事件与回执皆丢失时恢复可点
    QTimer::singleShot(60000, this, [this]() {
        if (m_wallpaperCommandInFlight) {
            qWarning() << "[Application] Wallpaper switching guard fired — clearing in-flight flag";
            finishWallpaperCommand();
        }
    });
}

void Application::daemonNext()
{
    qDebug() << "[Application] daemonNext() called, connected:" << m_daemonClient->isConnected();
    if (!m_daemonClient->isConnected() || m_wallpaperCommandInFlight)
        return;

    armWallpaperCommandGuard();
    m_daemonClient->next([this](const Daemon::Response &r) {
        handleWallpaperCommandResponse(r, "Next");
    });
}

void Application::daemonPrev()
{
    qDebug() << "[Application] daemonPrev() called, connected:" << m_daemonClient->isConnected();
    if (!m_daemonClient->isConnected() || m_wallpaperCommandInFlight)
        return;

    armWallpaperCommandGuard();
    m_daemonClient->prev([this](const Daemon::Response &r) {
        handleWallpaperCommandResponse(r, "Prev");
    });
}

void Application::daemonToggleLock()
{
    if (!m_daemonClient->isConnected() || m_lockCommandInFlight)
        return;

    auto path = m_daemonState->currentPath();
    qDebug() << "[Application] daemonToggleLock() path:" << path;
    if (path.isEmpty())
        return;

    m_lockCommandInFlight = true;
    m_daemonClient->toggleLock(path, [this](const Daemon::Response &r) {
        m_lockCommandInFlight = false;

        if (r.type == Daemon::ResponseType::Error) {
            auto err = r.asError();
            qWarning() << "[Application] ToggleLock failed:" << err.message;
            emit m_daemonState->daemonError(
                Daemon::errorCodeToString(err.code), err.message, true);
        }
    });
}

void Application::daemonRescan()
{
    qDebug() << "[Application] daemonRescan() called";
    if (m_daemonClient->isConnected())
        m_daemonClient->rescan();
}

void Application::daemonReloadConfig()
{
    qDebug() << "[Application] daemonReloadConfig() called";
    if (m_daemonClient->isConnected())
        m_daemonClient->reloadConfig();
}

void Application::daemonPauseRotation()
{
    qDebug() << "[Application] daemonPauseRotation() called";
    if (!m_daemonClient->isConnected())
        return;

    m_daemonClient->pauseRotation([this](const Daemon::Response &r) {
        if (r.type == Daemon::ResponseType::Error) {
            auto err = r.asError();
            qWarning() << "[Application] PauseRotation failed:" << err.message;
            emit m_daemonState->daemonError(
                Daemon::errorCodeToString(err.code), err.message, true);
        }
    });
}

void Application::daemonResumeRotation()
{
    qDebug() << "[Application] daemonResumeRotation() called";
    if (!m_daemonClient->isConnected())
        return;

    m_daemonClient->resumeRotation([this](const Daemon::Response &r) {
        if (r.type == Daemon::ResponseType::Error) {
            auto err = r.asError();
            qWarning() << "[Application] ResumeRotation failed:" << err.message;
            emit m_daemonState->daemonError(
                Daemon::errorCodeToString(err.code), err.message, true);
        }
    });
}

void Application::daemonSetMode(const QString &mode)
{
    qDebug() << "[Application] daemonSetMode() called, mode:" << mode
             << "connected:" << m_daemonClient->isConnected();
    if (!m_daemonClient->isConnected() || m_wallpaperCommandInFlight)
        return;

    auto m = Daemon::wallModeFromString(mode);
    armWallpaperCommandGuard();
    m_daemonClient->setMode(m, [this](const Daemon::Response &r) {
        handleWallpaperCommandResponse(r, "SetMode");
    });
}

void Application::runSystemdCommand(const QString &action)
{
    // systemctl --user <action> lianwalld.service
    QStringList args = {"--user", action, "lianwalld.service"};
    qDebug() << "[Application] systemctl" << args;

    auto *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [proc](int exitCode, QProcess::ExitStatus) {
        if (exitCode != 0) {
            auto err = proc->readAllStandardError().trimmed();
            qWarning() << "[Application] systemctl failed:" << err;
        }
        proc->deleteLater();
    });
    proc->start("systemctl", args);
}

// ============================================================================
// 托盘
// ============================================================================

void Application::updateTrayModeAction()
{
    if (!m_trayModeAction)
        return;
    auto mode = m_daemonState->mode();
    if (mode == QStringLiteral("Video"))
        m_trayModeAction->setText(tr("🖼️ 切到图片"));
    else
        m_trayModeAction->setText(tr("🎬 切到视频"));
}

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

