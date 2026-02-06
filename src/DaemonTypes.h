#pragma once

/// @file DaemonTypes.h
/// @brief Daemon 通信协议类型定义
///
/// 覆盖 DAEMON-API.md (Protocol V2) 全部 Query / Command / Subscribe / Event 类型。
/// 仅做数据定义，不含网络逻辑。

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QVector>
#include <QVariant>
#include <optional>

namespace Daemon {

// ============================================================================
// 枚举
// ============================================================================

/// 运行模式
enum class WallMode {
    Video,
    Image,
};

inline QString wallModeToString(WallMode m) {
    return m == WallMode::Video ? QStringLiteral("Video") : QStringLiteral("Image");
}

inline WallMode wallModeFromString(const QString &s) {
    return s == QStringLiteral("Image") ? WallMode::Image : WallMode::Video;
}

/// 壁纸切换触发原因
enum class WallpaperTrigger {
    Scheduled,
    ManualNext,
    ManualPrev,
    ManualSet,
    ModeSwitch,
    VramDowngrade,
    VramUpgrade,
    TimePointRefresh,
    DaemonStart,
};

inline WallpaperTrigger triggerFromString(const QString &s) {
    if (s == QLatin1String("scheduled"))           return WallpaperTrigger::Scheduled;
    if (s == QLatin1String("manual_next"))          return WallpaperTrigger::ManualNext;
    if (s == QLatin1String("manual_prev"))          return WallpaperTrigger::ManualPrev;
    if (s == QLatin1String("manual_set"))           return WallpaperTrigger::ManualSet;
    if (s == QLatin1String("mode_switch"))          return WallpaperTrigger::ModeSwitch;
    if (s == QLatin1String("vram_downgrade"))       return WallpaperTrigger::VramDowngrade;
    if (s == QLatin1String("vram_upgrade"))         return WallpaperTrigger::VramUpgrade;
    if (s == QLatin1String("time_point_refresh"))   return WallpaperTrigger::TimePointRefresh;
    if (s == QLatin1String("daemon_start"))         return WallpaperTrigger::DaemonStart;
    return WallpaperTrigger::Scheduled;
}

inline QString triggerToString(WallpaperTrigger t) {
    switch (t) {
        case WallpaperTrigger::Scheduled:         return QStringLiteral("scheduled");
        case WallpaperTrigger::ManualNext:         return QStringLiteral("manual_next");
        case WallpaperTrigger::ManualPrev:         return QStringLiteral("manual_prev");
        case WallpaperTrigger::ManualSet:          return QStringLiteral("manual_set");
        case WallpaperTrigger::ModeSwitch:         return QStringLiteral("mode_switch");
        case WallpaperTrigger::VramDowngrade:      return QStringLiteral("vram_downgrade");
        case WallpaperTrigger::VramUpgrade:        return QStringLiteral("vram_upgrade");
        case WallpaperTrigger::TimePointRefresh:   return QStringLiteral("time_point_refresh");
        case WallpaperTrigger::DaemonStart:        return QStringLiteral("daemon_start");
    }
    return QStringLiteral("scheduled");
}

/// 可订阅的事件类型
enum class EventType {
    WallpaperChanged,
    StatusChanged,
    ConfigChanged,
    SpaceUpdated,
    VramChanged,
    TimePointReached,
    ScanProgress,
    Error,
    All,
};

inline QString eventTypeToString(EventType e) {
    switch (e) {
        case EventType::WallpaperChanged:   return QStringLiteral("wallpaper_changed");
        case EventType::StatusChanged:      return QStringLiteral("status_changed");
        case EventType::ConfigChanged:      return QStringLiteral("config_changed");
        case EventType::SpaceUpdated:       return QStringLiteral("space_updated");
        case EventType::VramChanged:        return QStringLiteral("vram_changed");
        case EventType::TimePointReached:   return QStringLiteral("time_point_reached");
        case EventType::ScanProgress:       return QStringLiteral("scan_progress");
        case EventType::Error:              return QStringLiteral("error");
        case EventType::All:                return QStringLiteral("all");
    }
    return QStringLiteral("all");
}

inline EventType eventTypeFromString(const QString &s) {
    if (s == QLatin1String("wallpaper_changed"))   return EventType::WallpaperChanged;
    if (s == QLatin1String("status_changed"))      return EventType::StatusChanged;
    if (s == QLatin1String("config_changed"))      return EventType::ConfigChanged;
    if (s == QLatin1String("space_updated"))       return EventType::SpaceUpdated;
    if (s == QLatin1String("vram_changed"))        return EventType::VramChanged;
    if (s == QLatin1String("time_point_reached"))  return EventType::TimePointReached;
    if (s == QLatin1String("scan_progress"))       return EventType::ScanProgress;
    if (s == QLatin1String("error"))               return EventType::Error;
    return EventType::All;
}

/// 向量空间更新原因
enum class SpaceUpdateReason {
    LockChanged,
    Rescanned,
    TimePointRefresh,
    ConfigChanged,
};

inline SpaceUpdateReason spaceUpdateReasonFromString(const QString &s) {
    if (s == QLatin1String("lock_changed"))        return SpaceUpdateReason::LockChanged;
    if (s == QLatin1String("rescanned"))           return SpaceUpdateReason::Rescanned;
    if (s == QLatin1String("time_point_refresh"))  return SpaceUpdateReason::TimePointRefresh;
    if (s == QLatin1String("config_changed"))      return SpaceUpdateReason::ConfigChanged;
    return SpaceUpdateReason::Rescanned;
}

/// 显存动作
enum class VramAction {
    Downgrade,
    Upgrade,
};

inline VramAction vramActionFromString(const QString &s) {
    return s == QLatin1String("upgrade") ? VramAction::Upgrade : VramAction::Downgrade;
}

/// 错误码
enum class ErrorCode {
    Unknown,
    InvalidRequest,
    NotFound,
    EngineError,
    ConfigError,
    PermissionDenied,
    Timeout,
    EmptySpace,
    NoHistory,
    AlreadySubscribed,
    NotSubscribed,
    InternalError,
};

inline ErrorCode errorCodeFromString(const QString &s) {
    if (s == QLatin1String("invalid_request"))      return ErrorCode::InvalidRequest;
    if (s == QLatin1String("not_found"))            return ErrorCode::NotFound;
    if (s == QLatin1String("engine_error"))         return ErrorCode::EngineError;
    if (s == QLatin1String("config_error"))         return ErrorCode::ConfigError;
    if (s == QLatin1String("permission_denied"))    return ErrorCode::PermissionDenied;
    if (s == QLatin1String("timeout"))              return ErrorCode::Timeout;
    if (s == QLatin1String("empty_space"))          return ErrorCode::EmptySpace;
    if (s == QLatin1String("no_history"))           return ErrorCode::NoHistory;
    if (s == QLatin1String("already_subscribed"))   return ErrorCode::AlreadySubscribed;
    if (s == QLatin1String("not_subscribed"))       return ErrorCode::NotSubscribed;
    if (s == QLatin1String("internal_error"))       return ErrorCode::InternalError;
    return ErrorCode::Unknown;
}

// ============================================================================
// 响应数据结构
// ============================================================================

/// Pong 响应
struct PongPayload {
    quint64 uptimeSecs = 0;
    quint32 protocolVersion = 0;

