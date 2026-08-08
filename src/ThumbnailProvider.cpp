#include "ThumbnailProvider.h"
#include "Constants.h"

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

static constexpr int kDefaultThumbW = 320;
static constexpr int kDefaultThumbH = 180;
static constexpr int kFfmpegTimeoutMs = 8000;

// ============================================================================
// ThumbnailMemoryCache
// ============================================================================

ThumbnailMemoryCache::ThumbnailMemoryCache(qint64 budgetBytes)
    : m_budget(qMax<qint64>(8LL * 1024 * 1024, budgetBytes))
{
}

qint64 ThumbnailMemoryCache::imageBytes(const QImage &img)
{
    if (img.isNull())
        return 0;
    // depth() 是 bits-per-pixel
    return static_cast<qint64>(img.width()) * img.height() * qMax(1, img.depth() / 8);
}

qint64 ThumbnailMemoryCache::bytesUsed() const
{
    QMutexLocker lock(&m_mutex);
    return m_bytes;
}

void ThumbnailMemoryCache::setBudget(qint64 bytes)
{
    QMutexLocker lock(&m_mutex);
    m_budget = qMax<qint64>(8LL * 1024 * 1024, bytes);
    evictLocked(0);
}

void ThumbnailMemoryCache::clear()
{
    QMutexLocker lock(&m_mutex);
    m_entries.clear();
    m_lru.clear();
    m_bytes = 0;
}

void ThumbnailMemoryCache::touchLocked(const QString &key)
{
    m_lru.removeOne(key);
    m_lru.append(key);
}

void ThumbnailMemoryCache::evictLocked(qint64 needBytes)
{
    while (!m_lru.isEmpty() && m_bytes + needBytes > m_budget) {
        const QString oldest = m_lru.takeFirst();
        auto it = m_entries.find(oldest);
        if (it == m_entries.end())
            continue;
        m_bytes -= it->bytes;
        m_entries.erase(it);
    }
    if (m_bytes < 0)
        m_bytes = 0;
}

QImage ThumbnailMemoryCache::get(const QString &key)
{
    QMutexLocker lock(&m_mutex);
    auto it = m_entries.find(key);
    if (it == m_entries.end())
        return {};
    touchLocked(key);
    return it->image;
}

void ThumbnailMemoryCache::put(const QString &key, const QImage &image)
{
    if (key.isEmpty() || image.isNull())
        return;

    const qint64 bytes = imageBytes(image);
    QMutexLocker lock(&m_mutex);

    auto it = m_entries.find(key);
    if (it != m_entries.end()) {
        m_bytes -= it->bytes;
        it->image = image;
        it->bytes = bytes;
        m_bytes += bytes;
        touchLocked(key);
    } else {
        evictLocked(bytes);
        // 单图超过预算时仍保留一张（避免完全无法缓存）
        if (bytes > m_budget && !m_lru.isEmpty())
            evictLocked(m_budget); // 清空后只放这一张
        m_entries.insert(key, Entry{image, bytes});
        m_lru.append(key);
        m_bytes += bytes;
    }
}

// ============================================================================
// ThumbnailResponse
// ============================================================================

ThumbnailResponse::ThumbnailResponse(const QString &path, const QSize &requestedSize,
                                     QThreadPool *pool, QSemaphore *ffmpegSemaphore,
                                     std::shared_ptr<ThumbnailMemoryCache> memoryCache)
    : m_path(QUrl::fromPercentEncoding(path.toUtf8()))
    , m_requestedSize(snapSize(requestedSize))
    , m_ffmpegSemaphore(ffmpegSemaphore)
    , m_memoryCache(std::move(memoryCache))
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

void ThumbnailResponse::cancel()
{
    m_cancelled.storeRelaxed(1);
}

QSize ThumbnailResponse::snapSize(const QSize &requested)
{
    // 对齐到标准档位，避免 cell 像素差导致缓存碎片
    using namespace LianwallGui::Thumbnail;
    const QSize req = requested.isValid() ? requested : QSize(kDefaultThumbW, kDefaultThumbH);
    const int longEdge = qMax(req.width(), req.height());

    Quality q = Quality::Tiny;
    if (longEdge > 960)
        q = Quality::High;
    else if (longEdge > 560)
        q = Quality::Medium;
    else if (longEdge > 280)
        q = Quality::Low;
    else
        q = Quality::Tiny;

    auto pair = getSize(q);
    // 保持请求方向（竖图/横图）的近似比例：按请求框 fit 进档位
    return QSize(pair.first, pair.second);
}

QString ThumbnailResponse::memoryKey(const QString &path, const QSize &size)
{
    return cacheFilename(path, size);
}

void ThumbnailResponse::run()
{
    if (m_cancelled.loadRelaxed()) {
        emit finished();
        return;
    }

    if (m_path.isEmpty()) {
        m_errorString = QStringLiteral("Empty path");
        emit finished();
        return;
    }

    const QString memKey = memoryKey(m_path, m_requestedSize);

    // 1) 内存 LRU
    if (m_memoryCache) {
        m_image = m_memoryCache->get(memKey);
        if (!m_image.isNull()) {
            emit finished();
            return;
        }
    }

    if (m_cancelled.loadRelaxed()) {
        emit finished();
        return;
    }

    // 2) 磁盘缓存
    const QString cached = cachePath();
    if (QFileInfo::exists(cached)) {
        QFileInfo sourceInfo(m_path);
        QFileInfo cacheInfo(cached);
        if (sourceInfo.lastModified() <= cacheInfo.lastModified()) {
            m_image = QImage(cached);
            if (!m_image.isNull()) {
                if (m_memoryCache)
                    m_memoryCache->put(memKey, m_image);
                emit finished();
                return;
            }
        }
    }

    if (m_cancelled.loadRelaxed()) {
        emit finished();
        return;
    }

    // 3) 生成
    if (isVideo())
        m_image = loadVideoFrame();
    else
        m_image = loadImage();

    if (m_cancelled.loadRelaxed()) {
        m_image = {};
        emit finished();
        return;
    }

    if (m_image.isNull()) {
        if (m_errorString.isEmpty())
            m_errorString = QStringLiteral("Failed to generate thumbnail for: ") + m_path;
    } else {
        QDir().mkpath(cacheDir());
        m_image.save(cached, "JPEG", LianwallGui::Thumbnail::QUALITY);
        if (m_memoryCache)
            m_memoryCache->put(memKey, m_image);
    }

    emit finished();
}

