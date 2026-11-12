#include "Application.h"

#include <QDebug>
#include <QLocalSocket>
#include <QLocalServer>
#include <csignal>
#include <QCommandLineParser>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>

static const QString kServerName = QStringLiteral("lianwall-gui-single-instance");

// 优雅处理 SIGINT/SIGTERM（Ctrl+C / systemctl stop）
static Application *g_app = nullptr;

static void signalHandler(int sig)
{
    Q_UNUSED(sig)
    if (g_app)
        QMetaObject::invokeMethod(g_app, "quit", Qt::QueuedConnection);
}

/// 尝试连接已有实例，成功则通知其显示窗口并返回 true
static bool tryActivateExisting(const QString &serverName)
{
    QLocalSocket socket;
    socket.connectToServer(serverName);
    if (socket.waitForConnected(500)) {
        socket.write("show");
        socket.waitForBytesWritten(500);
        socket.disconnectFromServer();
        return true;
    }
    return false;
}

int main(int argc, char *argv[])
{
    // Application 构造仅创建 QApplication + 设置元信息（轻量级）
    Application app(argc, argv);
    g_app = &app;

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption socketOption(QStringLiteral("socket"),
        QStringLiteral("Daemon Unix socket path"), QStringLiteral("path"), QStringLiteral("/tmp/lianwall.sock"));
    parser.addOption(socketOption);
    parser.process(*QCoreApplication::instance());
    QString socketPath = parser.value(socketOption);
    if (socketPath.isEmpty()) {
        qCritical() << "--socket requires a nonempty path";
        return 2;
    }
    if (socketPath.startsWith(QStringLiteral("~/")))
        socketPath = QDir::homePath() + socketPath.mid(1);
    socketPath = QDir::cleanPath(QFileInfo(socketPath).absoluteFilePath());
    const QString serverName = socketPath == QStringLiteral("/tmp/lianwall.sock") ? kServerName
        : kServerName + '-' + QString::fromLatin1(QCryptographicHash::hash(socketPath.toUtf8(), QCryptographicHash::Sha256).toHex());

    // 单实例检测：如果已有实例在运行，激活它并退出
    if (tryActivateExisting(serverName)) {
        qDebug() << "[Main] Another instance is running, activating it.";
        return 0;
    }

    // 清除可能残留的旧 server，然后创建新的
    QLocalServer::removeServer(serverName);

    auto *server = new QLocalServer(&app);
    if (!server->listen(serverName)) {
        qWarning() << "[Main] Failed to create single-instance server:" << server->errorString();
    }

    QObject::connect(server, &QLocalServer::newConnection, [&app, server]() {
        auto *client = server->nextPendingConnection();
        if (client) {
            // 收到来自新实例的激活请求 → 显示窗口
            QObject::connect(client, &QLocalSocket::readyRead, [&app, client]() {
                client->readAll(); // 读取 "show"
                app.showMainWindow();
                client->deleteLater();
            });
            QObject::connect(client, &QLocalSocket::disconnected, client, &QObject::deleteLater);
        }
    });

    // 单实例检测通过，进行完整初始化
    app.init(socketPath);

    std::signal(SIGINT,  signalHandler);
    std::signal(SIGTERM, signalHandler);

    return app.exec();
}
