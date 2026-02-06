#pragma once

/// @file ThumbnailProvider.h
/// @brief 异步缩略图提供器
///
/// 继承 QQuickAsyncImageProvider，支持：
/// - 图片：QImage 异步缩放
/// - 视频：ffmpeg 截取第 5 秒静帧
/// - 磁盘缓存：~/.cache/lianwall/thumbnails/
/// - 并发限制：≤2 个 ffmpeg 进程
///
/// QML 使用：Image { source: "image://thumbnail/" + encodeURIComponent(path) }

#include <QQuickAsyncImageProvider>
#include <QThreadPool>
#include <QRunnable>
#include <QSemaphore>
#include <QImage>
#include <QSize>
#include <QString>
#include <QDir>

// ============================================================================
// ThumbnailResponse — 单个缩略图异步响应
// ============================================================================

class ThumbnailResponse : public QQuickImageResponse, public QRunnable
{
public:
    ThumbnailResponse(const QString &path, const QSize &requestedSize,
                      QThreadPool *pool, QSemaphore *ffmpegSemaphore);

    QQuickTextureFactory *textureFactory() const override;
    QString errorString() const override;
    void run() override;

private:
    /// 加载图片缩略图
    QImage loadImage() const;

    /// 通过 ffmpeg 截取视频帧
    QImage loadVideoFrame() const;

    /// 获取缓存路径
    QString cachePath() const;

    /// 判断是否为视频文件
    bool isVideo() const;

    /// 计算缓存文件名（基于路径哈希 + 尺寸）
    static QString cacheFilename(const QString &path, const QSize &size);

    /// 确保缓存目录存在
    static QString cacheDir();

    QString m_path;
    QSize m_requestedSize;
    QImage m_image;
    QString m_errorString;
    QSemaphore *m_ffmpegSemaphore;   // 限制并发 ffmpeg 进程数
};

// ============================================================================
// ThumbnailProvider — 图片提供器（注册到 QML 引擎）
// ============================================================================

class ThumbnailProvider : public QQuickAsyncImageProvider
{
public:
    ThumbnailProvider();
    ~ThumbnailProvider() override;

    QQuickImageResponse *requestImageResponse(
        const QString &id, const QSize &requestedSize) override;

private:
    QThreadPool m_pool;
    QSemaphore m_ffmpegSemaphore;   // 限制 ≤2 个并发 ffmpeg
};
