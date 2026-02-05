#include "TimelineModel.h"
#include "../LianwallClient.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QDebug>

TimelineModel::TimelineModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_client(nullptr)
    , m_currentMode("video")
    , m_scannedCount(0)
    , m_activeCount(0)
{
}

void TimelineModel::setClient(LianwallClient *client) {
    if (m_client) {
        disconnect(m_client, nullptr, this, nullptr);
    }
    
    m_client = client;
    
    if (m_client) {
        connect(m_client, &LianwallClient::timeInfoReceived, this, &TimelineModel::onTimeInfoReceived);
    }
}

void TimelineModel::refresh() {
    if (m_client && m_client->isConnected()) {
        m_client->getTimeInfo();
    }
}

void TimelineModel::setCurrentMode(const QString &mode) {
    if (m_currentMode != mode) {
        m_currentMode = mode;
        emit modeChanged();
        refresh();
    }
}

void TimelineModel::onTimeInfoReceived(const QJsonObject &timeInfo) {
    beginResetModel();
    
    m_currentTime = timeInfo["current_time"].toString();
    
    // 根据当前模式选择数据
    QString scheduleKey = (m_currentMode == "video") ? "video_schedule" : "image_schedule";
    QJsonObject schedule = timeInfo[scheduleKey].toObject();
    
    m_scannedCount = schedule["scanned_count"].toInt();
    m_activeCount = schedule["active_count"].toInt();
    m_nextTimePoint = schedule["next_time_point"].toString();
    
    // 时间点列表
    m_timePoints.clear();
    QJsonArray points = schedule["time_points"].toArray();
    for (const QJsonValue &val : points) {
        m_timePoints.append(val.toString());
    }
    
    // 壁纸时间段
    m_segments.clear();
    QJsonArray segments = schedule["wallpaper_segments"].toArray();
    for (const QJsonValue &val : segments) {
        QJsonObject obj = val.toObject();
        
        WallpaperSegment segment;
        segment.filename = obj["filename"].toString();
        segment.path = obj["path"].toString();
        segment.allDay = obj["all_day"].toBool();
        segment.activeRanges = obj["active_ranges"].toArray();
        
        m_segments.append(segment);
    }
    
    endResetModel();
    emit dataChanged();
}

int TimelineModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_segments.size();
}

QVariant TimelineModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_segments.size()) {
        return QVariant();
    }
    
    const WallpaperSegment &segment = m_segments.at(index.row());
    
    switch (role) {
        case FilenameRole: return segment.filename;
        case PathRole: return segment.path;
        case AllDayRole: return segment.allDay;
        case ActiveRangesRole: return segment.activeRanges.toVariantList();
        case ThumbnailRole: return segment.thumbnail;
        default: return QVariant();
    }
}

QHash<int, QByteArray> TimelineModel::roleNames() const {
    return {
        {FilenameRole, "filename"},
        {PathRole, "path"},
        {AllDayRole, "allDay"},
        {ActiveRangesRole, "activeRanges"},
        {ThumbnailRole, "thumbnail"}
    };
}

// ============================================================================
// 时间轴编辑操作
// ============================================================================

bool TimelineModel::createTimeDir(const QString &baseDir, const QString &start, const QString &end) {
    // 构建目录名: HHMM-HHMM 或 HH-HH
    QString dirName;
    if (start.length() == 5 && end.length() == 5) {
        // "HH:MM" -> "HHMM"
        dirName = start.left(2) + start.mid(3, 2) + "-" + end.left(2) + end.mid(3, 2);
    } else {
        dirName = start + "-" + end;
    }
    
    QString fullPath = baseDir + "/" + dirName;
    
    QDir dir;
    if (dir.mkpath(fullPath)) {
        qDebug() << "[TimelineModel] Created directory:" << fullPath;
        emit operationCompleted(true, tr("Created: %1").arg(dirName));
        return true;
    } else {
        emit operationCompleted(false, tr("Failed to create directory"));
        return false;
    }
}

bool TimelineModel::moveWallpaper(const QString &source, const QString &destDir, const QString &operation) {
    QString filename = QFileInfo(source).fileName();
    QString dest = destDir + "/" + filename;
    
    bool success = false;
    QString message;
    
    if (operation == "link") {
        // 硬链接
        QProcess ln;
        ln.start("ln", {source, dest});
        ln.waitForFinished(5000);
        success = (ln.exitCode() == 0);
        message = success ? tr("Linked: %1").arg(filename) : tr("Failed to link (try copy instead)");
    } else if (operation == "copy") {
        // 复制
        success = QFile::copy(source, dest);
        message = success ? tr("Copied: %1").arg(filename) : tr("Failed to copy");
    } else if (operation == "move") {
        // 移动
        success = QFile::rename(source, dest);
        message = success ? tr("Moved: %1").arg(filename) : tr("Failed to move");
    }
    
    emit operationCompleted(success, message);
    
    if (success) {
        // 刷新数据
        refresh();
    }
    
    return success;
}
