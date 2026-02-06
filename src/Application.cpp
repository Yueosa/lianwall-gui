#include "Application.h"
#include "ConfigManager.h"
#include "Constants.h"

#include <QQmlContext>
#include <QIcon>
#include <QDebug>

using namespace LianwallGui;

Application::Application(int &argc, char **argv)
    : QObject(nullptr)
    , m_app(new QApplication(argc, argv))
    , m_engine(new QQmlApplicationEngine(this))
    , m_translator(nullptr)
    , m_configManager(nullptr)
{
    // 设置应用信息
    m_app->setApplicationName(APP_NAME);
    m_app->setApplicationVersion(APP_VERSION);
    m_app->setOrganizationName(APP_AUTHOR);
    m_app->setOrganizationDomain("github.com/Yueosa");
    m_app->setWindowIcon(QIcon(":/icons/lianwall.svg"));
    
    // 不在最后一个窗口关闭时退出
    m_app->setQuitOnLastWindowClosed(false);
    
    initComponents();
    loadTranslations();
    registerQmlTypes();
}

Application::~Application() {
    delete m_app;
}

void Application::initComponents() {
    m_configManager = new ConfigManager(this);
}

void Application::loadTranslations() {
    QString lang = m_configManager->language();
    switchLanguage(lang);
}

void Application::switchLanguage(const QString &lang) {
    // 移除旧的翻译器
    if (m_translator) {
        m_app->removeTranslator(m_translator);
        delete m_translator;
        m_translator = nullptr;
    }
    
    // 加载新的翻译
    m_translator = new QTranslator(this);
    if (m_translator->load(QString(":/translations/lianwall-gui_%1.qm").arg(lang))) {
        m_app->installTranslator(m_translator);
        qDebug() << "[Application] Loaded translation:" << lang;
    } else {
        qWarning() << "[Application] Failed to load translation:" << lang;
        delete m_translator;
        m_translator = nullptr;
    }
    
    // 通知 QML 引擎重新翻译
    if (m_engine) {
        m_engine->retranslate();
    }
}

void Application::registerQmlTypes() {
    // 注册单例
    qmlRegisterSingletonInstance("LianwallGui", 1, 0, "ConfigManager", m_configManager);
    
    // 暴露常量
    m_engine->rootContext()->setContextProperty("AppVersion", APP_VERSION);
    m_engine->rootContext()->setContextProperty("AppAuthor", APP_AUTHOR);
    m_engine->rootContext()->setContextProperty("AppGitHub", APP_GITHUB);
    m_engine->rootContext()->setContextProperty("AppDescription", APP_DESCRIPTION);
}

int Application::exec() {
    qDebug() << "[Application] Starting...";
    
    // 加载 QML
    const QUrl url(QStringLiteral("qrc:/qt/qml/LianwallGui/qml/main.qml"));
    
    QObject::connect(m_engine, &QQmlApplicationEngine::objectCreated, m_app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) {
                qCritical() << "[Application] Failed to load QML";
                QCoreApplication::exit(-1);
            }
        }, Qt::QueuedConnection);
    
    m_engine->load(url);
    
    qDebug() << "[Application] Ready";
    return m_app->exec();
}

void Application::showMainWindow() {
    if (!m_engine->rootObjects().isEmpty()) {
        QMetaObject::invokeMethod(m_engine->rootObjects().first(), "show");
    }
}

void Application::hideMainWindow() {
    if (!m_engine->rootObjects().isEmpty()) {
        QMetaObject::invokeMethod(m_engine->rootObjects().first(), "hide");
    }
}

void Application::quit() {
    emit aboutToQuit();
    m_app->quit();
}
