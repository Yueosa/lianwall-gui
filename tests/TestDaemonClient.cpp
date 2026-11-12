#include "DaemonClient.h"
#include "ConfigManager.h"
#include "DaemonState.h"
#include "WallpaperController.h"
#include "WallpaperListModel.h"

#include <QJsonDocument>
#include <QFile>
#include <QLocalServer>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>
#include <memory>

class TestDaemonClient : public QObject
{
    Q_OBJECT
private:
    QTemporaryDir directory;
    QLocalServer server;
    std::unique_ptr<DaemonClient> client;
    std::unique_ptr<QLocalSocket> peer;

    QJsonObject request()
    {
        QElapsedTimer wait;
        wait.start();
        while (!peer->canReadLine() && wait.elapsed() < 1000) {
            QCoreApplication::processEvents();
            peer->waitForReadyRead(5);
        }
        return QJsonDocument::fromJson(peer->readLine()).object();
    }

    void reply(quint64 id, const QString &type, const QJsonObject &payload = {})
    {
        peer->write(QJsonDocument(QJsonObject{
            {"id", static_cast<qint64>(id)}, {"type", type}, {"payload", payload},
        }).toJson(QJsonDocument::Compact) + '\n');
        peer->flush();
    }

    void event(const QString &type, const QJsonObject &data)
    {
        reply(0, "Event", {{"event", type}, {"data", data}});
    }

