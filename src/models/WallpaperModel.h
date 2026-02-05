#pragma once

#include <QAbstractListModel>
#include <QJsonArray>
#include <QJsonObject>

class LianwallClient;

/**
 * @brief 壁纸列表模型
 * 
 * 为 QML 提供壁纸数据，支持筛选。
 */
class WallpaperModel : public QAbstractListModel {
    Q_OBJECT
    
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString modeFilter READ modeFilter WRITE setModeFilter NOTIFY filterChanged)
    Q_PROPERTY(QString statusFilter READ statusFilter WRITE setStatusFilter NOTIFY filterChanged)
    Q_PROPERTY(bool activeOnlyFilter READ activeOnlyFilter WRITE setActiveOnlyFilter NOTIFY filterChanged)

public:
    enum Roles {
        IndexRole = Qt::UserRole + 1,
        FilenameRole,
        PathRole,
        AngleRole,
        LockedRole,
        InCooldownRole,
        ThumbnailRole,
        IsCurrentRole
    };

    explicit WallpaperModel(QObject *parent = nullptr);

    Q_INVOKABLE void setClient(LianwallClient *client);

    // QAbstractListModel
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Properties
    int count() const { return m_filteredItems.size(); }
    QString modeFilter() const { return m_modeFilter; }
    QString statusFilter() const { return m_statusFilter; }
    bool activeOnlyFilter() const { return m_activeOnlyFilter; }

    void setModeFilter(const QString &filter);
    void setStatusFilter(const QString &filter);
    void setActiveOnlyFilter(bool active);

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void setThumbnailProvider(QObject *provider);

signals:
    void countChanged();
    void filterChanged();

private slots:
    void onSpaceReceived(const QJsonObject &space);

private:
    void applyFilters();

    struct WallpaperItem {
        int index;
        QString filename;
        QString path;
        double angle;
        bool locked;
        bool inCooldown;
        QString thumbnail;
    };

    LianwallClient *m_client;
    QObject *m_thumbnailProvider;
    
    QList<WallpaperItem> m_allItems;
    QList<WallpaperItem> m_filteredItems;
    int m_currentIndex;
    
    QString m_modeFilter;      // "all", "video", "image"
    QString m_statusFilter;    // "all", "locked", "unlocked", "cooldown"
    bool m_activeOnlyFilter;
};