    static PongPayload fromJson(const QJsonObject &obj) {
        return {
            static_cast<quint64>(obj[QLatin1String("uptime_secs")].toDouble()),
            static_cast<quint32>(obj[QLatin1String("protocol_version")].toInt()),
        };
    }
};

/// GetStatus 响应
struct StatusPayload {
    WallMode mode = WallMode::Video;
    QString current;              // nullable
    QString currentFilename;      // nullable
    QString engine;
    int totalWallpapers = 0;
    int lockedCount = 0;
    int availableCount = 0;
    int scannedCount = 0;
    quint64 vramUsedMb = 0;
    quint64 vramTotalMb = 0;
    bool vramDegraded = false;
    quint64 uptimeSecs = 0;
    quint32 protocolVersion = 0;
    QString nextTimePoint;        // nullable
    int timePointsCount = 0;
    int nextSwitchSecs = -1;      // -1 = null

    static StatusPayload fromJson(const QJsonObject &obj) {
        StatusPayload p;
        p.mode              = wallModeFromString(obj[QLatin1String("mode")].toString());
        p.current           = obj[QLatin1String("current")].toString();
        p.currentFilename   = obj[QLatin1String("current_filename")].toString();
        p.engine            = obj[QLatin1String("engine")].toString();
        p.totalWallpapers   = obj[QLatin1String("total_wallpapers")].toInt();
        p.lockedCount       = obj[QLatin1String("locked_count")].toInt();
        p.availableCount    = obj[QLatin1String("available_count")].toInt();
        p.scannedCount      = obj[QLatin1String("scanned_count")].toInt();
        p.vramUsedMb        = static_cast<quint64>(obj[QLatin1String("vram_used_mb")].toDouble());
        p.vramTotalMb       = static_cast<quint64>(obj[QLatin1String("vram_total_mb")].toDouble());
        p.vramDegraded      = obj[QLatin1String("vram_degraded")].toBool();
        p.uptimeSecs        = static_cast<quint64>(obj[QLatin1String("uptime_secs")].toDouble());
        p.protocolVersion   = static_cast<quint32>(obj[QLatin1String("protocol_version")].toInt());
        p.nextTimePoint     = obj[QLatin1String("next_time_point")].toString();
        p.timePointsCount   = obj[QLatin1String("time_points_count")].toInt();
        p.nextSwitchSecs    = obj[QLatin1String("next_switch_secs")].isNull()
                                  ? -1
                                  : obj[QLatin1String("next_switch_secs")].toInt();
        return p;
    }
};

/// WallpaperPoint（向量空间中的壁纸）
struct WallpaperPoint {
    int index = 0;
    QString filename;
    QString path;
    double angle = 0.0;
    bool locked = false;
    bool inCooldown = false;
    bool isCurrent = false;

