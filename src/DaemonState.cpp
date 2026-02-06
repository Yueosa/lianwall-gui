#include "DaemonState.h"
#include "DaemonClient.h"

#include <QDebug>

DaemonState::DaemonState(DaemonClient *client, QObject *parent)
    : QObject(parent)
    , m_client(client)
{
    connect(m_client, &DaemonClient::connectionChanged,
            this, &DaemonState::onConnectionChanged);
    connect(m_client, &DaemonClient::initialStatusReceived,
            this, &DaemonState::onInitialStatus);
    connect(m_client, &DaemonClient::eventReceived,
            this, &DaemonState::onEvent);
}

// ============================================================================
// QML 可调用
// ============================================================================

void DaemonState::refresh()
{
    if (!m_client->isConnected())
        return;

    m_client->getStatus([this](const Daemon::Response &r) {
        if (r.type == Daemon::ResponseType::Status)
            applyStatus(r.asStatus());
    });
}

// ============================================================================
// 槽
// ============================================================================

void DaemonState::onConnectionChanged(bool connected)
{
    if (m_connected == connected)
        return;
    m_connected = connected;
    emit daemonConnectedChanged();

    if (connected) {
        // 连接后立即订阅所有事件 + immediate_sync
        m_client->subscribe({Daemon::EventType::All}, true);
    }
}

void DaemonState::onInitialStatus(const Daemon::StatusPayload &status)
{
    applyStatus(status);
}

void DaemonState::onEvent(const Daemon::DaemonEvent &event)
{
    switch (event.type) {

    case Daemon::EventType::WallpaperChanged: {
        auto d = event.asWallpaperChanged();
        if (m_currentPath != d.path) {
            m_currentPath = d.path;
            emit currentPathChanged();
        }
        if (m_currentFilename != d.filename) {
            m_currentFilename = d.filename;
            emit currentFilenameChanged();
        }
        auto modeStr = Daemon::wallModeToString(d.mode);
        if (m_mode != modeStr) {
            m_mode = modeStr;
            emit modeChanged();
        }
        emit wallpaperChanged(d.path, d.filename,
                              Daemon::triggerToString(d.trigger));

        // 壁纸切换后主动刷新完整状态（获取新的 nextSwitchSecs 等）
        refresh();
        break;
    }

    case Daemon::EventType::StatusChanged: {
        auto d = event.asStatusChanged();
        for (const auto &change : d.changes) {
            if (change.field == QLatin1String("Mode")) {
                auto v = change.value.toString();
                if (m_mode != v) { m_mode = v; emit modeChanged(); }
            } else if (change.field == QLatin1String("Engine")) {
                auto v = change.value.toString();
                if (m_engine != v) { m_engine = v; emit engineChanged(); }
            } else if (change.field == QLatin1String("TotalWallpapers")) {
                auto v = change.value.toInt();
                if (m_totalWallpapers != v) { m_totalWallpapers = v; emit totalWallpapersChanged(); }
            } else if (change.field == QLatin1String("AvailableCount")) {
                auto v = change.value.toInt();
                if (m_availableCount != v) { m_availableCount = v; emit availableCountChanged(); }
            } else if (change.field == QLatin1String("LockedCount")) {
                auto v = change.value.toInt();
                if (m_lockedCount != v) { m_lockedCount = v; emit lockedCountChanged(); }
            } else if (change.field == QLatin1String("VramDegraded")) {
                auto v = change.value.toBool();
                if (m_vramDegraded != v) { m_vramDegraded = v; emit vramDegradedChanged(); }
            }
        }
        break;
    }

    case Daemon::EventType::SpaceUpdated: {
        // 空间更新后刷新完整状态
        refresh();
        break;
    }

    case Daemon::EventType::VramChanged: {
        auto d = event.asVramChanged();
        bool changed = false;
        if (m_vramUsedMb != d.usedMb || m_vramTotalMb != d.totalMb) {
            m_vramUsedMb = d.usedMb;
            m_vramTotalMb = d.totalMb;
            changed = true;
        }
        if (changed)
            emit vramChanged();

        bool degraded = (d.action == Daemon::VramAction::Downgrade);
        if (m_vramDegraded != degraded) {
            m_vramDegraded = degraded;
            emit vramDegradedChanged();
        }
        break;
    }

    case Daemon::EventType::TimePointReached: {
        // 时间点到达后刷新完整状态（壁纸可能重建）
        refresh();
        break;
    }

    case Daemon::EventType::ScanProgress: {
        auto d = event.asScanProgress();
        emit scanProgressUpdated(
            Daemon::wallModeToString(d.mode),
            d.filesFound, d.completed);
        break;
    }

    case Daemon::EventType::ConfigChanged: {
        // 配置变化后刷新完整状态
        refresh();
        break;
    }

    case Daemon::EventType::Error: {
        auto d = event.asError();
        qWarning() << "[DaemonState] Error event:" << d.message;
        emit daemonError(
            QString(), // errorCodeToString not exposed, use raw message
            d.message, d.recoverable);
        break;
    }

    default:
        break;
    }
}

// ============================================================================
// 内部
// ============================================================================

void DaemonState::applyStatus(const Daemon::StatusPayload &s)
{
    auto modeStr = Daemon::wallModeToString(s.mode);
    if (m_mode != modeStr) { m_mode = modeStr; emit modeChanged(); }
    if (m_currentPath != s.current) { m_currentPath = s.current; emit currentPathChanged(); }
    if (m_currentFilename != s.currentFilename) { m_currentFilename = s.currentFilename; emit currentFilenameChanged(); }
    if (m_engine != s.engine) { m_engine = s.engine; emit engineChanged(); }
    if (m_totalWallpapers != s.totalWallpapers) { m_totalWallpapers = s.totalWallpapers; emit totalWallpapersChanged(); }
    if (m_lockedCount != s.lockedCount) { m_lockedCount = s.lockedCount; emit lockedCountChanged(); }
    if (m_availableCount != s.availableCount) { m_availableCount = s.availableCount; emit availableCountChanged(); }
    if (m_scannedCount != s.scannedCount) { m_scannedCount = s.scannedCount; emit scannedCountChanged(); }

    if (m_vramUsedMb != s.vramUsedMb || m_vramTotalMb != s.vramTotalMb) {
        m_vramUsedMb = s.vramUsedMb;
        m_vramTotalMb = s.vramTotalMb;
        emit vramChanged();
    }
    if (m_vramDegraded != s.vramDegraded) { m_vramDegraded = s.vramDegraded; emit vramDegradedChanged(); }
    if (m_uptimeSecs != s.uptimeSecs) { m_uptimeSecs = s.uptimeSecs; emit uptimeChanged(); }
    if (m_nextSwitchSecs != s.nextSwitchSecs) { m_nextSwitchSecs = s.nextSwitchSecs; emit nextSwitchSecsChanged(); }
    if (m_nextTimePoint != s.nextTimePoint) { m_nextTimePoint = s.nextTimePoint; emit nextTimePointChanged(); }
    if (m_timePointsCount != s.timePointsCount) { m_timePointsCount = s.timePointsCount; emit timePointsCountChanged(); }
    if (m_protocolVersion != s.protocolVersion) { m_protocolVersion = s.protocolVersion; emit protocolVersionChanged(); }
}
