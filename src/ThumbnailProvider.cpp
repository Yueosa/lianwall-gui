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
#include <QSaveFile>
#include <QRegularExpression>
#include <QScopeGuard>

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
    return img.sizeInBytes();
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
    auto old = m_entries.find(key);
    if (old != m_entries.end()) {
        m_bytes -= old->bytes;
        m_entries.erase(old);
        m_lru.removeOne(key);
    }
    if (bytes > m_budget)
        return;
    evictLocked(bytes);
    m_entries.insert(key, Entry{image, bytes});
    m_lru.append(key);
    m_bytes += bytes;
}

qint64 ThumbnailMemoryCache::budget() const
{
    QMutexLocker lock(&m_mutex);
    return m_budget;
}

ThumbnailWorkState::ThumbnailWorkState(qint64 diskBudget, int maxEntries)
    : m_diskBudget(qMax<qint64>(0, diskBudget)), m_maxEntries(qMax(0, maxEntries))
{
}

std::shared_ptr<QMutex> ThumbnailWorkState::mutexFor(const QString &key)
{
    QMutexLocker lock(&m_mutex);
    if (auto current = m_keys.value(key).lock())
        return current;
    if (m_keys.size() >= 512) {
        for (auto it = m_keys.begin(); it != m_keys.end();) {
            if (it.value().expired()) it = m_keys.erase(it); else ++it;
        }
    }
    auto current = std::make_shared<QMutex>();
    m_keys.insert(key, current);
    return current;
}

void ThumbnailWorkState::maintainDiskCache(const QString &directory, bool force)
{
    if (!m_maintenanceMutex.tryLock())
        return;
    const auto unlock = qScopeGuard([this]() { m_maintenanceMutex.unlock(); });
    ++m_writes;
    if (!force && m_lastMaintenance.isValid() && m_lastMaintenance.elapsed() < 60000 && m_writes % 64 != 0)
        return;
    m_lastMaintenance.start();
    // Only v2 filenames owned by this cache are eligible; never delete source images.
    static const QRegularExpression owned(QStringLiteral("^v2_[0-9a-f]{64}_[0-9]+x[0-9]+\\.jpg$"));
    auto files = QDir(directory).entryInfoList({QStringLiteral("v2_*.jpg")}, QDir::Files | QDir::NoSymLinks, QDir::Time | QDir::Reversed);
    QList<QFileInfo> candidates;
    qint64 total = 0;
    for (const auto &file : files) {
        if (owned.match(file.fileName()).hasMatch()) {
            candidates.append(file);
            total += file.size();
        }
    }
    int remaining = candidates.size();
    for (const auto &file : candidates) {
        if (total <= m_diskBudget && remaining <= m_maxEntries)
            break;
        if (QFile::remove(file.absoluteFilePath())) {
            total -= file.size();
            --remaining;
        }
    }
}

// ============================================================================
// ThumbnailResponse
// ============================================================================

ThumbnailResponse::ThumbnailResponse(const QString &path, const QSize &requestedSize,
                                     QThreadPool *pool, QSemaphore *ffmpegSemaphore,
                                     std::shared_ptr<ThumbnailMemoryCache> memoryCache,
                                     std::shared_ptr<ThumbnailWorkState> workState)
    : m_path(QUrl::fromPercentEncoding(path.toUtf8()))
    , m_requestedSize(snapSize(requestedSize))
    , m_ffmpegSemaphore(ffmpegSemaphore)
    , m_memoryCache(std::move(memoryCache))
    , m_workState(std::move(workState))
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
    return req.height() > req.width() ? QSize(pair.second, pair.first) : QSize(pair.first, pair.second);
}

bool ThumbnailResponse::cancelled() const
{
    return m_cancelled.loadRelaxed() || m_workState->stopping.load();
}