    static WallpaperPoint fromJson(const QJsonObject &obj) {
        return {
            obj[QLatin1String("index")].toInt(),
            obj[QLatin1String("filename")].toString(),
            obj[QLatin1String("path")].toString(),
            obj[QLatin1String("angle")].toDouble(),
            obj[QLatin1String("locked")].toBool(),
            obj[QLatin1String("in_cooldown")].toBool(),
            obj[QLatin1String("is_current")].toBool(),
        };
    }
};

/// GetSpace 响应
struct SpacePayload {
    WallMode mode = WallMode::Video;
    QVector<WallpaperPoint> items;
    double pointerAngle = 0.0;
    int cooldownSize = 0;
    int currentIndex = -1;       // -1 = null

    static SpacePayload fromJson(const QJsonObject &obj) {
        SpacePayload p;
        p.mode          = wallModeFromString(obj[QLatin1String("mode")].toString());
        p.pointerAngle  = obj[QLatin1String("pointer_angle")].toDouble();
        p.cooldownSize  = obj[QLatin1String("cooldown_size")].toInt();
        p.currentIndex  = obj[QLatin1String("current_index")].isNull()
                              ? -1
                              : obj[QLatin1String("current_index")].toInt();

        const auto arr = obj[QLatin1String("items")].toArray();
        p.items.reserve(arr.size());
        for (const auto &v : arr)
            p.items.append(WallpaperPoint::fromJson(v.toObject()));
        return p;
    }
};

/// 时间范围信息
struct TimeRangeInfo {
    QString start;
    QString end;
    bool crossesMidnight = false;

    static TimeRangeInfo fromJson(const QJsonObject &obj) {
        return {
            obj[QLatin1String("start")].toString(),
            obj[QLatin1String("end")].toString(),
            obj[QLatin1String("crosses_midnight")].toBool(),
        };
    }
};

/// 壁纸时间分段
struct WallpaperTimeSegment {
    QString filename;
    QString path;
    QVector<TimeRangeInfo> activeRanges;
    bool allDay = false;