    static QJsonObject space(const QString &mode, const QString &path, bool locked = false)
    {
        return {{"mode", mode}, {"items", QJsonArray{QJsonObject{
            {"path", path}, {"filename", path}, {"locked", locked},
        }}}};
    }

private slots:
    void init()
    {
        QVERIFY(directory.isValid());
        const QString path = directory.path() + "/daemon.sock";
        QVERIFY(server.listen(path));
        const QString executable = directory.path() + "/fake-daemon.sh";
        QFile::remove(executable + ".called");
        QFile script(executable);
        QVERIFY(script.open(QIODevice::WriteOnly | QIODevice::Truncate));
        script.write("#!/bin/sh\nprintf called > \"$0.called\"\n");
        script.close();
        QVERIFY(script.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        client = std::make_unique<DaemonClient>(nullptr, 200, executable);
        client->setSocketPath(path);
        client->connectToDaemon();
        QTRY_VERIFY(server.hasPendingConnections());
        peer.reset(server.nextPendingConnection());
        QTRY_VERIFY(client->isConnected());
        client->setAutoReconnect(false);
    }

    void cleanup()
    {
        client.reset();
        peer.reset();
        server.close();
    }

    void imageArgumentsUseNewKeyForSnapshotEventAndMutation()
    {
        ConfigManager config(client.get());
        config.reload();
        const auto query = request();
        reply(query["id"].toInteger(), "Config", {{"key", QJsonValue::Null},
              {"value", QJsonObject{{"image_engine", QJsonObject{{"awww_args", QJsonArray{"--transition-type", "fade"}}}}}}});
        QTRY_COMPARE(config.awwwArgs(), (QStringList{"--transition-type", "fade"}));
        event("ConfigChanged", {{"key", "image_engine.awww_args"}, {"new_value", QJsonArray{"--transition-type", "outer"}}});
        QTRY_COMPARE(config.awwwArgs(), (QStringList{"--transition-type", "outer"}));
        config.setAwwwArgs({"--transition-type", "wipe"});
        const auto mutation = request();
        QCOMPARE(mutation["key"].toString(), QString("image_engine.awww_args"));
        QCOMPARE(mutation["value"].toArray(), (QJsonArray{"--transition-type", "wipe"}));
        reply(mutation["id"].toInteger(), "Ok");
    }

    void configDropsSnapshotInvalidatedByEventAndCoalescesReloads()
    {
        ConfigManager config(client.get());
        config.reload();
        const auto first = request();
        config.reload();
        config.reload();
        event("ConfigChanged", {{"key", "image_engine.awww_args"}, {"new_value", QJsonArray{"fresh"}}});
        QTRY_COMPARE(config.awwwArgs(), (QStringList{"fresh"}));
        reply(first["id"].toInteger(), "Config", {{"value", QJsonObject{{"image_engine", QJsonObject{{"awww_args", QJsonArray{"old"}}}}}}});
        const auto second = request();
        QVERIFY(!second.isEmpty());
        QCOMPARE(config.awwwArgs(), (QStringList{"fresh"}));
        reply(second["id"].toInteger(), "Config", {{"value", QJsonObject{{"image_engine", QJsonObject{{"awww_args", QJsonArray{"current"}}}}}}});
        QTRY_COMPARE(config.awwwArgs(), (QStringList{"current"}));
        QTest::qWait(20);
        QVERIFY(!peer->canReadLine());
        QVERIFY(config.configLoaded());
        client->disconnectFromDaemon();
        QVERIFY(!config.configLoaded());
    }

    void destroyedConfigManagerDoesNotReceivePendingCallbacks()
    {
        auto config = std::make_unique<ConfigManager>(client.get());
        config->reload();
        const auto query = request();
        config->setAwwwArgs({"test"});
        const auto mutation = request();
        config.reset();
        reply(query["id"].toInteger(), "Config", {{"value", QJsonObject{}}});
        reply(mutation["id"].toInteger(), "Error", {{"code", "InvalidRequest"}, {"message", "rejected"}});
        QTest::qWait(20);
        QVERIFY(client->isConnected());
    }

    void restartWaitsForOldSocketRemovalBeforeLaunching()
    {
        int callbacks = 0;
        client->restartDaemon([&](const Daemon::Response &response) {
            ++callbacks;
            QCOMPARE(response.type, Daemon::ResponseType::Ok);
        });
        const auto shutdown = request();
        QCOMPARE(shutdown["cmd"].toString(), QString("Shutdown"));
        reply(shutdown["id"].toInteger(), "Ok");
        QTRY_VERIFY(!client->isConnected());
        const QString marker = directory.path() + "/fake-daemon.sh.called";
        QTest::qWait(350);
        QVERIFY(!QFile::exists(marker));
        QCOMPARE(callbacks, 0);
        server.close();
        QLocalServer::removeServer(directory.path() + "/daemon.sock");
        QTRY_COMPARE(callbacks, 1);
        QTRY_VERIFY(QFile::exists(marker));
        client->setAutoReconnect(false);
    }

    void oversizedRequestIsRejectedWithoutDisconnectingOrSending()
    {
        int callbacks = 0;
        client->setConfig("video_engine.mpv_args", QJsonArray{QString(1024 * 1024, 'x')}, [&](const Daemon::Response &response) {
            ++callbacks;
            QCOMPARE(response.payload["code"].toString(), QString("request_too_large"));
        });
        QCOMPARE(callbacks, 1);
        QVERIFY(client->isConnected());
        QTest::qWait(20);
        QVERIFY(!peer->canReadLine());
    }

    void oversizedUnterminatedResponseResetsConnection()
    {
        QSignalSpy errors(client.get(), &DaemonClient::errorOccurred);
        const QByteArray oversized(64 * 1024 * 1024 + 1, ' ');
        QCOMPARE(peer->write(oversized), qint64(oversized.size()));
        peer->flush();
        QTRY_VERIFY_WITH_TIMEOUT(!client->isConnected(), 5000);
        QVERIFY(!errors.isEmpty());
        QVERIFY(errors.first().first().toString().contains("frame limit"));
    }

    void manualDisconnectNotifiesPending()
    {
        int completed = 0;
        client->ping([&](const Daemon::Response &response) {
            ++completed;
            QCOMPARE(response.type, Daemon::ResponseType::Error);
        });
        QVERIFY(request().contains("id"));
        client->disconnectFromDaemon();
        QTRY_COMPARE_WITH_TIMEOUT(completed, 1, 1000);
        QVERIFY(!client->isConnected());
    }

    void taggedStatusDoesNotConsumeInitialSync()
    {
        QSignalSpy initial(client.get(), &DaemonClient::initialStatusReceived);
        client->subscribe({Daemon::EventType::All}, true);
        const auto subscription = request();
        reply(subscription["id"].toInteger(), "Subscribed");
        QTest::qWait(10);
        int completed = 0;
        client->getStatus([&](const Daemon::Response &) { ++completed; });
        const auto status = request();
        reply(status["id"].toInteger(), "Status", {{"mode", "Image"}});
        reply(0, "Status", {{"mode", "Video"}});
        QTRY_COMPARE_WITH_TIMEOUT(completed, 1, 1000);
        QCOMPARE(initial.count(), 1);
    }

    void subscriptionWithoutSyncDoesNotStealStatus()
    {
        client->subscribe({Daemon::EventType::All}, false);
        const auto subscription = request();
        reply(subscription["id"].toInteger(), "Subscribed");
        QTest::qWait(10);
        int completed = 0;
        client->getStatus([&](const Daemon::Response &) { ++completed; });
        const auto status = request();
        reply(status["id"].toInteger(), "Status");
        QTRY_COMPARE_WITH_TIMEOUT(completed, 1, 1000);
    }
    void timeoutFailsPendingOnceAndAllowsReentrantFailure()
    {
        int completed = 0;
        int nested = 0;
        client->ping([&](const Daemon::Response &response) {
            ++completed;
            QCOMPARE(response.payload["code"].toString(), QString("timeout"));
            QVERIFY(!client->isConnected());
            client->ping([&](const Daemon::Response &) { ++nested; });
        });
        QVERIFY(request().contains("id"));
        QTRY_COMPARE_WITH_TIMEOUT(completed, 1, 1000);
        QCOMPARE(nested, 1);
        client->disconnectFromDaemon();
        QCOMPARE(completed, 1);
    }

    void malformedResponseFailsPending()
    {
        int completed = 0;
        client->ping([&](const Daemon::Response &response) {
            ++completed;
            QCOMPARE(response.payload["code"].toString(), QString("protocol_error"));
        });
        QVERIFY(request().contains("id"));
        peer->write("not json\n");
        peer->flush();
        QTRY_COMPARE_WITH_TIMEOUT(completed, 1, 1000);
        QVERIFY(!client->isConnected());
    }

    void outOfOrderRepliesMatchIds()
    {
        QStringList order;
        client->ping([&](const Daemon::Response &) { order.append("first"); });
        const auto first = request();
        client->getStatus([&](const Daemon::Response &) { order.append("second"); });
        const auto second = request();
        reply(second["id"].toInteger(), "Status");
        reply(first["id"].toInteger(), "Pong");
        QTRY_COMPARE(order.size(), 2);
        QCOMPARE(order, QStringList({"second", "first"}));
    }

    void missingInitialSnapshotTimesOut()
    {
        client->subscribe({Daemon::EventType::All}, true);
        const auto subscription = request();
        reply(subscription["id"].toInteger(), "Subscribed");
        QTRY_VERIFY_WITH_TIMEOUT(!client->isConnected(), 1000);
    }

    void operationWaitsForItsOwnReply()
    {
        WallpaperController controller(client.get());
        QSignalSpy refreshed(&controller, &WallpaperController::refreshRequested);
        QVERIFY(controller.next());
        const auto command = request();
        QVERIFY(controller.busy());
        QVERIFY(!controller.setWallpaper("/another.jpg"));
        event("WallpaperChanged", {{"path", "/external.jpg"}, {"mode", "Image"}});
        QTest::qWait(10);
        QVERIFY(controller.busy());
        reply(command["id"].toInteger(), "Ok");
        QTRY_VERIFY(!controller.busy());
        QCOMPARE(refreshed.count(), 1);
        QVERIFY(controller.prev());
        QVERIFY(controller.busy());
        // A late reply for the previous command cannot complete this command.
        reply(command["id"].toInteger(), "Ok");
        QTest::qWait(10);
        QVERIFY(controller.busy());
        const auto nextCommand = request();
        reply(nextCommand["id"].toInteger(), "Ok");
        QTRY_VERIFY(!controller.busy());
    }

    void disconnectedOperationHasUnknownResult()
    {
        WallpaperController controller(client.get());
        QSignalSpy errors(&controller, &WallpaperController::errorOccurred);
        QVERIFY(controller.setWallpaper("/sample.jpg"));
        QVERIFY(request().contains("id"));
        client->disconnectFromDaemon();
        QVERIFY(!controller.busy());
        QCOMPARE(errors.count(), 1);
        QCOMPARE(errors.at(0).at(0).toString(), QString("result_unknown"));
        QVERIFY(!controller.next());
    }

    void modelKeepsLatestLoadIntent()
    {
        WallpaperController controller(client.get());
        WallpaperListModel model(client.get(), &controller);
        model.load("Video");
        const auto old = request();
        model.load("Video");
        model.load("Image");
        reply(old["id"].toInteger(), "Space", space("Video", "/old.mp4"));
        const auto latest = request();
        QCOMPARE(latest["mode"].toString(), QString("Image"));
        QCOMPARE(model.count(), 0);
        reply(latest["id"].toInteger(), "Space", space("Image", "/new.jpg"));
        QTRY_VERIFY(!model.loading());
        QCOMPARE(model.pathAt(0), QString("/new.jpg"));
    }

    void modelDropsSnapshotInvalidatedByEvent()
    {
        WallpaperController controller(client.get());
        WallpaperListModel model(client.get(), &controller);
        model.load();
        const auto old = request();
        event("SpaceUpdated", {{"mode", "Video"}});
        reply(old["id"].toInteger(), "Space", space("Video", "/old.mp4"));
        const auto latest = request();
        QCOMPARE(model.count(), 0);
        reply(latest["id"].toInteger(), "Space", space("Video", "/new.mp4"));
        QTRY_VERIFY(!model.loading());
        QCOMPARE(model.pathAt(0), QString("/new.mp4"));
    }

    void lockReplyDoesNotInvertRefreshedState()
    {
        WallpaperController controller(client.get());
        WallpaperListModel model(client.get(), &controller);
        model.load();
        auto load = request();
        reply(load["id"].toInteger(), "Space", space("Image", "/image.jpg"));
        QTRY_VERIFY(!model.loading());
        model.toggleLock(0);
        const auto lock = request();
        model.load();
        load = request();
        reply(load["id"].toInteger(), "Space", space("Image", "/image.jpg", true));
        QTRY_VERIFY(!model.loading());
        reply(lock["id"].toInteger(), "Ok");
        QTest::qWait(10);
        QVERIFY(model.lockedAt(0));
    }

    void disconnectReleasesModelLoading()
    {
        WallpaperController controller(client.get());
        WallpaperListModel model(client.get(), &controller);
        model.load();
        QVERIFY(request().contains("id"));
        QVERIFY(model.loading());
        client->disconnectFromDaemon();
        QVERIFY(!model.loading());
    }

    void stateDoesNotOverwriteNewEventWithOldSnapshot()
    {
        DaemonState state(client.get());
        state.refresh();
        const auto old = request();
        event("WallpaperChanged", {{"path", "/new.jpg"}, {"filename", "new.jpg"}, {"mode", "Image"}, {"trigger", "manual_set"}});
        reply(old["id"].toInteger(), "Status", {{"mode", "Video"}, {"current", "/old.mp4"}});
        const auto latest = request();
        QCOMPARE(state.currentPath(), QString("/new.jpg"));
        reply(latest["id"].toInteger(), "Status", {{"mode", "Image"}, {"current", "/new.jpg"}, {"next_switch_secs", 100}});
        QTRY_COMPARE(state.nextSwitchSecs(), 100);
        QCOMPARE(state.mode(), QString("Image"));
    }

    void unsolicitedScheduleSnapshotRefreshesCountdown()
    {
        DaemonState state(client.get());
        reply(0, "Status", {{"mode", "Video"}, {"next_switch_secs", 80}});
        QTRY_COMPARE(state.nextSwitchSecs(), 80);
        reply(0, "Status", {{"mode", "Video"}, {"next_switch_secs", QJsonValue::Null}});
        QTRY_COMPARE(state.nextSwitchSecs(), -1);
    }

    void reconnectResubscribesWithoutReplayingMutation()
    {
        DaemonState state(client.get());
        WallpaperController controller(client.get());
        QSignalSpy errors(&controller, &WallpaperController::errorOccurred);
        QVERIFY(controller.next());
        const auto oldCommand = request();
        peer->abort();
        QTRY_VERIFY(!client->isConnected());
        QTRY_VERIFY(!controller.busy());
        QCOMPARE(errors.count(), 1);
        client->connectToDaemon();
        QTRY_VERIFY(server.hasPendingConnections());
        peer.reset(server.nextPendingConnection());
        QTRY_VERIFY(client->isConnected());
        client->setAutoReconnect(false);
        const auto subscription = request();
        QCOMPARE(subscription["cmd"].toString(), QString("Subscribe"));
        reply(subscription["id"].toInteger(), "Subscribed");
        reply(0, "Status", {{"mode", "Image"}, {"current", "/actual.jpg"}});
        QTRY_COMPARE(state.currentPath(), QString("/actual.jpg"));
        QVERIFY(controller.next());
        const auto newCommand = request();
        reply(oldCommand["id"].toInteger(), "Ok");
        QTest::qWait(10);
        QVERIFY(controller.busy());
        reply(newCommand["id"].toInteger(), "Ok");
        QTRY_VERIFY(!controller.busy());
        QCOMPARE(errors.count(), 1);
    }

    void operationTimeoutReleasesBusy()
    {
        WallpaperController controller(client.get());
        QSignalSpy errors(&controller, &WallpaperController::errorOccurred);
        QVERIFY(controller.next());
        QVERIFY(request().contains("id"));
        QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 2500);
        QCOMPARE(errors.count(), 1);
        QCOMPARE(errors.at(0).at(0).toString(), QString("result_unknown"));
        QVERIFY(!client->isConnected());
    }

