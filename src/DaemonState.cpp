#include "DaemonState.h"
#include "DaemonClient.h"

#include <QDebug>
#include <QPointer>

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
    if (m_refreshInFlight) {
        m_refreshPending = true;
        return;
    }
    m_refreshInFlight = true;
    m_refreshPending = false;
    const quint64 generation = ++m_refreshGeneration;
    const quint64 revision = m_revision;
    const QPointer<DaemonState> guard(this);
    m_client->getStatus([guard, generation, revision](const Daemon::Response &response) {
        if (!guard || generation != guard->m_refreshGeneration)
            return;
        const bool again = guard->m_refreshPending || revision != guard->m_revision;
        guard->m_refreshInFlight = false;
        guard->m_refreshPending = false;
        if (!again && response.type == Daemon::ResponseType::Status)
            guard->applyStatus(response.asStatus());
        if (again && guard->m_client->isConnected())
            guard->refresh();
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
    ++m_revision;
    ++m_refreshGeneration;
    m_refreshInFlight = false;
    m_refreshPending = false;
    if (!connected) {
        m_nextSwitchSecs = -1;
        emit nextSwitchSecsChanged();
    }
    emit daemonConnectedChanged();

    if (connected) {
        // 连接后立即订阅所有事件 + immediate_sync
        m_client->subscribe({Daemon::EventType::All}, true);
    }
}

void DaemonState::onInitialStatus(const Daemon::StatusPayload &status)
{
    ++m_revision;
    applyStatus(status);
}

void DaemonState::onEvent(const Daemon::DaemonEvent &event)
{
    ++m_revision;

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
        refresh();
        break;
    }

    case Daemon::EventType::SpaceUpdated: {
        emit spaceUpdated(Daemon::wallModeToString(event.asSpaceUpdated().mode));
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

        // 仅在明确的降级/恢复动作时更新降级状态
        // Keep 表示维持现状，不应改变 degraded 标志
        if (d.action == Daemon::VramAction::Downgrade && !m_vramDegraded) {
            m_vramDegraded = true;
            emit vramDegradedChanged();
        } else if (d.action == Daemon::VramAction::Upgrade && m_vramDegraded) {
            m_vramDegraded = false;
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
            Daemon::errorCodeToString(d.code),
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

    // 始终更新并发信号：CountdownBar 本地递减与 daemon 值脱钩，
    // 相等性检查会导致倒计时走到 0 后永远收不到新值。
    m_nextSwitchSecs = s.nextSwitchSecs;
    emit nextSwitchSecsChanged();

    if (m_nextTimePoint != s.nextTimePoint) { m_nextTimePoint = s.nextTimePoint; emit nextTimePointChanged(); }
    if (m_timePointsCount != s.timePointsCount) { m_timePointsCount = s.timePointsCount; emit timePointsCountChanged(); }
    if (m_protocolVersion != s.protocolVersion) { m_protocolVersion = s.protocolVersion; emit protocolVersionChanged(); }
}
