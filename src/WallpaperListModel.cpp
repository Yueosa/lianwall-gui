#include "WallpaperListModel.h"
#include "DaemonClient.h"
#include "WallpaperController.h"
#include <QPointer>

#include <QFileInfo>
#include <QDebug>

// ============================================================================
// 视频扩展名集合
// ============================================================================

static const QStringList kVideoExtensions = {
    "mp4", "mkv", "webm", "avi", "mov", "flv", "wmv", "m4v", "3gp", "ogv", "ts", "m2ts",
};

// ============================================================================
// WallpaperListModel
// ============================================================================

WallpaperListModel::WallpaperListModel(DaemonClient *client, WallpaperController *controller, QObject *parent)
    : QAbstractListModel(parent)
    , m_client(client)
    , m_wallpaperController(controller)
{
    connect(client, &DaemonClient::connectionChanged, this, [this](bool connected) {
        if (connected)
            return;
        ++m_loadGeneration;
        ++m_contentRevision;
        m_reloadPending = false;
        m_pendingLockPaths.clear();
        if (m_loading) {
            m_loading = false;
            emit loadingChanged();
        }
    });
    connect(client, &DaemonClient::eventReceived, this, [this](const Daemon::DaemonEvent &event) {
        if (event.type == Daemon::EventType::SpaceUpdated
            || event.type == Daemon::EventType::WallpaperChanged
            || event.type == Daemon::EventType::StatusChanged)
            ++m_contentRevision;
    });
}

int WallpaperListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant WallpaperListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return {};

    const auto &item = m_items.at(index.row());

    switch (role) {
    case PathRole:        return item.path;
    case FilenameRole:    return item.filename;
    case AngleRole:       return item.angle;
    case LockedRole:      return item.locked;
    case InCooldownRole:  return item.inCooldown;
    case IsCurrentRole:   return item.isCurrent;
    case IndexRole:       return item.index;
    case IsVideoRole:     return isVideoFile(item.filename);
    }
    return {};
}

QHash<int, QByteArray> WallpaperListModel::roleNames() const
{
    return {
        { PathRole,       "wallpaperPath" },
        { FilenameRole,   "wallpaperFilename" },
        { AngleRole,      "wallpaperAngle" },
        { LockedRole,     "wallpaperLocked" },
        { InCooldownRole, "wallpaperInCooldown" },
        { IsCurrentRole,  "wallpaperIsCurrent" },
        { IndexRole,      "wallpaperIndex" },
        { IsVideoRole,    "wallpaperIsVideo" },
    };
}

void WallpaperListModel::load(const QString &mode)
{
    m_requestedMode = mode;
    if (m_loading) {
        m_reloadPending = true;
        return;
    }
    if (!m_client->isConnected())
        return;
    m_loading = true;
    m_reloadPending = false;
    const quint64 generation = ++m_loadGeneration;
    const quint64 revision = m_contentRevision;
    emit loadingChanged();
    std::optional<Daemon::WallMode> queryMode;
    if (!mode.isEmpty())
        queryMode = Daemon::wallModeFromString(mode);
    const QPointer<WallpaperListModel> guard(this);
    m_client->getSpace(queryMode, [guard, generation, revision](const Daemon::Response &response) {
        if (!guard || generation != guard->m_loadGeneration)
            return;
        const bool again = guard->m_reloadPending || revision != guard->m_contentRevision;
        if (!again && response.type == Daemon::ResponseType::Space) {
            const auto space = response.asSpace();
            guard->beginResetModel();
            guard->m_items = space.items;
            guard->endResetModel();
            ++guard->m_refreshToken;
            emit guard->refreshTokenChanged();
            guard->m_mode = Daemon::wallModeToString(space.mode);
            emit guard->modeChanged();
            emit guard->countChanged();
        } else if (response.type == Daemon::ResponseType::Error) {
            emit guard->errorOccurred(response.asError().message);
        }
        if (!guard)
            return;
        guard->m_loading = false;
        guard->m_reloadPending = false;
        emit guard->loadingChanged();
        if (guard && again && !guard->m_loading && guard->m_client->isConnected())
            guard->load(guard->m_requestedMode);
    });
}

void WallpaperListModel::toggleLock(int row)
{
    if (row < 0 || row >= m_items.size())
        return;

    const auto path = m_items.at(row).path;
    if (m_pendingLockPaths.contains(path))
        return;

    m_pendingLockPaths.insert(path);
    qDebug() << "[WallpaperListModel] ToggleLock:" << m_items.at(row).filename;

    const bool targetLocked = !m_items.at(row).locked;
    const QPointer<WallpaperListModel> guard(this);
    m_client->toggleLock(path, [guard, path, targetLocked](const Daemon::Response &response) {
        if (!guard)
            return;
        guard->m_pendingLockPaths.remove(path);
        if (response.type == Daemon::ResponseType::Ok) {
            for (int i = 0; i < guard->m_items.size(); ++i) {
                if (guard->m_items[i].path == path) {
                    guard->m_items[i].locked = targetLocked;
                    const auto idx = guard->index(i);
                    emit guard->dataChanged(idx, idx, { LockedRole });
                    break;
                }
            }
        } else if (response.type == Daemon::ResponseType::Error) {
            emit guard->errorOccurred(response.asError().message);
        }
    });
}

void WallpaperListModel::setAsCurrent(int row)
{
    if (row < 0 || row >= m_items.size())
        return;
    m_wallpaperController->setWallpaper(m_items.at(row).path);
}

