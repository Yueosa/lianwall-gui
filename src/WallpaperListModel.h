#pragma once

/// @file WallpaperListModel.h
/// @brief 壁纸列表模型（QML 数据源）
///
/// 继承 QAbstractListModel，数据来自 GetSpace API。
/// 支持搜索过滤、锁定状态过滤，以及 lock/unlock 操作。
/// 懒加载：仅在 LibraryPage 激活时请求 GetSpace。

#include "DaemonTypes.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QString>
#include <QVector>

class DaemonClient;

// ============================================================================
// WallpaperListModel — 原始数据模型
// ============================================================================

class WallpaperListModel : public QAbstractListModel
{
    Q_OBJECT

    /// 当前查询的模式
    Q_PROPERTY(QString mode READ mode NOTIFY modeChanged)

    /// 数据是否正在加载
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)

    /// 壁纸总数
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        PathRole = Qt::UserRole + 1,
        FilenameRole,
        AngleRole,
        LockedRole,
        InCooldownRole,
        IsCurrentRole,
        IndexRole,
        IsVideoRole,          // 根据文件扩展名判断
    };

    explicit WallpaperListModel(DaemonClient *client, QObject *parent = nullptr);

    // --- QAbstractListModel 接口 ---
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // --- 属性 ---
    QString mode() const { return m_mode; }
    bool loading() const { return m_loading; }
    int count() const { return m_items.size(); }

    // --- QML 可调用 ---

    /// 请求加载指定模式（空字符串 = 当前模式）
    Q_INVOKABLE void load(const QString &mode = QString());

    /// 切换壁纸锁定状态
    Q_INVOKABLE void toggleLock(int row);

    /// 设为当前壁纸
    Q_INVOKABLE void setAsCurrent(int row);

    /// 获取指定行的路径
    Q_INVOKABLE QString pathAt(int row) const;

    /// 获取指定行的文件名
    Q_INVOKABLE QString filenameAt(int row) const;

    /// 获取指定行是否锁定
    Q_INVOKABLE bool lockedAt(int row) const;

    /// 获取指定行是否为当前壁纸
    Q_INVOKABLE bool isCurrentAt(int row) const;

    /// 判断文件是否为视频
    static bool isVideoFile(const QString &filename);

signals:
    void modeChanged();
    void loadingChanged();
    void countChanged();

    /// 操作错误
    void errorOccurred(const QString &message);

private:
    DaemonClient *m_client;
    QVector<Daemon::WallpaperPoint> m_items;
    QString m_mode;
    bool m_loading = false;
};

// ============================================================================
// WallpaperFilterModel — 搜索 + 锁定状态过滤代理
// ============================================================================

class WallpaperFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

    /// 搜索关键词（按文件名过滤）
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)

    /// 锁定状态过滤：-1=全部, 0=未锁定, 1=已锁定
    Q_PROPERTY(int lockFilter READ lockFilter WRITE setLockFilter NOTIFY lockFilterChanged)

    /// 过滤后的数量
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    explicit WallpaperFilterModel(QObject *parent = nullptr);

    QString searchText() const { return m_searchText; }
    void setSearchText(const QString &text);

    int lockFilter() const { return m_lockFilter; }
    void setLockFilter(int filter);

    int count() const { return rowCount(); }

    // 透传源模型的 Q_INVOKABLE 方法（将 proxy row 映射到 source row）
    Q_INVOKABLE void toggleLock(int proxyRow);
    Q_INVOKABLE void setAsCurrent(int proxyRow);
    Q_INVOKABLE QString pathAt(int proxyRow) const;
    Q_INVOKABLE QString filenameAt(int proxyRow) const;
    Q_INVOKABLE bool lockedAt(int proxyRow) const;
    Q_INVOKABLE bool isCurrentAt(int proxyRow) const;
    Q_INVOKABLE bool isVideoAt(int proxyRow) const;

signals:
    void searchTextChanged();
    void lockFilterChanged();
    void countChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QString m_searchText;
    int m_lockFilter = -1;   // -1=全部
};
