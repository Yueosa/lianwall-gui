#include "WallpaperController.h"
#include "DaemonClient.h"
#include <QPointer>
#include <utility>

WallpaperController::WallpaperController(DaemonClient *client, QObject *parent)
    : QObject(parent), m_client(client)
{
}

bool WallpaperController::submit(const std::function<void(Callback)> &send)
{
    if (m_busy || !m_client->isConnected())
        return false;
    m_busy = true;
    const quint64 operation = ++m_operation;
    emit busyChanged();
    const QPointer<WallpaperController> guard(this);
    send([guard, operation](const Daemon::Response &response) {
        if (!guard || operation != guard->m_operation)
            return;
        guard->m_busy = false;
        emit guard->busyChanged();
        if (!guard)
            return;
        if (response.type == Daemon::ResponseType::Error) {
            const QString code = response.payload.value(QStringLiteral("code")).toString();
            if (code == QLatin1String("timeout") || code == QLatin1String("connection_lost")
                || code == QLatin1String("protocol_error")) {
                emit guard->errorOccurred(QStringLiteral("result_unknown"),
                    tr("The operation result could not be confirmed. Reconnecting to refresh the current wallpaper."));
            } else {
                emit guard->errorOccurred(code, response.asError().message);
            }
        }
        if (guard)
            emit guard->refreshRequested();
    });
    return true;
}

bool WallpaperController::next()
{
    return submit([this](Callback cb) { m_client->next(std::move(cb)); });
}

bool WallpaperController::prev()
{
    return submit([this](Callback cb) { m_client->prev(std::move(cb)); });
}

bool WallpaperController::setMode(Daemon::WallMode mode)
{
    return submit([this, mode](Callback cb) { m_client->setMode(mode, std::move(cb)); });
}

bool WallpaperController::setWallpaper(const QString &path)
{
    return submit([this, path](Callback cb) { m_client->setWallpaper(path, std::move(cb)); });
}
