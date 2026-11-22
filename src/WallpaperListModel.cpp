#include "WallpaperListModel.h"
#include "DaemonClient.h"
#include "WallpaperController.h"
#include <QPointer>

#include <QFileInfo>
#include <QDateTime>
#include <memory>
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
        m_metadataUnsupported = false;
        ++m_metadataEpoch;
        ++m_metadataGeneration;
        m_metadataLoading = false; m_metadataAgain = false; m_metadataAvailable = false;
        m_metadata.clear(); m_metadataBuffer.clear(); m_pendingFavorites.clear(); m_metadataError.clear();
        emit metadataChanged();
        if (!m_items.isEmpty()) emit dataChanged(index(0), index(m_items.size() - 1));
        ++m_loadGeneration;
        ++m_contentRevision;
        m_reloadPending = false;
        m_pendingLockPaths.clear();
        if (m_loading) {
            m_loading = false;
            emit loadingChanged();
        }
    });
    const auto updateCurrent = [this](const QString &path) {
        if (m_activePath == path) return;
        m_activePath = path;
        if (!m_items.isEmpty()) emit dataChanged(index(0), index(m_items.size() - 1), {IsCurrentRole});
    };
    connect(client, &DaemonClient::initialStatusReceived, this, [updateCurrent](const Daemon::StatusPayload &status) { updateCurrent(status.current); });
    connect(client, &DaemonClient::eventReceived, this, [updateCurrent](const Daemon::DaemonEvent &event) {
        if (event.type == Daemon::EventType::WallpaperChanged) updateCurrent(event.asWallpaperChanged().path);
    });
    connect(client, &DaemonClient::eventReceived, this, [this](const Daemon::DaemonEvent &event) {
        if (event.type == Daemon::EventType::LibraryChanged && m_metadataAvailable) refreshMetadata();
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
    case IsCurrentRole:   return m_activePath.isEmpty() ? (m_mode == QLatin1String("All") ? false : item.isCurrent) : item.path == m_activePath;
    case IndexRole:       return item.index;
    case FavoriteRole: return m_metadata.value(item.path).value("favorite", false);
    case PlayCountRole: return m_metadata.value(item.path).value("play_count", 0);
    case DisplaySecondsRole: return m_metadata.value(item.path).value("display_seconds", 0);
    case AddedAtRole: return m_metadata.value(item.path).value("added_at", 0);
    case LastUsedRole: return m_metadata.value(item.path).value("last_used", 0);
    case OutsideScheduleRole: return index.row() >= m_spaceCount;
    case TimeConstraintsRole: return item.timeConstraints;
    case ModifiedRole:   return m_modifiedTimes.value(item.path);
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
        { FavoriteRole, "wallpaperFavorite" }, { PlayCountRole, "wallpaperPlayCount" },
        { DisplaySecondsRole, "wallpaperDisplaySeconds" }, { AddedAtRole, "wallpaperAddedAt" }, { LastUsedRole, "wallpaperLastUsed" },
        { OutsideScheduleRole, "wallpaperOutsideSchedule" },
        { TimeConstraintsRole, "wallpaperTimeConstraints" },
        { ModifiedRole, "wallpaperModified" },
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
    struct Result { int remaining; QVector<Daemon::WallpaperPoint> items; QString error; QString mode; };
    const bool all = mode == QLatin1String("All");
    const auto result = std::make_shared<Result>();
    result->remaining = all ? 2 : 1;
    result->mode = all ? QStringLiteral("All") : mode;
    const QPointer<WallpaperListModel> guard(this);
    auto received = [guard, generation, revision, result](const Daemon::Response &response) {
        if (!guard || generation != guard->m_loadGeneration) return;
        if (response.type == Daemon::ResponseType::Space) {
            const auto space = response.asSpace();
            result->items += space.items;
            if (result->mode.isEmpty()) result->mode = Daemon::wallModeToString(space.mode);
        } else result->error = response.asError().message;
        if (--result->remaining) return;
        const bool again = guard->m_reloadPending || revision != guard->m_contentRevision;
        if (!again && result->error.isEmpty()) {
            guard->beginResetModel();
            guard->m_items.clear(); guard->m_modifiedTimes.clear();
            QSet<QString> paths;
            for (const auto &item : result->items) {
                if (paths.contains(item.path)) continue;
                paths.insert(item.path); guard->m_items.append(item);
                guard->m_modifiedTimes.insert(item.path, QFileInfo(item.path).lastModified().toMSecsSinceEpoch());
            }
            guard->m_spaceCount = guard->m_items.size();
            guard->m_mode = result->mode;
            guard->endResetModel();
            guard->syncScheduledRows();
            ++guard->m_refreshToken; emit guard->refreshTokenChanged();
            emit guard->modeChanged(); emit guard->countChanged();
        } else if (!result->error.isEmpty()) emit guard->errorOccurred(result->error);
        if (!guard) return;
        guard->m_loading = false; guard->m_reloadPending = false;
        emit guard->loadingChanged();
        if (guard && again && guard->m_client->isConnected()) guard->load(guard->m_requestedMode);
    };
    if (all) {
        m_client->getSpace(Daemon::WallMode::Video, received);
        m_client->getSpace(Daemon::WallMode::Image, received);
    } else {
        m_client->getSpace(mode.isEmpty() ? std::nullopt : std::optional<Daemon::WallMode>(Daemon::wallModeFromString(mode)), received);
    }
}

void WallpaperListModel::syncScheduledRows()
{
    QSet<QString> active;
    for (int i = 0; i < m_spaceCount; ++i)
        active.insert(m_items[i].path);

    QVector<Daemon::WallpaperPoint> scheduled;
    for (auto it = m_metadata.cbegin(); it != m_metadata.cend(); ++it) {
        const auto ranges = it.value().value("scanned_time_constraints").toStringList();
        if (ranges.isEmpty() || active.contains(it.key())) continue;
        Daemon::WallpaperPoint point;
        point.path = it.key();
        point.filename = QFileInfo(point.path).fileName();
        if (m_mode != "All" && (m_mode == "Video") != isVideoFile(point.filename)) continue;
        point.timeConstraints = ranges;
        scheduled.append(point);
    }
    bool unchanged = m_items.size() == m_spaceCount + scheduled.size();
    for (int i = 0; unchanged && i < scheduled.size(); ++i) {
        const auto &old = m_items[m_spaceCount + i];
        unchanged = old.path == scheduled[i].path && old.timeConstraints == scheduled[i].timeConstraints;
    }
    if (unchanged) return;
    if (m_items.size() > m_spaceCount) {
        beginRemoveRows({}, m_spaceCount, m_items.size() - 1);
        while (m_items.size() > m_spaceCount)
            m_modifiedTimes.remove(m_items.takeLast().path);
        endRemoveRows();
    }
    if (!scheduled.isEmpty()) {
        beginInsertRows({}, m_spaceCount, m_spaceCount + scheduled.size() - 1);
        for (const auto &point : scheduled)
            m_modifiedTimes.insert(point.path, QFileInfo(point.path).lastModified().toMSecsSinceEpoch());
        m_items += scheduled;
        endInsertRows();
    }
    emit countChanged();
}

void WallpaperListModel::refreshMetadata()
{
    if (!m_client->isConnected()) return;
    if (m_metadataLoading) { m_metadataAgain = true; return; }
    m_metadataLoading = true; m_metadataAgain = false; m_metadataRestarts = 0;
    m_metadataRevision = -1; m_metadataBuffer.clear();
    metadataPage(0, ++m_metadataGeneration);
}

void WallpaperListModel::metadataPage(int offset, quint64 generation)
{
    const QPointer<WallpaperListModel> guard(this);
    m_client->query("GetLibrary", {{"offset", offset}, {"limit", 128}}, [guard, offset, generation](const Daemon::Response &response) {
        if (!guard || generation != guard->m_metadataGeneration) return;
        auto finish = [guard]() {
            guard->m_metadataLoading = false;
            emit guard->metadataChanged();
            if (!guard) return;
            if (!guard->m_items.isEmpty()) emit guard->dataChanged(guard->index(0), guard->index(guard->m_items.size() - 1));
            if (guard && guard->m_metadataAgain) guard->refreshMetadata();
        };
        if (response.type != Daemon::ResponseType::Library) {
            guard->m_metadataUnsupported = response.type == Daemon::ResponseType::Error && response.asError().code == Daemon::ErrorCode::InvalidRequest;
            guard->m_metadataError = response.asError().message;
            guard->m_metadataAvailable = false; finish(); return;
        }
        const auto payload = response.payload;
        if (payload.value("schema_version").toInt() != 1) {
            guard->m_metadataUnsupported = true;
            guard->m_metadataAvailable = false; guard->m_metadataError = tr("不支持的资料库版本"); finish(); return;
        }
        const qint64 revision = payload.value("revision").toInteger();
        if (offset && revision != guard->m_metadataRevision) {
            if (++guard->m_metadataRestarts <= 2) {
                guard->m_metadataBuffer.clear(); guard->m_metadataRevision = -1;
                guard->metadataPage(0, generation); return;
            }
            guard->m_metadataError = tr("资料库正在更新，请稍后刷新"); finish(); return;
        }
        guard->m_metadataRevision = revision;
        for (const auto &value : payload.value("entries").toArray()) {
            const auto entry = value.toObject(); guard->m_metadataBuffer.insert(entry.value("path").toString(), entry.toVariantMap());
        }
        const auto next = payload.value("next_offset");
        if (next.isDouble()) {
            const int cursor = next.toInt(-1);
            if (cursor <= offset || guard->m_metadataBuffer.size() > 200000) {
                guard->m_metadataError = tr("资料库分页响应无效或过大"); finish(); return;
            }
            guard->metadataPage(cursor, generation); return;
        }
        guard->m_metadata = std::move(guard->m_metadataBuffer);
        guard->m_metadataUnsupported = false;
        guard->m_metadataAvailable = true;
        guard->syncScheduledRows();
        guard->m_metadataError = payload.value("persistence_error").toString();
        finish();
    });
}

void WallpaperListModel::setFavorite(const QString &path, bool favorite)
{
    if (!m_client->isConnected() || !m_metadataAvailable || !m_metadata.contains(path) || m_pendingFavorites.contains(path)) return;
    m_pendingFavorites.insert(path); emit metadataChanged();
    const quint64 epoch = m_metadataEpoch;
    const QPointer<WallpaperListModel> guard(this);
    m_client->query("SetFavorite", {{"path", path}, {"favorite", favorite}}, [guard, path, epoch](const Daemon::Response &response) {
        if (!guard) return;
        // Pending requests are failed at disconnect; never accept an old session's result.
        if (!guard->m_client->isConnected() || epoch != guard->m_metadataEpoch) return;
        guard->m_pendingFavorites.remove(path); emit guard->metadataChanged();
        if (!guard) return;
        if (response.type == Daemon::ResponseType::Error) emit guard->errorOccurred(response.asError().message);
        else guard->refreshMetadata();
    });
}

void WallpaperListModel::toggleLock(int row)
{
    if (row < 0 || row >= m_spaceCount)
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
    if (row < 0 || row >= m_spaceCount)
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

void WallpaperFilterModel::setSortKey(const QString &key)
{
    static const QHash<QString, int> roles{{"name", WallpaperListModel::FilenameRole}, {"modified", WallpaperListModel::ModifiedRole}, {"added", WallpaperListModel::AddedAtRole},
        {"count", WallpaperListModel::PlayCountRole}, {"duration", WallpaperListModel::DisplaySecondsRole}, {"recent", WallpaperListModel::LastUsedRole}};
    if (!roles.contains(key) || key == m_sortKey) return;
    m_sortKey = key;
    setSortRole(roles.value(key));
    sort(0, key == QLatin1String("name") ? Qt::AscendingOrder : Qt::DescendingOrder);
    emit sortKeyChanged();
}

bool WallpaperFilterModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if (m_sortKey != QLatin1String("name")) {
        const auto a = left.data(sortRole()).toULongLong();
        const auto b = right.data(sortRole()).toULongLong();
        if (a != b) return a < b;
    }
    const int name = QString::localeAwareCompare(left.data(WallpaperListModel::FilenameRole).toString(), right.data(WallpaperListModel::FilenameRole).toString());
    return name ? name < 0 : left.data(WallpaperListModel::PathRole).toString() < right.data(WallpaperListModel::PathRole).toString();
}

void WallpaperFilterModel::setFavoritesOnly(bool value)
{
    if (m_favoritesOnly == value) return;
    beginFilterChange();
    m_favoritesOnly = value;
    endFilterChange(QSortFilterProxyModel::Direction::Rows);
    emit favoritesOnlyChanged();
}

void WallpaperFilterModel::setSearchText(const QString &text)
{
    if (m_searchText == text)
        return;
    beginFilterChange();
    m_searchText = text;
    endFilterChange(QSortFilterProxyModel::Direction::Rows);
    emit searchTextChanged();
}

void WallpaperFilterModel::setLockFilter(int filter)
{
    if (m_lockFilter == filter)
        return;
    beginFilterChange();
    m_lockFilter = filter;
    endFilterChange(QSortFilterProxyModel::Direction::Rows);
    emit lockFilterChanged();
}

bool WallpaperFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    auto srcModel = sourceModel();
    if (!srcModel)
        return true;

    auto idx = srcModel->index(sourceRow, 0, sourceParent);

    if (m_favoritesOnly && !idx.data(WallpaperListModel::FavoriteRole).toBool()) return false;

    // 搜索过滤（文件名）
    if (!m_searchText.isEmpty()) {
        auto filename = idx.data(WallpaperListModel::FilenameRole).toString();
        if (!filename.contains(m_searchText, Qt::CaseInsensitive) && !idx.data(WallpaperListModel::PathRole).toString().contains(m_searchText, Qt::CaseInsensitive))
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
