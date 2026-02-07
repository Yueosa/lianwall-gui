#include "WallpaperListModel.h"
#include "DaemonClient.h"

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

WallpaperListModel::WallpaperListModel(DaemonClient *client, QObject *parent)
    : QAbstractListModel(parent)
    , m_client(client)
{
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
    if (m_loading)
        return;

    m_loading = true;
    emit loadingChanged();

    // 确定查询模式
    std::optional<Daemon::WallMode> queryMode;
    if (!mode.isEmpty()) {
        queryMode = Daemon::wallModeFromString(mode);
    }

    qDebug() << "[WallpaperListModel] Loading space"
             << (queryMode ? Daemon::wallModeToString(*queryMode) : "(current mode)");

    m_client->getSpace(queryMode, [this](const Daemon::Response &resp) {
        if (resp.type == Daemon::ResponseType::Space) {
            auto space = resp.asSpace();

            beginResetModel();
            m_items = space.items;
            endResetModel();

            m_mode = Daemon::wallModeToString(space.mode);
            emit modeChanged();
            emit countChanged();

            qDebug() << "[WallpaperListModel] Loaded" << m_items.size() << "wallpapers in" << m_mode;
        } else if (resp.type == Daemon::ResponseType::Error) {
            auto err = resp.asError();
            qWarning() << "[WallpaperListModel] Error:" << err.message;
            emit errorOccurred(err.message);
        }

        m_loading = false;
        emit loadingChanged();
    });
}

void WallpaperListModel::toggleLock(int row)
{
    if (row < 0 || row >= m_items.size())
        return;

    const auto &item = m_items.at(row);
    qDebug() << "[WallpaperListModel] ToggleLock:" << item.filename;

    m_client->toggleLock(item.path, [this, row](const Daemon::Response &resp) {
        if (resp.type == Daemon::ResponseType::Ok) {
            // 立即本地更新（daemon 也会推送 SpaceUpdated 事件，但本地先更新提升响应感）
            if (row < m_items.size()) {
                m_items[row].locked = !m_items[row].locked;
                auto idx = index(row);
                emit dataChanged(idx, idx, { LockedRole });
            }
        } else if (resp.type == Daemon::ResponseType::Error) {
            auto err = resp.asError();
            qWarning() << "[WallpaperListModel] ToggleLock error:" << err.message;
            emit errorOccurred(err.message);
        }
    });
}

void WallpaperListModel::setAsCurrent(int row)
{
    if (row < 0 || row >= m_items.size())
        return;

    const auto &item = m_items.at(row);
    qDebug() << "[WallpaperListModel] SetWallpaper:" << item.path;

    m_client->setWallpaper(item.path, [this](const Daemon::Response &resp) {
        if (resp.type == Daemon::ResponseType::Error) {
            auto err = resp.asError();
            qWarning() << "[WallpaperListModel] SetWallpaper error:" << err.message;
            emit errorOccurred(err.message);
        }
        // isCurrent 状态将通过 WallpaperChanged 事件 → DaemonState 更新
        // 下次 load() 时会反映新的 is_current 标记
    });
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
    invalidateFilter();
}

void WallpaperFilterModel::setLockFilter(int filter)
{
    if (m_lockFilter == filter)
        return;
    m_lockFilter = filter;
    emit lockFilterChanged();
    invalidateFilter();
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