    static WallpaperTimeSegment fromJson(const QJsonObject &obj) {
        WallpaperTimeSegment s;
        s.filename = obj[QLatin1String("filename")].toString();
        s.path     = obj[QLatin1String("path")].toString();
        s.allDay   = obj[QLatin1String("all_day")].toBool();
        const auto arr = obj[QLatin1String("active_ranges")].toArray();
        s.activeRanges.reserve(arr.size());
        for (const auto &v : arr)
            s.activeRanges.append(TimeRangeInfo::fromJson(v.toObject()));
        return s;
    }
};

/// 单模式调度信息
struct ModeSchedule {
    int scannedCount = 0;
    int activeCount = 0;
    QStringList timePoints;
    QString nextTimePoint;         // nullable
    QVector<WallpaperTimeSegment> wallpaperSegments;

    static ModeSchedule fromJson(const QJsonObject &obj) {
        ModeSchedule m;
        m.scannedCount  = obj[QLatin1String("scanned_count")].toInt();
        m.activeCount   = obj[QLatin1String("active_count")].toInt();
        m.nextTimePoint = obj[QLatin1String("next_time_point")].toString();
        const auto tpArr = obj[QLatin1String("time_points")].toArray();
        for (const auto &v : tpArr)
            m.timePoints.append(v.toString());
        const auto wsArr = obj[QLatin1String("wallpaper_segments")].toArray();
        m.wallpaperSegments.reserve(wsArr.size());
        for (const auto &v : wsArr)
            m.wallpaperSegments.append(WallpaperTimeSegment::fromJson(v.toObject()));
        return m;
    }
};

/// GetTimeInfo 响应
struct TimeInfoPayload {
    QString currentTime;
    ModeSchedule videoSchedule;
    ModeSchedule imageSchedule;

    static TimeInfoPayload fromJson(const QJsonObject &obj) {
        return {
            obj[QLatin1String("current_time")].toString(),
            ModeSchedule::fromJson(obj[QLatin1String("video_schedule")].toObject()),
            ModeSchedule::fromJson(obj[QLatin1String("image_schedule")].toObject()),
        };
    }
};

/// GetConfig 响应
struct ConfigPayload {
    QString key;                   // nullable ("" = 全部)
    QJsonValue value;              // 全部时为 object，单键时为具体值

    static ConfigPayload fromJson(const QJsonObject &obj) {
        return {
            obj[QLatin1String("key")].toString(),
            obj[QLatin1String("value")],
        };
    }
};

/// Subscribe 响应
struct SubscribedPayload {
    QString sessionId;
    QStringList subscribedEvents;

    static SubscribedPayload fromJson(const QJsonObject &obj) {
        SubscribedPayload p;
        p.sessionId = obj[QLatin1String("session_id")].toString();
        const auto arr = obj[QLatin1String("subscribed_events")].toArray();
        for (const auto &v : arr)
            p.subscribedEvents.append(v.toString());
        return p;
    }
};

/// 错误响应
struct ErrorPayload {
    ErrorCode code = ErrorCode::Unknown;
    QString message;

    static ErrorPayload fromJson(const QJsonObject &obj) {
        return {
            errorCodeFromString(obj[QLatin1String("code")].toString()),
            obj[QLatin1String("message")].toString(),
        };
    }
};

// ============================================================================
// 事件数据结构
// ============================================================================

/// WallpaperChanged 事件数据
struct WallpaperChangedData {
    QString path;
    QString filename;
    WallMode mode = WallMode::Video;
    WallpaperTrigger trigger = WallpaperTrigger::Scheduled;

    static WallpaperChangedData fromJson(const QJsonObject &obj) {
        return {
            obj[QLatin1String("path")].toString(),
            obj[QLatin1String("filename")].toString(),
            wallModeFromString(obj[QLatin1String("mode")].toString()),
            triggerFromString(obj[QLatin1String("trigger")].toString()),
        };
    }
};

/// StatusChanged 事件中的单个变化
struct StatusChange {
    QString field;
    QJsonValue value;

    static StatusChange fromJson(const QJsonObject &obj) {
        return {
            obj[QLatin1String("field")].toString(),
            obj[QLatin1String("value")],
        };
    }
};

/// StatusChanged 事件数据
struct StatusChangedData {
    QVector<StatusChange> changes;