QImage ThumbnailResponse::loadImage() const
{
    QImageReader reader(m_path);
    reader.setAutoTransform(true);

    QSize origSize = reader.size();
    if (origSize.isValid() && (origSize.width() > m_requestedSize.width()
                               || origSize.height() > m_requestedSize.height())) {
        QSize scaled = origSize.scaled(m_requestedSize, Qt::KeepAspectRatio);
        reader.setScaledSize(scaled);
    }

    QImage img = reader.read();
    if (img.isNull())
        return {};

    if (img.width() > m_requestedSize.width() || img.height() > m_requestedSize.height())
        img = img.scaled(m_requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // 统一到较省内存的格式，便于 LRU 估算
    if (img.format() != QImage::Format_RGB32 && img.format() != QImage::Format_ARGB32)
        img = img.convertToFormat(QImage::Format_RGB32);

    return img;
}

QImage ThumbnailResponse::loadVideoFrame() const
{
    if (!m_ffmpegSemaphore->tryAcquire(1, kFfmpegTimeoutMs)) {
        qWarning() << "[ThumbnailProvider] Timeout waiting for ffmpeg slot:" << m_path;
        return {};
    }

    // 取消时尽快释放
    if (m_cancelled.loadRelaxed()) {
        m_ffmpegSemaphore->release();
        return {};
    }

    auto runFfmpeg = [this](const QString &seekSecs) -> QImage {
        QProcess ffmpeg;
        ffmpeg.setProcessChannelMode(QProcess::MergedChannels);

        // -ss 放在 -i 前：输入 seek，对大视频快很多
        const QStringList args = {
            QStringLiteral("-hide_banner"),
            QStringLiteral("-loglevel"), QStringLiteral("error"),
            QStringLiteral("-ss"), seekSecs,
            QStringLiteral("-i"), m_path,
            QStringLiteral("-frames:v"), QStringLiteral("1"),
            QStringLiteral("-vf"),
            QStringLiteral("scale=%1:%2:force_original_aspect_ratio=decrease")
                .arg(m_requestedSize.width())
                .arg(m_requestedSize.height()),
            QStringLiteral("-f"), QStringLiteral("image2pipe"),
            QStringLiteral("-vcodec"), QStringLiteral("mjpeg"),
            QStringLiteral("-q:v"), QStringLiteral("3"),
            QStringLiteral("-"),
        };

        ffmpeg.start(QStringLiteral("ffmpeg"), args);
        if (!ffmpeg.waitForStarted(3000))
            return {};

        if (!ffmpeg.waitForFinished(kFfmpegTimeoutMs)) {
            ffmpeg.kill();
            ffmpeg.waitForFinished(2000);
            return {};
        }

        if (ffmpeg.exitCode() != 0)
            return {};

        QImage result;
        result.loadFromData(ffmpeg.readAllStandardOutput(), "JPG");
        return result;
    };

    QImage result = runFfmpeg(QString::number(LianwallGui::Thumbnail::SEEK_SECONDS));
    if (result.isNull() && !m_cancelled.loadRelaxed())
        result = runFfmpeg(QStringLiteral("0"));

    m_ffmpegSemaphore->release();

    if (!result.isNull()
        && result.format() != QImage::Format_RGB32
        && result.format() != QImage::Format_ARGB32) {
        result = result.convertToFormat(QImage::Format_RGB32);
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
    : m_ffmpegSemaphore(2)
    , m_memoryCache(std::make_shared<ThumbnailMemoryCache>(96LL * 1024 * 1024))
{
    // 磁盘命中很快，可多线程；ffmpeg 另有信号量限制
    m_pool.setMaxThreadCount(6);
}

ThumbnailProvider::~ThumbnailProvider()
{
    m_pool.waitForDone(5000);
}

void ThumbnailProvider::setMemoryBudget(qint64 bytes)
{
    if (m_memoryCache)
        m_memoryCache->setBudget(bytes);
}

qint64 ThumbnailProvider::memoryBudget() const
{
    return m_memoryCache ? m_memoryCache->budget() : 0;
}

qint64 ThumbnailProvider::memoryBytesUsed() const
{
    return m_memoryCache ? m_memoryCache->bytesUsed() : 0;
}

QQuickImageResponse *ThumbnailProvider::requestImageResponse(
    const QString &id, const QSize &requestedSize)
{
    QString cleanId = id;
    int queryIdx = cleanId.indexOf(QLatin1Char('?'));
    if (queryIdx >= 0)
        cleanId = cleanId.left(queryIdx);

    return new ThumbnailResponse(cleanId, requestedSize, &m_pool,
                                 &m_ffmpegSemaphore, m_memoryCache);
}
