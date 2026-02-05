#pragma once

#include <QObject>
#include <QString>
#include <QHash>

/**
 * @brief 缩略图缓存管理器
 * 
 * 为视频壁纸生成和缓存缩略图。
 * 使用 ffmpeg 提取第1秒帧，存储在 ~/.cache/lianwall/thumbnails/
 */
class ThumbnailCache : public QObject {
    Q_OBJECT

public:
    explicit ThumbnailCache(QObject *parent = nullptr);

    /// 获取缩略图路径（如果不存在则生成）
    Q_INVOKABLE QString getThumbnail(const QString &videoPath);

    /// 异步生成缩略图
    Q_INVOKABLE void generateThumbnailAsync(const QString &videoPath);

    /// 检查缩略图是否存在
    Q_INVOKABLE bool hasThumbnail(const QString &videoPath);

    /// 清理无效缩略图（不再关联到任何壁纸）
    Q_INVOKABLE void cleanup(const QStringList &validPaths);

    /// 获取缓存目录大小（字节）
    Q_INVOKABLE qint64 cacheSize();

    /// 清空所有缩略图
    Q_INVOKABLE void clearAll();

signals:
    void thumbnailReady(const QString &videoPath, const QString &thumbnailPath);
    void thumbnailFailed(const QString &videoPath, const QString &error);

private:
    QString hashPath(const QString &path);
    QString thumbnailPath(const QString &videoPath);
    bool generateThumbnail(const QString &videoPath, const QString &outputPath);
    void ensureCacheDir();

    QHash<QString, QString> m_cache;  // videoPath -> thumbnailPath
};