void ThumbnailResponse::run()
{
    const auto finish = qScopeGuard([this]() { emit finished(); });
    if (cancelled())
        return;
    const QFileInfo source(m_path);
    if (!source.exists() || !source.isFile()) {
        m_errorString = QStringLiteral("Source file does not exist: ") + m_path;
        return;
    }
    const QString key = cacheFilename(m_path, m_requestedSize);
    const auto keyMutex = m_workState->mutexFor(key);
    while (!keyMutex->tryLock(50)) {
        if (cancelled())
            return;
    }
    const auto unlock = qScopeGuard([keyMutex]() { keyMutex->unlock(); });
    if (cancelled())
        return;
    m_workState->maintainDiskCache(cacheDir());
    if (m_memoryCache)
        m_image = m_memoryCache->get(key);
    if (!m_image.isNull()) {
        if (key != cacheFilename(m_path, m_requestedSize) || !QFileInfo::exists(m_path)) {
            m_image = {};
            m_errorString = QStringLiteral("Source changed during thumbnail lookup");
        }
        return;
    }
    const QString cached = cacheDir() + "/" + key;
    m_image = QImage(cached);
    const bool diskHit = !m_image.isNull();
    if (!diskHit) {
        m_workState->generationAttempts.fetch_add(1);
        m_image = isVideo() ? loadVideoFrame() : loadImage();
    }
    if (cancelled()) {
        m_image = {};
        return;
    }
    if (key != cacheFilename(m_path, m_requestedSize) || !QFileInfo::exists(m_path)) {
        m_image = {};
        m_errorString = QStringLiteral("Source changed during thumbnail generation");
        return;
    }
    if (m_image.isNull()) {
        m_errorString = QStringLiteral("Failed to generate thumbnail for: ") + m_path;
        return;
    }
    if (m_memoryCache)
        m_memoryCache->put(key, m_image);
    if (!diskHit) {
        QSaveFile output(cached);
        if (!QDir().mkpath(cacheDir()) || !output.open(QIODevice::WriteOnly)
            || !m_image.save(&output, "JPEG", LianwallGui::Thumbnail::QUALITY) || !output.commit()) {
            qWarning() << "[ThumbnailProvider] Failed to save cache:" << cached << output.errorString();
        } else {
            m_workState->maintainDiskCache(cacheDir());
        }
    }
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
    QElapsedTimer wait;
    wait.start();
    while (!m_ffmpegSemaphore->tryAcquire(1, 50)) {
        if (cancelled() || wait.elapsed() >= kFfmpegTimeoutMs)
            return {};
    }
    const auto release = qScopeGuard([this]() { m_ffmpegSemaphore->release(); });

    // 取消时尽快释放
    if (cancelled()) {
        return {};
    }

    auto runFfmpeg = [this](const QString &seekSecs) -> QImage {
        QProcess ffmpeg;
        ffmpeg.setProcessChannelMode(QProcess::SeparateChannels);

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

        QElapsedTimer elapsed;
        elapsed.start();
        while (ffmpeg.state() != QProcess::NotRunning && !ffmpeg.waitForFinished(50)) {
            if (cancelled() || elapsed.elapsed() >= kFfmpegTimeoutMs) {
                ffmpeg.kill();
                ffmpeg.waitForFinished(1000);
                return {};
            }
        }

        if (ffmpeg.exitCode() != 0)
            return {};

        QImage result;
        result.loadFromData(ffmpeg.readAllStandardOutput(), "JPG");
        return result;
    };

    QImage result = runFfmpeg(QString::number(LianwallGui::Thumbnail::SEEK_SECONDS));
    if (result.isNull() && !cancelled())
        result = runFfmpeg(QStringLiteral("0"));

    if (!result.isNull()
        && result.format() != QImage::Format_RGB32
        && result.format() != QImage::Format_ARGB32) {
        result = result.convertToFormat(QImage::Format_RGB32);
    }
    return result;
}

bool ThumbnailResponse::isVideo() const
{
    auto ext = QFileInfo(m_path).suffix().toLower();
    return kVideoExtensions.contains(ext);
}

QString ThumbnailResponse::cacheFilename(const QString &path, const QSize &size)
{
    const QFileInfo source(path);
    QByteArray identity = source.canonicalFilePath().toUtf8() + '\0';
    identity += QByteArray::number(source.size()) + ':';
    identity += QByteArray::number(source.lastModified().toMSecsSinceEpoch()) + ':';
    identity += QByteArray::number(source.metadataChangeTime().toMSecsSinceEpoch());
    const auto hash = QCryptographicHash::hash(identity, QCryptographicHash::Sha256).toHex();
    return QStringLiteral("v2_%1_%2x%3.jpg").arg(QString::fromLatin1(hash)).arg(size.width()).arg(size.height());
}

QString ThumbnailResponse::cacheDir()
{
    return LianwallGui::Paths::thumbnailDir();
}

// ============================================================================
// ThumbnailProvider
// ============================================================================

ThumbnailProvider::ThumbnailProvider()
    : m_ffmpegSemaphore(2)
    , m_memoryCache(std::make_shared<ThumbnailMemoryCache>(96LL * 1024 * 1024))
    , m_workState(std::make_shared<ThumbnailWorkState>())
{
    // 磁盘命中很快，可多线程；ffmpeg 另有信号量限制
    m_pool.setMaxThreadCount(6);
}

ThumbnailProvider::~ThumbnailProvider()
{
    m_workState->stopping.store(true);
    // Semaphore and worker state must remain alive until all jobs stop.
    m_pool.waitForDone();
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
                                 &m_ffmpegSemaphore, m_memoryCache, m_workState);
}
