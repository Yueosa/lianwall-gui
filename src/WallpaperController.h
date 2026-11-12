#pragma once

#include "DaemonTypes.h"
#include <QObject>
#include <functional>

class DaemonClient;

/// One wallpaper mutation at a time, shared by dashboard, tray and library.
class WallpaperController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
public:
    explicit WallpaperController(DaemonClient *client, QObject *parent = nullptr);
    bool busy() const { return m_busy; }
    bool next();
    bool prev();
    bool setMode(Daemon::WallMode mode);
    bool setWallpaper(const QString &path);

signals:
    void busyChanged();
    void refreshRequested();
    void errorOccurred(const QString &code, const QString &message);

private:
    using Callback = std::function<void(const Daemon::Response &)>;
    bool submit(const std::function<void(Callback)> &send);
    DaemonClient *m_client;
    bool m_busy = false;
    quint64 m_operation = 0;
};