    void mismatchedResponseTypeFailsPending()
    {
        int completed = 0;
        client->ping([&](const Daemon::Response &response) {
            ++completed;
            QCOMPARE(response.payload["code"].toString(), QString("protocol_error"));
        });
        const auto ping = request();
        reply(ping["id"].toInteger(), "Status");
        QTRY_COMPARE_WITH_TIMEOUT(completed, 1, 1000);
        QVERIFY(!client->isConnected());
    }

    void daemonTimeoutAlsoResetsSession()
    {
        WallpaperController controller(client.get());
        QSignalSpy errors(&controller, &WallpaperController::errorOccurred);
        QVERIFY(controller.next());
        const auto command = request();
        reply(command["id"].toInteger(), "Error", {{"code", "timeout"}, {"message", "Command timeout"}});
        QTRY_VERIFY(!controller.busy());
        QVERIFY(!client->isConnected());
        QCOMPARE(errors.count(), 1);
        QCOMPARE(errors.at(0).at(0).toString(), QString("result_unknown"));
    }

    void peerShutdownDoesNotAutoStartDaemonAgain()
    {
        client->setAutoReconnect(true);
        server.close();
        peer->abort();
        QTRY_VERIFY(!client->isConnected());
        // Also cover the subsequent reconnect failure, not just PeerClosedError.
        QTest::qWait(1200);
        QVERIFY(!QFile::exists(directory.path() + "/fake-daemon.sh.called"));
        client->setAutoReconnect(false);
    }

};

QTEST_GUILESS_MAIN(TestDaemonClient)
#include "TestDaemonClient.moc"
