#pragma once
#include "DaemonTypes.h"
#include <QObject>
#include <functional>
#include <QVariantMap>
#include <QVariantList>
#include <QSet>
class DaemonClient;

/// New client-facing APIs. Business wallpaper state remains in DaemonState.
class FrontendState : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap engineInfo READ engineInfo NOTIFY engineInfoChanged)
    Q_PROPERTY(QVariantMap runtime READ runtime NOTIFY runtimeChanged)
    Q_PROPERTY(QVariantList outputs READ outputs NOTIFY outputsChanged)
    Q_PROPERTY(QVariantList hooks READ hooks NOTIFY hooksChanged)
    Q_PROPERTY(QVariantMap vramTest READ vramTest NOTIFY vramTestChanged)
    Q_PROPERTY(QVariantList logs READ logs NOTIFY logsChanged)
    Q_PROPERTY(QString logsError READ logsError NOTIFY logsChanged)
    Q_PROPERTY(QString logPath READ logPath NOTIFY logsChanged)
    Q_PROPERTY(QString logFilter READ logFilter NOTIFY logsChanged)
    Q_PROPERTY(bool logEnvironmentOverride READ logEnvironmentOverride NOTIFY logsChanged)
    Q_PROPERTY(QString outputsError READ outputsError NOTIFY outputsChanged)
    Q_PROPERTY(QString hooksError READ hooksError NOTIFY hooksChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QStringList pending READ pending NOTIFY pendingChanged)
    Q_PROPERTY(bool rotationPaused READ rotationPaused NOTIFY rotationChanged)
    Q_PROPERTY(bool temporaryPause READ temporaryPause NOTIFY rotationChanged)
public:
    explicit FrontendState(DaemonClient *client, QObject *parent = nullptr);
    QVariantMap engineInfo() const { return m_engineInfo.toVariantMap(); }
    QVariantMap runtime() const { return m_runtime.toVariantMap(); }
    QVariantList outputs() const { return m_outputs; }
    QVariantList hooks() const { return m_hooks; }
    QVariantMap vramTest() const { return m_vramTest; }
    QVariantList logs() const { return m_logs; }
    QString logsError() const { return m_logsError; }
    QString logPath() const { return m_logPath; }
    QString logFilter() const { return m_logFilter; }
    bool logEnvironmentOverride() const { return m_logEnvironmentOverride; }
    QString outputsError() const { return m_outputsError; }
    QString hooksError() const { return m_hooksError; }
    QString error() const { return m_error; }
    QStringList pending() const { return m_pending.values(); }
    bool rotationPaused() const { return m_rotationPaused; }
    bool temporaryPause() const { return m_temporaryPause; }
    void enableLibraryEvents();
    Q_INVOKABLE void refreshEngine();
    Q_INVOKABLE void refreshOutputs();
    Q_INVOKABLE void refreshHooks();
    Q_INVOKABLE void reloadHooks();
    Q_INVOKABLE void refreshRotation();
    Q_INVOKABLE void toggleRotation();
    Q_INVOKABLE void testVram(const QString &backend = QString(), const QString &command = QString());
    Q_INVOKABLE void exportLogs();
    bool exportLogsTo(const QString &path);
    Q_INVOKABLE void openLogsDirectory();
    Q_INVOKABLE void refreshLogs(const QString &level = QString());
    Q_INVOKABLE void openHooksFile();
    Q_INVOKABLE void openPath(const QString &path);
signals:
    void engineInfoChanged();
    void runtimeChanged();
    void outputsChanged();
    void hooksChanged();
    void vramTestChanged();
    void logsChanged();
    void pendingChanged();
    void errorChanged();
    void rotationChanged();
    void businessRefreshRequested();
    void operationError(const QString &message);
private:
    void updateSubscription();
    void applyInfo(const QJsonObject &info);
    void applyRuntime(const QJsonObject &runtime, bool snapshot);
    void request(const QString &command, const QJsonObject &fields,
                 std::function<void(const Daemon::Response &)> callback);
    DaemonClient *m_client;
    quint64 m_epoch = 0;
    quint64 m_revision = 0;
    bool m_haveRuntime = false;
    bool m_engineSubscribed = false, m_librarySubscribed = false;
    QJsonObject m_engineInfo, m_runtime;
    QVariantList m_outputs, m_hooks, m_logs;
    QVariantMap m_vramTest;
    QString m_error, m_outputsError, m_hooksError;
    QString m_logsError, m_logPath, m_logFilter, m_requestedLogLevel;
    bool m_logRefreshAgain = false;
    bool m_logEnvironmentOverride = false;
    QSet<QString> m_pending;
    bool m_rotationPaused = false, m_temporaryPause = false;
};
