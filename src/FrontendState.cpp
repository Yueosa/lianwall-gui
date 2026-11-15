#include "FrontendState.h"
#include "DaemonClient.h"
#include "Constants.h"
#include <QDesktopServices>
#include <QUrl>
#include <QPointer>
#include <QTimer>
#include <QFileInfo>
#include <QFileDialog>
#include <QSaveFile>
#include <QStandardPaths>
#include <QDateTime>

FrontendState::FrontendState(DaemonClient *client, QObject *parent)
    : QObject(parent), m_client(client)
{
    connect(client, &DaemonClient::connectionChanged, this, [this](bool connected) {
        ++m_epoch; m_revision = 0; m_haveRuntime = false; m_engineSubscribed = false; m_librarySubscribed = false;
        m_runtime = {}; m_engineInfo = {}; m_outputs.clear(); m_hooks.clear();
        m_logRefreshAgain = false; m_requestedLogLevel.clear();
        m_logs.clear(); m_vramTest.clear(); m_logsError.clear(); m_logPath.clear(); m_logFilter.clear(); m_logEnvironmentOverride = false;
        emit logsChanged(); emit vramTestChanged();
        m_pending.clear(); m_rotationPaused = false; m_temporaryPause = false;
        emit runtimeChanged(); emit engineInfoChanged(); emit outputsChanged(); emit hooksChanged();
        emit pendingChanged(); emit rotationChanged();
        if (connected) { refreshEngine(); refreshHooks(); refreshRotation(); }
    });
    connect(client, &DaemonClient::initialEngineInfoReceived, this, &FrontendState::applyInfo);
    connect(client, &DaemonClient::eventReceived, this, [this](const Daemon::DaemonEvent &event) {
        if (event.type == Daemon::EventType::EngineChanged)
            applyRuntime(event.rawData.value(QStringLiteral("runtime")).toObject(), false);
        if (event.type == Daemon::EventType::WallpaperChanged
            || event.type == Daemon::EventType::StatusChanged
            || event.type == Daemon::EventType::ConfigChanged)
            refreshRotation();
    });
}

void FrontendState::request(const QString &command, const QJsonObject &fields,
                            std::function<void(const Daemon::Response &)> callback)
{
    if (!m_client->isConnected() || m_pending.contains(command)) return;
    m_pending.insert(command); emit pendingChanged();
    const quint64 epoch = m_epoch;
    const QPointer<FrontendState> guard(this);
    m_client->query(command, fields, [guard, epoch, command, callback](const Daemon::Response &response) {
        if (!guard || epoch != guard->m_epoch) return;
        guard->m_pending.remove(command); emit guard->pendingChanged();
        if (!guard) return;
        if (response.type == Daemon::ResponseType::Error) {
            guard->m_error = response.asError().message;
            emit guard->errorChanged();
        }
        if (guard) callback(response);
    });
}

void FrontendState::applyRuntime(const QJsonObject &runtime, bool snapshot)
{
    if (runtime.isEmpty()) return;
    const quint64 revision = static_cast<quint64>(runtime.value(QStringLiteral("revision")).toDouble());
    if (m_haveRuntime && (revision < m_revision || (!snapshot && revision == m_revision))) return;
    const bool newlyFailed = runtime.value(QStringLiteral("phase")) == QLatin1String("failed")
        && m_runtime.value(QStringLiteral("phase")) != QLatin1String("failed");
    m_haveRuntime = true; m_revision = revision; m_runtime = runtime;
    emit runtimeChanged();
    if (runtime.value(QStringLiteral("phase")) != QLatin1String("preparing")) emit businessRefreshRequested();
    if (newlyFailed) emit operationError(runtime.value(QStringLiteral("last_error")).toString());
}

void FrontendState::applyInfo(const QJsonObject &info)
{
    m_engineInfo = info;
    applyRuntime(info.value(QStringLiteral("native")).toObject(), true);
    emit engineInfoChanged();
}

void FrontendState::refreshEngine()
{
    request(QStringLiteral("GetEngineInfo"), {}, [this](const Daemon::Response &response) {
        if (response.type != Daemon::ResponseType::EngineInfo) return;
        applyInfo(response.payload);
        if (!m_engineSubscribed) {
            m_engineSubscribed = true;
            updateSubscription();
        }
    });
}

void FrontendState::enableLibraryEvents()
{
    if (m_librarySubscribed || !m_client->isConnected()) return;
    m_librarySubscribed = true;
    updateSubscription();
}

void FrontendState::updateSubscription()
{
    QList<Daemon::EventType> events{Daemon::EventType::All};
    if (m_engineSubscribed) events.append(Daemon::EventType::EngineChanged);
    if (m_librarySubscribed) events.append(Daemon::EventType::LibraryChanged);
    const QPointer<FrontendState> guard(this);
    const quint64 epoch = m_epoch;
    m_client->subscribe(events, true, [guard, epoch](const Daemon::Response &response) {
        if (!guard || epoch != guard->m_epoch) return;
        if (response.type == Daemon::ResponseType::Error) {
            guard->m_engineSubscribed = false; guard->m_librarySubscribed = false;
            emit guard->operationError(response.asError().message);
        }
    });
}

void FrontendState::refreshOutputs()
{
    request(QStringLiteral("GetOutputs"), {}, [this](const Daemon::Response &response) {
        m_outputs.clear(); m_outputsError.clear();
        if (response.type == Daemon::ResponseType::Outputs) {
            m_outputs = response.payload.value(QStringLiteral("outputs")).toArray().toVariantList();
            m_outputsError = response.payload.value(QStringLiteral("error")).toString();
        } else m_outputsError = response.asError().message;
        emit outputsChanged();
    });
}

