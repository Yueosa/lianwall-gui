#include "ThumbnailProvider.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QProcess>
#include <QStandardPaths>
#include <QDebug>
#include <QUrl>

// ============================================================================
// 常量
// ============================================================================

static const QStringList kVideoExtensions = {
    "mp4", "mkv", "webm", "avi", "mov", "flv", "wmv", "m4v", "3gp", "ogv", "ts", "m2ts",
};

static constexpr int kDefaultThumbSize = 320;   // 默认缩略图尺寸
static constexpr int kFfmpegTimeoutMs = 10000;  // ffmpeg 超时 10s

// ============================================================================
// ThumbnailResponse
// ============================================================================

ThumbnailResponse::ThumbnailResponse(const QString &path, const QSize &requestedSize,
                                     QThreadPool *pool, QSemaphore *ffmpegSemaphore)
    : m_path(QUrl::fromPercentEncoding(path.toUtf8()))
    , m_requestedSize(requestedSize.isValid() ? requestedSize : QSize(kDefaultThumbSize, kDefaultThumbSize))
    , m_ffmpegSemaphore(ffmpegSemaphore)
{
    setAutoDelete(false);
    pool->start(this);
}

QQuickTextureFactory *ThumbnailResponse::textureFactory() const
{
    return m_image.isNull() ? nullptr : QQuickTextureFactory::textureFactoryForImage(m_image);
}

QString ThumbnailResponse::errorString() const
{
    return m_errorString;
}

void ThumbnailResponse::run()
{
    if (m_path.isEmpty()) {
        m_errorString = QStringLiteral("Empty path");
        emit finished();
        return;
    }

    // 检查缓存
    QString cached = cachePath();
    if (QFileInfo::exists(cached)) {
        m_image = QImage(cached);
        if (!m_image.isNull()) {
            emit finished();
            return;
        }
    }

    // 生成缩略图
    if (isVideo()) {
        m_image = loadVideoFrame();
    } else {
        m_image = loadImage();
    }

    if (m_image.isNull()) {
        if (m_errorString.isEmpty())
            m_errorString = QStringLiteral("Failed to generate thumbnail for: ") + m_path;
    } else {
        // 保存到缓存
        QDir().mkpath(cacheDir());
        m_image.save(cached, "JPEG", 85);
    }

    emit finished();
}

QImage ThumbnailResponse::loadImage() const
{
    QImageReader reader(m_path);
    reader.setAutoTransform(true);

    // 设置缩放尺寸以节省内存
    QSize origSize = reader.size();
    if (origSize.isValid() && (origSize.width() > m_requestedSize.width()
                               || origSize.height() > m_requestedSize.height())) {
        QSize scaled = origSize.scaled(m_requestedSize, Qt::KeepAspectRatio);
        reader.setScaledSize(scaled);
    }

    QImage img = reader.read();
    if (img.isNull()) {
        return {};
    }

    // 确保不超过请求尺寸
    if (img.width() > m_requestedSize.width() || img.height() > m_requestedSize.height()) {
        img = img.scaled(m_requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    return img;
}

QImage ThumbnailResponse::loadVideoFrame() const
{
    // 使用信号量限制并发 ffmpeg 数量
    if (!m_ffmpegSemaphore->tryAcquire(1, kFfmpegTimeoutMs)) {
        qWarning() << "[ThumbnailProvider] Timeout waiting for ffmpeg slot:" << m_path;
        return {};
    }

    QImage result;

    QProcess ffmpeg;
    ffmpeg.setProcessChannelMode(QProcess::ForwardedErrorChannel);

    QStringList args = {
        "-ss", "5",                    // 跳到第 5 秒
        "-i", m_path,                  // 输入文件
        "-vframes", "1",               // 只截 1 帧
        "-vf", QStringLiteral("scale=%1:%2:force_original_aspect_ratio=decrease")
                   .arg(m_requestedSize.width())
                   .arg(m_requestedSize.height()),
        "-f", "image2pipe",            // 输出到 pipe
        "-vcodec", "png",              // PNG 格式（无损，便于 QImage 读取）
        "-"                            // 输出到 stdout
    };

    ffmpeg.start(QStringLiteral("ffmpeg"), args);

    if (!ffmpeg.waitForStarted(5000)) {
        qWarning() << "[ThumbnailProvider] ffmpeg failed to start";
        m_ffmpegSemaphore->release();
        return {};
    }

    if (!ffmpeg.waitForFinished(kFfmpegTimeoutMs)) {
        qWarning() << "[ThumbnailProvider] ffmpeg timeout for:" << m_path;
        ffmpeg.kill();
        ffmpeg.waitForFinished(2000);
        m_ffmpegSemaphore->release();
        return {};
    }

    m_ffmpegSemaphore->release();

    if (ffmpeg.exitCode() != 0) {
        // ffmpeg -ss 5 可能失败（视频不够长），尝试 -ss 0
        QProcess retry;
        retry.setProcessChannelMode(QProcess::ForwardedErrorChannel);
        args[1] = "0";   // -ss 0
        retry.start(QStringLiteral("ffmpeg"), args);

        if (!retry.waitForStarted(5000) || !retry.waitForFinished(kFfmpegTimeoutMs)) {
            qWarning() << "[ThumbnailProvider] ffmpeg retry failed for:" << m_path;
            return {};
        }

        if (retry.exitCode() == 0) {
            QByteArray data = retry.readAllStandardOutput();
            result.loadFromData(data, "PNG");
        }
    } else {
        QByteArray data = ffmpeg.readAllStandardOutput();
        result.loadFromData(data, "PNG");
    }

    return result;
}

QString ThumbnailResponse::cachePath() const
{
    return cacheDir() + "/" + cacheFilename(m_path, m_requestedSize);
}

bool ThumbnailResponse::isVideo() const
{
    auto ext = QFileInfo(m_path).suffix().toLower();
    return kVideoExtensions.contains(ext);
}

QString ThumbnailResponse::cacheFilename(const QString &path, const QSize &size)
{
    QByteArray hash = QCryptographicHash::hash(
        path.toUtf8(), QCryptographicHash::Md5).toHex();
    return QStringLiteral("%1_%2x%3.jpg")
        .arg(QString::fromLatin1(hash))
        .arg(size.width())
        .arg(size.height());
}

QString ThumbnailResponse::cacheDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
           + QStringLiteral("/thumbnails");
}

// ============================================================================
// ThumbnailProvider
// ============================================================================

ThumbnailProvider::ThumbnailProvider()
    : m_ffmpegSemaphore(2)   // 最多 2 个并发 ffmpeg
{
    // 缩略图线程池（独立于全局线程池，避免抢占）
    m_pool.setMaxThreadCount(4);
}

ThumbnailProvider::~ThumbnailProvider()
{
    m_pool.waitForDone(5000);
}

QQuickImageResponse *ThumbnailProvider::requestImageResponse(
    const QString &id, const QSize &requestedSize)
{
    return new ThumbnailResponse(id, requestedSize, &m_pool, &m_ffmpegSemaphore);
}
