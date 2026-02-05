#include "WallpaperModel.h"
#include "../LianwallClient.h"
#include "../ThumbnailCache.h"

#include <QFileInfo>

WallpaperModel::WallpaperModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_client(nullptr)
    , m_thumbnailProvider(nullptr)
    , m_currentIndex(-1)
    , m_modeFilter("all")
    , m_statusFilter("all")
    , m_activeOnlyFilter(false)
{
}

void WallpaperModel::setClient(LianwallClient *client) {
    if (m_client) {
        disconnect(m_client, nullptr, this, nullptr);
    }
    
    m_client = client;
    
    if (m_client) {
        connect(m_client, &LianwallClient::spaceReceived, this, &WallpaperModel::onSpaceReceived);
        
        // 连接后自动刷新一次
        if (m_client->isConnected()) {
            m_client->getSpace();
        }
    }
}

void WallpaperModel::setThumbnailProvider(QObject *provider) {
    m_thumbnailProvider = provider;
}

void WallpaperModel::refresh() {
    if (m_client && m_client->isConnected()) {
        m_client->getSpace();
    }
}

void WallpaperModel::onSpaceReceived(const QJsonObject &space) {
    beginResetModel();
    
    m_allItems.clear();
    m_currentIndex = space["current_index"].toInt(-1);
    
    QJsonArray items = space["items"].toArray();
    for (const QJsonValue &val : items) {
        QJsonObject obj = val.toObject();
        
        WallpaperItem item;
        item.index = obj["index"].toInt();
        item.filename = obj["filename"].toString();
        item.path = obj["path"].toString();
        item.angle = obj["angle"].toDouble();
        item.locked = obj["locked"].toBool();
        item.inCooldown = obj["in_cooldown"].toBool();
        
        // 获取缩略图
        if (m_thumbnailProvider) {
            ThumbnailCache *cache = qobject_cast<ThumbnailCache*>(m_thumbnailProvider);
            if (cache) {
                item.thumbnail = cache->getThumbnail(item.path);
            }
        }
        
        m_allItems.append(item);
    }
    
    applyFilters();
    endResetModel();
    
    emit countChanged();
}

void WallpaperModel::applyFilters() {
    m_filteredItems.clear();
    
    for (const WallpaperItem &item : m_allItems) {
        // Mode filter
        if (m_modeFilter != "all") {
            QString ext = QFileInfo(item.filename).suffix().toLower();
            bool isVideo = (ext == "mp4" || ext == "mkv" || ext == "webm" || ext == "avi");
            
            if (m_modeFilter == "video" && !isVideo) continue;
            if (m_modeFilter == "image" && isVideo) continue;
        }
        
        // Status filter
        if (m_statusFilter == "locked" && !item.locked) continue;
        if (m_statusFilter == "unlocked" && item.locked) continue;
        if (m_statusFilter == "cooldown" && !item.inCooldown) continue;
        
        m_filteredItems.append(item);
    }
}

void WallpaperModel::setModeFilter(const QString &filter) {
    if (m_modeFilter != filter) {
        m_modeFilter = filter;
        beginResetModel();
        applyFilters();
        endResetModel();
        emit filterChanged();
        emit countChanged();
    }
}

void WallpaperModel::setStatusFilter(const QString &filter) {
    if (m_statusFilter != filter) {
        m_statusFilter = filter;
        beginResetModel();
        applyFilters();
        endResetModel();
        emit filterChanged();
        emit countChanged();
    }
}

void WallpaperModel::setActiveOnlyFilter(bool active) {
    if (m_activeOnlyFilter != active) {
        m_activeOnlyFilter = active;
        beginResetModel();
        applyFilters();
        endResetModel();
        emit filterChanged();
        emit countChanged();
    }
}

int WallpaperModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_filteredItems.size();
}

QVariant WallpaperModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_filteredItems.size()) {
        return QVariant();
    }
    
    const WallpaperItem &item = m_filteredItems.at(index.row());
    
    switch (role) {
        case IndexRole: return item.index;
        case FilenameRole: return item.filename;
        case PathRole: return item.path;
        case AngleRole: return item.angle;
        case LockedRole: return item.locked;
        case InCooldownRole: return item.inCooldown;
        case ThumbnailRole: return item.thumbnail;
        case IsCurrentRole: return item.index == m_currentIndex;
        default: return QVariant();
    }
}

QHash<int, QByteArray> WallpaperModel::roleNames() const {
    return {
        {IndexRole, "index"},
        {FilenameRole, "filename"},
        {PathRole, "path"},
        {AngleRole, "angle"},
        {LockedRole, "locked"},
        {InCooldownRole, "inCooldown"},
        {ThumbnailRole, "thumbnail"},
        {IsCurrentRole, "isCurrent"}
    };
}
