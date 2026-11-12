#include "ThumbnailProvider.h"
#include "SettingsStorage.h"
#include "Constants.h"
#include <QtTest>
#include <QTemporaryDir>
#include <QSaveFile>
#include <QFileInfo>
#include <QImage>
#include <QUrl>
#include <QScopeGuard>

class TestThumbnail : public QObject
{
    Q_OBJECT
private:
    QTemporaryDir directory;
    QByteArray originalCache;
    QByteArray originalConfig;
    QThreadPool pool;
    QSemaphore semaphore{2};
    std::shared_ptr<ThumbnailMemoryCache> memory;
    std::shared_ptr<ThumbnailWorkState> work;

    QImage thumbnail(const QString &path)
    {
        ThumbnailResponse response(QString::fromLatin1(QUrl::toPercentEncoding(path)), QSize(64, 64), &pool, &semaphore, memory, work);
        if (!pool.waitForDone(5000))
            return {};
        std::unique_ptr<QQuickTextureFactory> factory(response.textureFactory());
        return factory ? factory->image() : QImage();
    }

    QString source(const QString &name, Qt::GlobalColor color, QSize size = QSize(16, 16))
    {
        const QString path = directory.path() + "/" + name;
        QImage image(size, QImage::Format_RGB32);
        image.fill(color);
        if (!image.save(path, "PNG"))
            return {};
        return path;
    }

private slots:
    void init()
    {
        QVERIFY(directory.isValid());
        originalCache = qgetenv("XDG_CACHE_HOME");
        originalConfig = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CACHE_HOME", (directory.path() + "/cache").toUtf8());
        qputenv("XDG_CONFIG_HOME", (directory.path() + "/config").toUtf8());
        QVERIFY(LianwallGui::Paths::thumbnailDir().startsWith(directory.path()));
        memory = std::make_shared<ThumbnailMemoryCache>();
        work = std::make_shared<ThumbnailWorkState>();
    }

    void cleanup()
    {
        work->stopping.store(true);
        pool.waitForDone();
        qputenv("XDG_CACHE_HOME", originalCache);
        qputenv("XDG_CONFIG_HOME", originalConfig);
        QDir(directory.path() + "/cache").removeRecursively();
        QDir(directory.path() + "/config").removeRecursively();
    }

    void replacedOrDeletedSourceDoesNotReturnOldMemoryImage()
    {
        const QString path = source("中文 # image.png", Qt::red);
        QVERIFY(!path.isEmpty());
        QVERIFY(thumbnail(path).pixelColor(0, 0).red() > 200);
        source("中文 # image.png", Qt::blue, QSize(24, 24));
        const QImage replaced = thumbnail(path);
        QVERIFY(!replaced.isNull());
        QVERIFY(replaced.pixelColor(0, 0).blue() > 200);
        QVERIFY(QFile::remove(path));
        QVERIFY(thumbnail(path).isNull());
    }

    void corruptedDiskCacheIsRegenerated()
    {
        const QString path = source("corrupt.png", Qt::green);
        QVERIFY(!thumbnail(path).isNull());
        const QString cache = LianwallGui::Paths::thumbnailDir();
        const auto files = QDir(cache).entryList({"v2_*.jpg"}, QDir::Files);
        QCOMPARE(files.size(), 1);
        QFile file(cache + "/" + files.first());
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        file.write("broken");
        file.close();
        memory->clear();
        const QImage regenerated = thumbnail(path);
        QVERIFY(!regenerated.isNull());
        QVERIFY(!QImage(file.fileName()).isNull());
    }

    void simultaneousRequestsShareOneCacheFile()
    {
        const QString path = source("duplicate.png", Qt::red);
        QList<ThumbnailResponse *> responses;
        for (int index = 0; index < 12; ++index)
            responses.append(new ThumbnailResponse(QString::fromLatin1(QUrl::toPercentEncoding(path)), QSize(64, 64), &pool, &semaphore, memory, work));
        QVERIFY(pool.waitForDone(5000));
        for (auto *response : responses) {
            std::unique_ptr<QQuickTextureFactory> factory(response->textureFactory());
            QVERIFY(factory);
            delete response;
        }
        QCOMPARE(QDir(LianwallGui::Paths::thumbnailDir()).entryList({"v2_*.jpg"}, QDir::Files).size(), 1);
        QCOMPARE(work->generationAttempts.load(), quint64(1));
    }

    void cancellationWhileWaitingForVideoSlotFinishesPromptly()
    {
        const QString path = directory.path() + "/waiting.mp4";
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
        QSemaphore unavailable(0);
        ThumbnailResponse response(QString::fromLatin1(QUrl::toPercentEncoding(path)), QSize(64, 64), &pool, &unavailable, memory, work);
        QTest::qWait(60);
        response.cancel();
        QVERIFY(pool.waitForDone(1000));
        QVERIFY(!response.textureFactory());
    }

