#include "ThumbnailCache.h"
#include "Constants.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QtConcurrent>
#include <QDebug>

using namespace LianwallGui;

ThumbnailCache::ThumbnailCache(QObject *parent)
    : QObject(parent)
{
    ensureCacheDir();
}

void ThumbnailCache::ensureCacheDir() {
    QDir().mkpath(Paths::thumbnailDir());
}

QString ThumbnailCache::hashPath(const QString &path) {
    QByteArray hash = QCryptographicHash::hash(path.toUtf8(), QCryptographicHash::Md5);
    return QString::fromLatin1(hash.toHex());
}

QString ThumbnailCache::thumbnailPath(const QString &videoPath) {
    QString hash = hashPath(videoPath);
    return QString("%1/%2.%3").arg(Paths::thumbnailDir(), hash, Thumbnail::FORMAT);
}

bool ThumbnailCache::hasThumbnail(const QString &videoPath) {
    return QFile::exists(thumbnailPath(videoPath));
}

QString ThumbnailCache::getThumbnail(const QString &videoPath) {
    // 检查缓存
    if (m_cache.contains(videoPath)) {
        return m_cache[videoPath];
    }
    
    QString thumbPath = thumbnailPath(videoPath);
    
    // 检查文件是否存在
    if (QFile::exists(thumbPath)) {
        m_cache[videoPath] = thumbPath;
        return thumbPath;
    }
    
    // 同步生成
    if (generateThumbnail(videoPath, thumbPath)) {
        m_cache[videoPath] = thumbPath;
        return thumbPath;
    }
    
    return QString();
}

void ThumbnailCache::generateThumbnailAsync(const QString &videoPath) {
    QString thumbPath = thumbnailPath(videoPath);
    
    // 已存在则直接返回
    if (QFile::exists(thumbPath)) {
        emit thumbnailReady(videoPath, thumbPath);
        return;
    }
    
    // 异步生成
    QtConcurrent::run([this, videoPath, thumbPath]() {
        if (generateThumbnail(videoPath, thumbPath)) {
            m_cache[videoPath] = thumbPath;
            emit thumbnailReady(videoPath, thumbPath);
        } else {
            emit thumbnailFailed(videoPath, tr("Failed to generate thumbnail"));
        }
    });
}

bool ThumbnailCache::generateThumbnail(const QString &videoPath, const QString &outputPath) {
    // 使用 ffmpeg 提取第1秒的帧
    QProcess ffmpeg;
    QStringList args = {
        "-ss", "1",                          // 跳到第1秒
        "-i", videoPath,                     // 输入文件
        "-vframes", "1",                     // 只提取1帧
        "-vf", QString("scale=%1:%2:force_original_aspect_ratio=decrease,pad=%1:%2:(ow-iw)/2:(oh-ih)/2")
               .arg(Thumbnail::WIDTH).arg(Thumbnail::HEIGHT),
        "-pix_fmt", "yuvj420p",              // 使用标准 JPEG YUV 格式
        "-q:v", QString::number(100 - Thumbnail::QUALITY),  // 质量
        "-y",                                // 覆盖已存在文件
        outputPath
    };
    
    ffmpeg.start("ffmpeg", args);
    ffmpeg.waitForFinished(10000);
    
    if (ffmpeg.exitCode() != 0) {
        qWarning() << "[ThumbnailCache] ffmpeg failed for:" << videoPath;
        qWarning() << ffmpeg.readAllStandardError();
        return false;
    }
    
    qDebug() << "[ThumbnailCache] Generated thumbnail for:" << videoPath;
    return QFile::exists(outputPath);
}

void ThumbnailCache::cleanup(const QStringList &validPaths) {
    QSet<QString> validHashes;
    for (const QString &path : validPaths) {
        validHashes.insert(hashPath(path));
    }
    
    QDir cacheDir(Paths::thumbnailDir());
    QStringList filters = {QString("*.%1").arg(Thumbnail::FORMAT)};
    
    int removed = 0;
    for (const QString &file : cacheDir.entryList(filters, QDir::Files)) {
        QString hash = QFileInfo(file).baseName();
        if (!validHashes.contains(hash)) {
            cacheDir.remove(file);
            removed++;
        }
    }
    
    qDebug() << "[ThumbnailCache] Cleanup removed" << removed << "orphaned thumbnails";
}

qint64 ThumbnailCache::cacheSize() {
    qint64 size = 0;
    QDir cacheDir(Paths::thumbnailDir());
    
    for (const QFileInfo &info : cacheDir.entryInfoList(QDir::Files)) {
        size += info.size();
    }
    
    return size;
}

void ThumbnailCache::clearAll() {
    QDir cacheDir(Paths::thumbnailDir());
    
    for (const QString &file : cacheDir.entryList(QDir::Files)) {
        cacheDir.remove(file);
    }
    
    m_cache.clear();
    qDebug() << "[ThumbnailCache] Cache cleared";
}