    static StatusChangedData fromJson(const QJsonObject &obj) {
        StatusChangedData d;
        const auto arr = obj[QLatin1String("changes")].toArray();
        d.changes.reserve(arr.size());
        for (const auto &v : arr)
            d.changes.append(StatusChange::fromJson(v.toObject()));
        return d;
    }
};

/// ConfigChanged 事件数据
struct ConfigChangedData {
    QString key;                   // "all" for ReloadConfig
    QJsonValue oldValue;           // null for ReloadConfig
    QJsonValue newValue;           // null for ReloadConfig

    static ConfigChangedData fromJson(const QJsonObject &obj) {
        return {
            obj[QLatin1String("key")].toString(),
            obj[QLatin1String("old_value")],
            obj[QLatin1String("new_value")],
        };
    }
};

/// SpaceUpdated 摘要
struct SpaceSummary {
    int total = 0;
    int available = 0;
    int locked = 0;
    int inCooldown = 0;

    static SpaceSummary fromJson(const QJsonObject &obj) {
        return {
            obj[QLatin1String("total")].toInt(),
            obj[QLatin1String("available")].toInt(),
            obj[QLatin1String("locked")].toInt(),
            obj[QLatin1String("in_cooldown")].toInt(),
        };
    }
};

/// SpaceUpdated 事件数据
struct SpaceUpdatedData {
    WallMode mode = WallMode::Video;
    SpaceUpdateReason reason = SpaceUpdateReason::Rescanned;
    SpaceSummary summary;

    static SpaceUpdatedData fromJson(const QJsonObject &obj) {
        return {
            wallModeFromString(obj[QLatin1String("mode")].toString()),
            spaceUpdateReasonFromString(obj[QLatin1String("reason")].toString()),
            SpaceSummary::fromJson(obj[QLatin1String("summary")].toObject()),
        };
    }
};

/// VramChanged 事件数据
struct VramChangedData {
    VramAction action = VramAction::Downgrade;
    quint64 usedMb = 0;
    quint64 totalMb = 0;
    double freePercent = 0.0;

    static VramChangedData fromJson(const QJsonObject &obj) {
        return {
            vramActionFromString(obj[QLatin1String("action")].toString()),
            static_cast<quint64>(obj[QLatin1String("used_mb")].toDouble()),
            static_cast<quint64>(obj[QLatin1String("total_mb")].toDouble()),
            obj[QLatin1String("free_percent")].toDouble(),
        };
    }
};

/// TimePointReached 事件数据
struct TimePointReachedData {
    QString time;
    QString nextTime;

    static TimePointReachedData fromJson(const QJsonObject &obj) {
        return {
            obj[QLatin1String("time")].toString(),
            obj[QLatin1String("next_time")].toString(),
        };
    }
};

/// ScanProgress 事件数据
struct ScanProgressData {
    WallMode mode = WallMode::Video;
    int dirsScanned = 0;
    int filesFound = 0;
    bool completed = false;

    static ScanProgressData fromJson(const QJsonObject &obj) {
        return {
            wallModeFromString(obj[QLatin1String("mode")].toString()),
            obj[QLatin1String("dirs_scanned")].toInt(),
            obj[QLatin1String("files_found")].toInt(),
            obj[QLatin1String("completed")].toBool(),
        };
    }
};

/// Error 事件数据
struct ErrorData {
    ErrorCode code = ErrorCode::Unknown;
    QString message;
    bool recoverable = true;

    static ErrorData fromJson(const QJsonObject &obj) {
        return {
            errorCodeFromString(obj[QLatin1String("code")].toString()),
            obj[QLatin1String("message")].toString(),
            obj[QLatin1String("recoverable")].toBool(true),
        };
    }
};

/// 统一事件容器（由 DaemonClient 解析后分发）
struct DaemonEvent {
    EventType type = EventType::All;
    QJsonObject rawData;           // 保留原始 JSON 便于扩展