void FrontendState::refreshHooks()
{
    request(QStringLiteral("ListHooks"), {}, [this](const Daemon::Response &response) {
        m_hooks.clear(); m_hooksError.clear();
        if (response.type == Daemon::ResponseType::HookList)
            m_hooks = response.rawPayload.toArray().toVariantList();
        else m_hooksError = response.asError().message;
        emit hooksChanged();
    });
}

void FrontendState::reloadHooks()
{
    request(QStringLiteral("ReloadHooks"), {}, [this](const Daemon::Response &response) {
        if (response.type == Daemon::ResponseType::Ok) refreshHooks();
        else emit operationError(response.asError().message);
    });
}

void FrontendState::refreshRotation()
{
    request(QStringLiteral("GetRotationStatus"), {}, [this](const Daemon::Response &response) {
        if (response.type != Daemon::ResponseType::RotationStatus) return;
        m_rotationPaused = response.payload.value(QStringLiteral("paused")).toBool();
        m_temporaryPause = response.payload.value(QStringLiteral("saved_interval")).isDouble();
        emit rotationChanged();
    });
}

void FrontendState::toggleRotation()
{
    if (m_rotationPaused && !m_temporaryPause) {
        emit operationError(tr("自动轮换已由配置关闭，请保存一个正间隔以恢复。"));
        return;
    }
    request(m_rotationPaused ? QStringLiteral("ResumeRotation") : QStringLiteral("PauseRotation"), {},
        [this](const Daemon::Response &response) {
            if (response.type == Daemon::ResponseType::Error) emit operationError(response.asError().message);
            refreshRotation();
        });
}

void FrontendState::testVram(const QString &backend, const QString &command)
{
    QJsonObject fields;
    if (!backend.isEmpty()) fields.insert(QStringLiteral("backend"), backend);
    if (backend == QLatin1String("custom")) fields.insert(QStringLiteral("command"), command);
    m_vramTest.clear(); emit vramTestChanged();
    request(QStringLiteral("TestVram"), fields, [this](const Daemon::Response &response) {
        m_vramTest = response.type == Daemon::ResponseType::VramTest
            ? response.payload.toVariantMap()
            : QVariantMap{{QStringLiteral("error"), response.asError().message}};
        emit vramTestChanged();
    });
}

void FrontendState::refreshLogs(const QString &level)
{
    m_requestedLogLevel = level;
    if (m_pending.contains(QStringLiteral("GetLogs"))) { m_logRefreshAgain = true; return; }
    m_logRefreshAgain = false;
    request(QStringLiteral("GetLogs"), {{QStringLiteral("level"), level}, {QStringLiteral("limit"), 200}},
        [this](const Daemon::Response &response) {
            m_logsError.clear();
            if (response.type == Daemon::ResponseType::Logs) {
                m_logs = response.payload.value(QStringLiteral("entries")).toArray().toVariantList();
                m_logPath = response.payload.value(QStringLiteral("path")).toString();
                m_logFilter = response.payload.value(QStringLiteral("active_filter")).toString();
                m_logEnvironmentOverride = response.payload.value(QStringLiteral("environment_override")).toBool();
                m_logsError = response.payload.value(QStringLiteral("persistence_error")).toString();
                const auto dropped = response.payload.value(QStringLiteral("dropped_disk_entries")).toInteger();
                if (dropped > 0) m_logsError += tr(" 文件日志已丢弃 %1 条，当前视图来自内存记录。").arg(dropped);
            } else {
                // Keep the last valid snapshot visible and label the failed refresh.
                m_logsError = response.asError().message;
            }
            emit logsChanged();
            if (m_logRefreshAgain) refreshLogs(m_requestedLogLevel);
        });
}

void FrontendState::exportLogs()
{
    const auto directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const auto filename = QStringLiteral("lianwall-daemon-%1.log").arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")));
    const auto path = QFileDialog::getSaveFileName(nullptr, tr("导出当前日志"), directory + '/' + filename, tr("日志文件 (*.log);;所有文件 (*)"));
    if (!path.isEmpty()) exportLogsTo(path);
}

bool FrontendState::exportLogsTo(const QString &path)
{
    if (m_logs.isEmpty()) { emit operationError(tr("暂无可导出的日志。")); return false; }
    QByteArray bytes;
    for (const auto &value : m_logs) {
        const auto entry = value.toMap();
        bytes += (entry.value(QStringLiteral("timestamp")).toString() + ' ' + entry.value(QStringLiteral("line")).toString() + '\n').toUtf8();
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || !file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner)
        || file.write(bytes) != bytes.size() || !file.commit()) {
        emit operationError(tr("导出失败：%1").arg(file.errorString())); return false;
    }
    return true;
}

void FrontendState::openLogsDirectory()
{
    if (!m_logPath.isEmpty()) openPath(QFileInfo(m_logPath).absolutePath());
}

void FrontendState::openPath(const QString &path)
{
    if (!path.isEmpty()) QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void FrontendState::openHooksFile()
{
    const auto file = LianwallGui::Paths::configDir() + QStringLiteral("/hooks.toml");
    openPath(QFileInfo::exists(file) ? file : LianwallGui::Paths::configDir());
}
