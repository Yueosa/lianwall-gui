#pragma once

/// @file ThumbnailProvider.h
/// @brief 异步缩略图提供器（磁盘缓存 + 内存 LRU）
///
/// - 图片：QImage 异步缩放
/// - 视频：ffmpeg 截帧（优先磁盘缓存）
/// - 磁盘缓存：~/.cache/lianwall/thumbnails/
/// - 内存 LRU：按字节预算淘汰最旧条目，避免滑出视口即重载
/// - 尺寸归档：请求尺寸对齐到标准档位，减少缓存碎片

#include <QQuickAsyncImageProvider>
#include <QThreadPool>
#include <QRunnable>
#include <QSemaphore>
#include <QMutex>
#include <QHash>
#include <QList>
#include <QImage>
#include <QSize>
#include <QString>
#include <QAtomicInt>
#include <memory>
#include <atomic>
#include <QElapsedTimer>

// ============================================================================
// ThumbnailMemoryCache — 线程安全的字节预算 LRU
// ============================================================================

class ThumbnailMemoryCache
{
public:
    explicit ThumbnailMemoryCache(qint64 budgetBytes = 96LL * 1024 * 1024);

    QImage get(const QString &key);
    void put(const QString &key, const QImage &image);
    void clear();
    qint64 bytesUsed() const;
    qint64 budget() const;
    void setBudget(qint64 bytes);

private:
    struct Entry {
        QImage image;
        qint64 bytes = 0;
    };

    static qint64 imageBytes(const QImage &img);
    void touchLocked(const QString &key);
    void evictLocked(qint64 needBytes);

    mutable QMutex m_mutex;
    QHash<QString, Entry> m_entries;
    QList<QString> m_lru;   // front = oldest
    qint64 m_bytes = 0;
    qint64 m_budget = 0;
};

/// Shared worker lifetime, per-key deduplication and a soft disk-cache budget.
class ThumbnailWorkState
{
public:
    explicit ThumbnailWorkState(qint64 diskBudget = 512LL * 1024 * 1024, int maxEntries = 4096);
    std::shared_ptr<QMutex> mutexFor(const QString &key);
    void maintainDiskCache(const QString &directory, bool force = false);
    std::atomic_bool stopping{false};
    std::atomic_uint64_t generationAttempts{0};
private:
    QMutex m_mutex;
    QHash<QString, std::weak_ptr<QMutex>> m_keys;
    QMutex m_maintenanceMutex;
    QElapsedTimer m_lastMaintenance;
    quint64 m_writes = 0;
    qint64 m_diskBudget;
    int m_maxEntries;
};

// ============================================================================
// ThumbnailResponse — 单个缩略图异步响应
// ============================================================================

class ThumbnailResponse : public QQuickImageResponse, public QRunnable
{
public:
    ThumbnailResponse(const QString &path, const QSize &requestedSize,
                      QThreadPool *pool, QSemaphore *ffmpegSemaphore,
                      std::shared_ptr<ThumbnailMemoryCache> memoryCache,
                      std::shared_ptr<ThumbnailWorkState> workState = std::make_shared<ThumbnailWorkState>());

    QQuickTextureFactory *textureFactory() const override;
    QString errorString() const override;
    void cancel() override;
    void run() override;

private:
    QImage loadImage() const;
    QImage loadVideoFrame() const;
    bool cancelled() const;
    bool isVideo() const;

    static QSize snapSize(const QSize &requested);
    static QString cacheFilename(const QString &path, const QSize &size);
    static QString cacheDir();

    QString m_path;
    QSize m_requestedSize;
    QImage m_image;
    QString m_errorString;
    QSemaphore *m_ffmpegSemaphore;
    std::shared_ptr<ThumbnailMemoryCache> m_memoryCache;
    std::shared_ptr<ThumbnailWorkState> m_workState;
    QAtomicInt m_cancelled {0};
};

// ============================================================================
// ThumbnailProvider
// ============================================================================

class ThumbnailProvider : public QQuickAsyncImageProvider
{
public:
    ThumbnailProvider();
    ~ThumbnailProvider() override;

    QQuickImageResponse *requestImageResponse(
        const QString &id, const QSize &requestedSize) override;

    /// 内存预算（字节），默认 96 MiB
    void setMemoryBudget(qint64 bytes);
    qint64 memoryBudget() const;
    qint64 memoryBytesUsed() const;

private:
    QThreadPool m_pool;
    QSemaphore m_ffmpegSemaphore;
    std::shared_ptr<ThumbnailMemoryCache> m_memoryCache;
    std::shared_ptr<ThumbnailWorkState> m_workState;
};
