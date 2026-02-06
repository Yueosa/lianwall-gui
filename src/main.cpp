#include "Application.h"

#include <QDebug>
#include <csignal>

// 优雅处理 SIGINT/SIGTERM（Ctrl+C / systemctl stop）
static Application *g_app = nullptr;

static void signalHandler(int sig)
{
    Q_UNUSED(sig)
    if (g_app)
        QMetaObject::invokeMethod(g_app, "quit", Qt::QueuedConnection);
}

int main(int argc, char *argv[])
{
    Application app(argc, argv);
    g_app = &app;

    std::signal(SIGINT,  signalHandler);
    std::signal(SIGTERM, signalHandler);

    return app.exec();
}
