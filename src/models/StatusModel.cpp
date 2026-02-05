#include "StatusModel.h"
#include "../LianwallClient.h"

#include <QFileInfo>

StatusModel::StatusModel(QObject *parent)
    : QObject(parent)
    , m_client(nullptr)
    , m_totalWallpapers(0)
    , m_lockedCount(0)
    , m_availableCount(0)
    , m_scannedCount(0)
    , m_vramUsedMb(0)
    , m_vramTotalMb(0)
    , m_uptimeSecs(0)
    , m_timePointsCount(0)
    , m_connected(false)
{
}

void StatusModel::setClient(LianwallClient *client) {
    if (m_client) {
        disconnect(m_client, nullptr, this, nullptr);
    }
    
    m_client = client;
    
    if (m_client) {
        connect(m_client, &LianwallClient::statusReceived, this, &StatusModel::onStatusReceived);
        connect(m_client, &LianwallClient::connectedChanged, this, &StatusModel::onConnectedChanged);
        
        m_connected = m_client->isConnected();
        emit connectedChanged();
    }
}

void StatusModel::refresh() {
    if (m_client && m_client->isConnected()) {
        m_client->getStatus();
    }
}

void StatusModel::onStatusReceived(const QJsonObject &status) {
    m_mode = status["mode"].toString();
    m_current = status["current"].toString();
    m_engine = status["engine"].toString();
    m_totalWallpapers = status["total_wallpapers"].toInt();
    m_lockedCount = status["locked_count"].toInt();
    m_availableCount = status["available_count"].toInt();
    m_scannedCount = status["scanned_count"].toInt();
    m_vramUsedMb = status["vram_used_mb"].toInt();
    m_vramTotalMb = status["vram_total_mb"].toInt();
    m_uptimeSecs = status["uptime_secs"].toInt();
    m_nextTimePoint = status["next_time_point"].toString();
    m_timePointsCount = status["time_points_count"].toInt();
    
    emit statusChanged();
}

void StatusModel::onConnectedChanged() {
    if (m_client) {
        m_connected = m_client->isConnected();
        emit connectedChanged();
    }
}

QString StatusModel::currentFilename() const {
    if (m_current.isEmpty()) {
        return QString();
    }
    return QFileInfo(m_current).fileName();
}

QString StatusModel::uptimeFormatted() const {
    int hours = m_uptimeSecs / 3600;
    int minutes = (m_uptimeSecs % 3600) / 60;
    int seconds = m_uptimeSecs % 60;
    
    if (hours > 0) {
        return QString("%1h %2m %3s").arg(hours).arg(minutes).arg(seconds);
    } else if (minutes > 0) {
        return QString("%1m %2s").arg(minutes).arg(seconds);
    } else {
        return QString("%1s").arg(seconds);
    }
}