    // 按需解析（调用方知道类型后再解析，避免无用开销）
    WallpaperChangedData   asWallpaperChanged()   const { return WallpaperChangedData::fromJson(rawData); }
    StatusChangedData      asStatusChanged()      const { return StatusChangedData::fromJson(rawData); }
    ConfigChangedData      asConfigChanged()      const { return ConfigChangedData::fromJson(rawData); }
    SpaceUpdatedData       asSpaceUpdated()       const { return SpaceUpdatedData::fromJson(rawData); }
    VramChangedData        asVramChanged()        const { return VramChangedData::fromJson(rawData); }
    TimePointReachedData   asTimePointReached()   const { return TimePointReachedData::fromJson(rawData); }
    ScanProgressData       asScanProgress()       const { return ScanProgressData::fromJson(rawData); }
    ErrorData              asError()              const { return ErrorData::fromJson(rawData); }
};

// ============================================================================
// 请求构造（生成 JSON 行）
// ============================================================================

namespace Request {

inline QByteArray ping() {
    return QJsonDocument(QJsonObject{{QStringLiteral("cmd"), QStringLiteral("Ping")}})
        .toJson(QJsonDocument::Compact) + '\n';
}

inline QByteArray getStatus() {
    return QJsonDocument(QJsonObject{{QStringLiteral("cmd"), QStringLiteral("GetStatus")}})
        .toJson(QJsonDocument::Compact) + '\n';
}

inline QByteArray getSpace(std::optional<WallMode> mode = std::nullopt) {
    QJsonObject obj{{QStringLiteral("cmd"), QStringLiteral("GetSpace")}};
    if (mode.has_value())
        obj[QStringLiteral("mode")] = wallModeToString(*mode);
    return QJsonDocument(obj).toJson(QJsonDocument::Compact) + '\n';
}

inline QByteArray getTimeInfo() {
    return QJsonDocument(QJsonObject{{QStringLiteral("cmd"), QStringLiteral("GetTimeInfo")}})
        .toJson(QJsonDocument::Compact) + '\n';
}

inline QByteArray getConfig(const QString &key = QString()) {
    QJsonObject obj{{QStringLiteral("cmd"), QStringLiteral("GetConfig")}};
    if (!key.isEmpty())
        obj[QStringLiteral("key")] = key;
    return QJsonDocument(obj).toJson(QJsonDocument::Compact) + '\n';
}

inline QByteArray next() {
    return QJsonDocument(QJsonObject{{QStringLiteral("cmd"), QStringLiteral("Next")}})
        .toJson(QJsonDocument::Compact) + '\n';
}

inline QByteArray prev() {
    return QJsonDocument(QJsonObject{{QStringLiteral("cmd"), QStringLiteral("Prev")}})
        .toJson(QJsonDocument::Compact) + '\n';
}

inline QByteArray setWallpaper(const QString &path) {
    return QJsonDocument(QJsonObject{
        {QStringLiteral("cmd"),  QStringLiteral("SetWallpaper")},
        {QStringLiteral("path"), path},
    }).toJson(QJsonDocument::Compact) + '\n';
}

inline QByteArray setMode(WallMode mode) {
    return QJsonDocument(QJsonObject{
        {QStringLiteral("cmd"),  QStringLiteral("SetMode")},
        {QStringLiteral("mode"), wallModeToString(mode)},
    }).toJson(QJsonDocument::Compact) + '\n';
}

inline QByteArray lock(const QString &path) {
    return QJsonDocument(QJsonObject{
        {QStringLiteral("cmd"),  QStringLiteral("Lock")},
        {QStringLiteral("path"), path},
    }).toJson(QJsonDocument::Compact) + '\n';
}

inline QByteArray unlock(const QString &path) {
    return QJsonDocument(QJsonObject{
        {QStringLiteral("cmd"),  QStringLiteral("Unlock")},
        {QStringLiteral("path"), path},
    }).toJson(QJsonDocument::Compact) + '\n';
}

inline QByteArray toggleLock(const QString &path) {
    return QJsonDocument(QJsonObject{
        {QStringLiteral("cmd"),  QStringLiteral("ToggleLock")},
        {QStringLiteral("path"), path},
    }).toJson(QJsonDocument::Compact) + '\n';
}

inline QByteArray setConfig(const QString &key, const QJsonValue &value) {
    return QJsonDocument(QJsonObject{
        {QStringLiteral("cmd"),   QStringLiteral("SetConfig")},
        {QStringLiteral("key"),   key},
        {QStringLiteral("value"), value},
    }).toJson(QJsonDocument::Compact) + '\n';
}

inline QByteArray rescan() {
    return QJsonDocument(QJsonObject{{QStringLiteral("cmd"), QStringLiteral("Rescan")}})
        .toJson(QJsonDocument::Compact) + '\n';
}

inline QByteArray reloadConfig() {
    return QJsonDocument(QJsonObject{{QStringLiteral("cmd"), QStringLiteral("ReloadConfig")}})
        .toJson(QJsonDocument::Compact) + '\n';
}

inline QByteArray shutdown() {
    return QJsonDocument(QJsonObject{{QStringLiteral("cmd"), QStringLiteral("Shutdown")}})
        .toJson(QJsonDocument::Compact) + '\n';
}

inline QByteArray subscribe(const QVector<EventType> &events, bool immediateSync = true) {
    QJsonArray arr;
    for (auto e : events)
        arr.append(eventTypeToString(e));
    return QJsonDocument(QJsonObject{
        {QStringLiteral("cmd"),            QStringLiteral("Subscribe")},
        {QStringLiteral("events"),         arr},
        {QStringLiteral("immediate_sync"), immediateSync},
    }).toJson(QJsonDocument::Compact) + '\n';
}

inline QByteArray unsubscribe() {
    return QJsonDocument(QJsonObject{{QStringLiteral("cmd"), QStringLiteral("Unsubscribe")}})
        .toJson(QJsonDocument::Compact) + '\n';
}

} // namespace Request

// ============================================================================
// 响应解析
// ============================================================================

/// 响应类型
enum class ResponseType {
    Ok,
    Error,
    Pong,
    Status,
    Space,
    TimeInfo,
    Config,
    Subscribed,
    Unsubscribed,
    Event,
    Unknown,
};

inline ResponseType responseTypeFromString(const QString &s) {
    if (s == QLatin1String("Ok"))           return ResponseType::Ok;
    if (s == QLatin1String("Error"))        return ResponseType::Error;
    if (s == QLatin1String("Pong"))         return ResponseType::Pong;
    if (s == QLatin1String("Status"))       return ResponseType::Status;
    if (s == QLatin1String("Space"))        return ResponseType::Space;
    if (s == QLatin1String("TimeInfo"))     return ResponseType::TimeInfo;
    if (s == QLatin1String("Config"))       return ResponseType::Config;
    if (s == QLatin1String("Subscribed"))   return ResponseType::Subscribed;
    if (s == QLatin1String("Unsubscribed")) return ResponseType::Unsubscribed;
    if (s == QLatin1String("Event"))        return ResponseType::Event;
    return ResponseType::Unknown;
}

/// 解析后的响应
struct Response {
    ResponseType type = ResponseType::Unknown;
    QJsonObject payload;           // 原始 payload（按需解析具体结构）

