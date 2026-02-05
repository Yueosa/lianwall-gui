#pragma once

#include <QAbstractListModel>
#include <QJsonObject>
#include <QJsonArray>

class LianwallClient;

/**
 * @brief 时间轴数据模型
 * 
 * 为时间轴编辑器提供数据，支持壁纸时间段的可视化和编辑。
 */
class TimelineModel : public QAbstractListModel {
    Q_OBJECT
    
    Q_PROPERTY(QString currentTime READ currentTime NOTIFY dataChanged)
    Q_PROPERTY(QString currentMode READ currentMode WRITE setCurrentMode NOTIFY modeChanged)
    Q_PROPERTY(QStringList timePoints READ timePoints NOTIFY dataChanged)
    Q_PROPERTY(QString nextTimePoint READ nextTimePoint NOTIFY dataChanged)
    Q_PROPERTY(int scannedCount READ scannedCount NOTIFY dataChanged)
    Q_PROPERTY(int activeCount READ activeCount NOTIFY dataChanged)

public:
    enum Roles {
        FilenameRole = Qt::UserRole + 1,
        PathRole,
        AllDayRole,
        ActiveRangesRole,
        ThumbnailRole
    };

    explicit TimelineModel(QObject *parent = nullptr);

    Q_INVOKABLE void setClient(LianwallClient *client);

    // QAbstractListModel
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Properties
    QString currentTime() const { return m_currentTime; }
    QString currentMode() const { return m_currentMode; }
    QStringList timePoints() const { return m_timePoints; }
    QString nextTimePoint() const { return m_nextTimePoint; }
    int scannedCount() const { return m_scannedCount; }
    int activeCount() const { return m_activeCount; }

    void setCurrentMode(const QString &mode);

    Q_INVOKABLE void refresh();

    // 时间轴编辑操作
    Q_INVOKABLE bool createTimeDir(const QString &baseDir, const QString &start, const QString &end);
    Q_INVOKABLE bool moveWallpaper(const QString &source, const QString &destDir, const QString &operation);

signals:
    void dataChanged();
    void modeChanged();
    void operationCompleted(bool success, const QString &message);

private slots:
    void onTimeInfoReceived(const QJsonObject &timeInfo);

private:
    struct WallpaperSegment {
        QString filename;
        QString path;
        bool allDay;
        QJsonArray activeRanges;
        QString thumbnail;
    };

    LianwallClient *m_client;
    
    QString m_currentTime;
    QString m_currentMode;
    QStringList m_timePoints;
    QString m_nextTimePoint;
    int m_scannedCount;
    int m_activeCount;
    
    QList<WallpaperSegment> m_segments;
};