    void diskBudgetDeletesOnlyOwnedCacheFiles()
    {
        const QString cache = LianwallGui::Paths::thumbnailDir();
        QVERIFY(QDir().mkpath(cache));
        for (int index = 0; index < 4; ++index) {
            QFile file(cache + "/v2_" + QString(64, QChar('a' + index)) + "_320x180.jpg");
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write(QByteArray(20, 'x'));
        }
        const QString unrelated = source("original.png", Qt::red);
        QVERIFY(QFile::copy(unrelated, cache + "/keep.jpg"));
        ThumbnailWorkState limited(100, 2);
        limited.maintainDiskCache(cache, true);
        QCOMPARE(QDir(cache).entryList({"v2_*.jpg"}, QDir::Files).size(), 2);
        QVERIFY(QFile::exists(cache + "/keep.jpg"));
        QVERIFY(QFile::exists(unrelated));
    }

    void settingsMigrationKeepsExistingDestinationAndLegacyFile()
    {
        const QString legacy = directory.path() + "/legacy.conf";
        const QString destination = LianwallGui::Paths::configDir() + "/gui.conf";
        QFile file(legacy);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("[app]\ntheme=dark\n");
        file.close();
        QVERIFY(LianwallGui::Paths::migrateGuiSettings(legacy, destination));
        QVERIFY(QFile::exists(legacy));
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        file.write("different");
        file.close();
        QVERIFY(LianwallGui::Paths::migrateGuiSettings(legacy, destination));
        QFile migrated(destination);
        QVERIFY(migrated.open(QIODevice::ReadOnly));
        QCOMPARE(migrated.readAll(), QByteArray("[app]\ntheme=dark\n"));
        QCOMPARE(LianwallGui::Paths::cacheDir(), directory.path() + "/cache/lianwall");
    }
    void cacheWriteFailureStillReturnsTheImage()
    {
        const QString cache = LianwallGui::Paths::thumbnailDir();
        QVERIFY(QDir().mkpath(QFileInfo(cache).absolutePath()));
        QFile blocker(cache);
        QVERIFY(blocker.open(QIODevice::WriteOnly));
        blocker.write("keep");
        blocker.close();
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("\\[ThumbnailProvider\\] Failed to save cache:.*"));
        QVERIFY(!thumbnail(source("uncached.png", Qt::blue)).isNull());
        QVERIFY(blocker.open(QIODevice::ReadOnly));
        QCOMPARE(blocker.readAll(), QByteArray("keep"));
    }

    void updatingMemoryEntryRespectsBudget()
    {
        ThumbnailMemoryCache cache(8LL * 1024 * 1024);
        QImage first(1000, 1000, QImage::Format_RGB32);
        first.fill(Qt::red);
        cache.put("a", first);
        cache.put("b", first);
        QImage bigger(1200, 1200, QImage::Format_RGB32);
        bigger.fill(Qt::blue);
        cache.put("a", bigger);
        QVERIFY(cache.bytesUsed() <= cache.budget());
        QVERIFY(cache.get("b").isNull());
        QVERIFY(!cache.get("a").isNull());
        cache.put("oversized", QImage(2000, 2000, QImage::Format_RGB32));
        QVERIFY(cache.bytesUsed() <= cache.budget());
        QVERIFY(cache.get("oversized").isNull());
    }

    void providerShutdownCancelsRunningVideoWorker()
    {
        const QByteArray previousPath = qgetenv("PATH");
        const auto restore = qScopeGuard([previousPath]() { qputenv("PATH", previousPath); });
        const QString program = directory.path() + "/ffmpeg";
        QFile script(program);
        QVERIFY(script.open(QIODevice::WriteOnly | QIODevice::Truncate));
        script.write("#!/bin/sh\nprintf started > \"$0.started\"\nexec sleep 10\n");
        script.close();
        QVERIFY(script.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        qputenv("PATH", directory.path().toUtf8() + ':' + previousPath);
        const QString video = directory.path() + "/shutdown.mp4";
        QFile file(video);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
        std::unique_ptr<QQuickImageResponse> response;
        QElapsedTimer elapsed;
        {
            ThumbnailProvider provider;
            response.reset(provider.requestImageResponse(QString::fromLatin1(QUrl::toPercentEncoding(video)), QSize(64, 64)));
            QTRY_VERIFY(QFile::exists(program + ".started"));
            elapsed.start();
        }
        QVERIFY(elapsed.elapsed() < 2000);
    }

};

QTEST_MAIN(TestThumbnail)
#include "TestThumbnail.moc"
