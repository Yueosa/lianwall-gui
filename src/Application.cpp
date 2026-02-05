#include "Application.h"
#include "LianwallClient.h"
#include "ProcessManager.h"
#include "TrayManager.h"
#include "ConfigManager.h"
#include "ThumbnailCache.h"
#include "Constants.h"
#include "models/StatusModel.h"
#include "models/WallpaperModel.h"
#include "models/TimelineModel.h"

#include <QQmlContext>
#include <QTranslator>
#include <QLocale>
#include <QIcon>
#include <QDebug>

using namespace LianwallGui;

Application::Application(int &argc, char **argv)
    : QObject(nullptr)
    , m_app(new QApplication(argc, argv))
    , m_engine(new QQmlApplicationEngine(this))
    , m_client(nullptr)
    , m_processManager(nullptr)
    , m_trayManager(nullptr)
    , m_configManager(nullptr)
    , m_thumbnailCache(nullptr)
{
    // 设置应用信息
    m_app->setApplicationName(APP_NAME);
    m_app->setApplicationVersion(APP_VERSION);
    m_app->setOrganizationName(APP_AUTHOR);
    m_app->setOrganizationDomain("github.com/Yueosa");
    m_app->setWindowIcon(QIcon(":/icons/lianwall.svg"));
    
    // 不在最后一个窗口关闭时退出（托盘常驻）
    m_app->setQuitOnLastWindowClosed(false);
    
    initComponents();
    loadTranslations();
    registerQmlTypes();
    setupConnections();
}

Application::~Application() {
    delete m_app;
}

void Application::initComponents() {
    // 创建核心组件
    m_client = new LianwallClient(this);
    m_processManager = new ProcessManager(m_client, this);
    m_configManager = new ConfigManager(this);
    m_thumbnailCache = new ThumbnailCache(this);
    
    // 创建系统托盘管理器
    m_trayManager = new TrayManager(m_client, m_processManager, this);
}

void Application::loadTranslations() {
    QString lang = m_configManager->language();
    
    QTranslator *translator = new QTranslator(this);
    if (translator->load(QString(":/translations/lianwall-gui_%1.qm").arg(lang))) {
        m_app->installTranslator(translator);
        qDebug() << "[Application] Loaded translation:" << lang;
    } else {
        qWarning() << "[Application] Failed to load translation:" << lang;
    }
}

void Application::registerQmlTypes() {
    // 注册 C++ 类型到 QML
    qmlRegisterSingletonInstance("LianwallGui", 1, 0, "Client", m_client);
    qmlRegisterSingletonInstance("LianwallGui", 1, 0, "ProcessManager", m_processManager);
    qmlRegisterSingletonInstance("LianwallGui", 1, 0, "ConfigManager", m_configManager);
    qmlRegisterSingletonInstance("LianwallGui", 1, 0, "ThumbnailCache", m_thumbnailCache);
    
    // 注册 Model 类型
    qmlRegisterType<StatusModel>("LianwallGui", 1, 0, "StatusModel");
    qmlRegisterType<WallpaperModel>("LianwallGui", 1, 0, "WallpaperModel");
    qmlRegisterType<TimelineModel>("LianwallGui", 1, 0, "TimelineModel");
    
    // 暴露常量
    m_engine->rootContext()->setContextProperty("AppVersion", APP_VERSION);
    m_engine->rootContext()->setContextProperty("AppAuthor", APP_AUTHOR);
    m_engine->rootContext()->setContextProperty("AppGitHub", APP_GITHUB);
    m_engine->rootContext()->setContextProperty("AppDescription", APP_DESCRIPTION);
}

void Application::setupConnections() {
    // 监听配置变化
    connect(m_configManager, &ConfigManager::configChanged, m_client, &LianwallClient::reload);
    
    // 监听语言变化
    connect(m_configManager, &ConfigManager::languageChanged, this, [this](const QString &lang) {
        // 需要重启应用以应用新语言
        qDebug() << "[Application] Language changed to:" << lang;
    });
    
    // 状态更新时更新托盘提示
    connect(m_client, &LianwallClient::statusReceived, this, [this](const QJsonObject &status) {
        if (m_trayManager) {
            QString mode = status["mode"].toString();
            QString current = status["current"].toString();
            m_trayManager->updateTooltip(mode, current);
        }
    });
    
    // 托盘信号连接
    if (m_trayManager) {
        connect(m_trayManager, &TrayManager::openPanelRequested, this, &Application::showMainWindow);
        connect(m_trayManager, &TrayManager::shutdownRequested, this, &Application::quit);
        connect(m_trayManager, &TrayManager::restartRequested, this, &Application::restart);
    }
}

int Application::exec() {
    // 1. 静默启动，确保 daemon 运行
    qDebug() << "[Application] Starting...";
    
    if (!m_processManager->ensureDaemonRunning()) {
        qCritical() << "[Application] Failed to start daemon";
        // 继续运行，让用户看到错误
    }
    
    // 2. 连接到 daemon
    m_client->connectToDaemon();
    
    // 3. 获取初始状态
    m_client->getStatus();
    
    // 4. 加载 QML
    const QUrl url(QStringLiteral("qrc:/qt/qml/LianwallGui/qml/main.qml"));
    
    QObject::connect(m_engine, &QQmlApplicationEngine::objectCreated, m_app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) {
                qCritical() << "[Application] Failed to load QML";
                QCoreApplication::exit(-1);
            }
        }, Qt::QueuedConnection);
    
    m_engine->load(url);
    
    // 5. 显示托盘（如果可用）
    if (m_trayManager) {
        m_trayManager->show();
    }
    
    qDebug() << "[Application] Ready";
    return m_app->exec();
}

void Application::showMainWindow() {
    // QML 中处理窗口显示
    QMetaObject::invokeMethod(m_engine->rootObjects().first(), "show");
}

void Application::hideMainWindow() {
    QMetaObject::invokeMethod(m_engine->rootObjects().first(), "hide");
}

void Application::quit() {
    emit aboutToQuit();
    m_processManager->gracefulShutdown();
    m_app->quit();
}

void Application::restart() {
    m_processManager->restartDaemon();
}

void Application::handleExitRequest() {
    QString behavior = m_configManager->exitBehavior();
    
    if (behavior == "minimize") {
        hideMainWindow();
    } else if (behavior == "shutdown") {
        quit();
    } else {
        // "ask" - QML 中弹窗处理
    }
}