QString WallpaperListModel::pathAt(int row) const
{
    if (row < 0 || row >= m_items.size())
        return {};
    return m_items.at(row).path;
}

QString WallpaperListModel::filenameAt(int row) const
{
    if (row < 0 || row >= m_items.size())
        return {};
    return m_items.at(row).filename;
}

bool WallpaperListModel::lockedAt(int row) const
{
    if (row < 0 || row >= m_items.size())
        return false;
    return m_items.at(row).locked;
}

bool WallpaperListModel::isCurrentAt(int row) const
{
    if (row < 0 || row >= m_items.size())
        return false;
    return m_items.at(row).isCurrent;
}

bool WallpaperListModel::isVideoFile(const QString &filename)
{
    auto ext = QFileInfo(filename).suffix().toLower();
    return kVideoExtensions.contains(ext);
}

void WallpaperListModel::toggleLockByPath(const QString &path)
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].path == path) {
            toggleLock(i);
            return;
        }
    }
}

void WallpaperListModel::setAsCurrentByPath(const QString &path)
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].path == path) {
            setAsCurrent(i);
            return;
        }
    }
}

// ============================================================================
// WallpaperFilterModel
// ============================================================================

WallpaperFilterModel::WallpaperFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    // 数据变化时更新 count
    connect(this, &QAbstractItemModel::rowsInserted,  this, &WallpaperFilterModel::countChanged);
    connect(this, &QAbstractItemModel::rowsRemoved,   this, &WallpaperFilterModel::countChanged);
    connect(this, &QAbstractItemModel::modelReset,    this, &WallpaperFilterModel::countChanged);
    connect(this, &QAbstractItemModel::layoutChanged, this, &WallpaperFilterModel::countChanged);
}

void WallpaperFilterModel::setSearchText(const QString &text)
{
    if (m_searchText == text)
        return;
    m_searchText = text;
    emit searchTextChanged();
    beginFilterChange();
    endFilterChange();
}

void WallpaperFilterModel::setLockFilter(int filter)
{
    if (m_lockFilter == filter)
        return;
    m_lockFilter = filter;
    emit lockFilterChanged();
    beginFilterChange();
    endFilterChange();
}

bool WallpaperFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    auto srcModel = sourceModel();
    if (!srcModel)
        return true;

    auto idx = srcModel->index(sourceRow, 0, sourceParent);

    // 搜索过滤（文件名）
    if (!m_searchText.isEmpty()) {
        auto filename = idx.data(WallpaperListModel::FilenameRole).toString();
        if (!filename.contains(m_searchText, Qt::CaseInsensitive))
            return false;
    }

    // 锁定状态过滤
    if (m_lockFilter >= 0) {
        bool locked = idx.data(WallpaperListModel::LockedRole).toBool();
        if (m_lockFilter == 0 && locked)     return false;   // 只显示未锁定
        if (m_lockFilter == 1 && !locked)    return false;   // 只显示已锁定
    }

    return true;
}

// --- 透传方法 ---

void WallpaperFilterModel::toggleLock(int proxyRow)
{
    auto src = qobject_cast<WallpaperListModel*>(sourceModel());
    if (!src) return;
    auto srcIdx = mapToSource(index(proxyRow, 0));
    src->toggleLock(srcIdx.row());
}

void WallpaperFilterModel::setAsCurrent(int proxyRow)
{
    auto src = qobject_cast<WallpaperListModel*>(sourceModel());
    if (!src) return;
    auto srcIdx = mapToSource(index(proxyRow, 0));
    src->setAsCurrent(srcIdx.row());
}

QString WallpaperFilterModel::pathAt(int proxyRow) const
{
    auto src = qobject_cast<WallpaperListModel*>(sourceModel());
    if (!src) return {};
    auto srcIdx = mapToSource(index(proxyRow, 0));
    return src->pathAt(srcIdx.row());
}

QString WallpaperFilterModel::filenameAt(int proxyRow) const
{
    auto src = qobject_cast<WallpaperListModel*>(sourceModel());
    if (!src) return {};
    auto srcIdx = mapToSource(index(proxyRow, 0));
    return src->filenameAt(srcIdx.row());
}

bool WallpaperFilterModel::lockedAt(int proxyRow) const
{
    auto src = qobject_cast<WallpaperListModel*>(sourceModel());
    if (!src) return false;
    auto srcIdx = mapToSource(index(proxyRow, 0));
    return src->lockedAt(srcIdx.row());
}

bool WallpaperFilterModel::isCurrentAt(int proxyRow) const
{
    auto src = qobject_cast<WallpaperListModel*>(sourceModel());
    if (!src) return false;
    auto srcIdx = mapToSource(index(proxyRow, 0));
    return src->isCurrentAt(srcIdx.row());
}

bool WallpaperFilterModel::isVideoAt(int proxyRow) const
{
    auto src = qobject_cast<WallpaperListModel*>(sourceModel());
    if (!src) return false;
    auto srcIdx = mapToSource(index(proxyRow, 0));
    auto filename = src->filenameAt(srcIdx.row());
    return WallpaperListModel::isVideoFile(filename);
}

void WallpaperFilterModel::toggleLockByPath(const QString &path)
{
    auto src = qobject_cast<WallpaperListModel*>(sourceModel());
    if (src) src->toggleLockByPath(path);
}

void WallpaperFilterModel::setAsCurrentByPath(const QString &path)
{
    auto src = qobject_cast<WallpaperListModel*>(sourceModel());
    if (src) src->setAsCurrentByPath(path);
}