    /// 从一行 JSON 解析
    static std::optional<Response> fromLine(const QByteArray &line) {
        QJsonParseError err;
        auto doc = QJsonDocument::fromJson(line, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject())
            return std::nullopt;

        auto obj = doc.object();
        Response r;
        r.type    = responseTypeFromString(obj[QLatin1String("type")].toString());
        r.payload = obj[QLatin1String("payload")].toObject();
        return r;
    }

    // 便捷解析方法
    PongPayload        asPong()        const { return PongPayload::fromJson(payload); }
    StatusPayload      asStatus()      const { return StatusPayload::fromJson(payload); }
    SpacePayload       asSpace()       const { return SpacePayload::fromJson(payload); }
    TimeInfoPayload    asTimeInfo()    const { return TimeInfoPayload::fromJson(payload); }
    ConfigPayload      asConfig()      const { return ConfigPayload::fromJson(payload); }
    SubscribedPayload  asSubscribed()  const { return SubscribedPayload::fromJson(payload); }
    ErrorPayload       asError()       const { return ErrorPayload::fromJson(payload); }

    /// 从 Event 响应中提取 DaemonEvent
    DaemonEvent asEvent() const {
        DaemonEvent e;
        e.type    = eventTypeFromString(payload[QLatin1String("event")].toString());
        e.rawData = payload[QLatin1String("data")].toObject();
        return e;
    }
};

} // namespace Daemon
