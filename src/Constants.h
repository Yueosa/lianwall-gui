#pragma once

#include <QString>
#include <QPair>
#include <QDir>
#include <QFile>
#include <QCoreApplication>

namespace LianwallGui {

// ============================================================================
// 版本信息
// ============================================================================
constexpr const char* APP_NAME = "LianWall";
constexpr const char* APP_VERSION = "1.0.0";
constexpr const char* APP_AUTHOR = "Lian";
constexpr const char* APP_GITHUB_GUI = "https://github.com/Yueosa/lianwall-gui";
constexpr const char* APP_GITHUB_DAEMON = "https://github.com/Yueosa/lianwall";
constexpr const char* APP_DESCRIPTION = "Graphical client for lianwalld — the LianWall wallpaper daemon";

// ============================================================================
// Socket 协议 (v2: 行分隔 JSON)
// ============================================================================
namespace Protocol {
    constexpr const char* DEFAULT_SOCKET_PATH = "/tmp/lianwall.sock";
    constexpr quint32 PROTOCOL_VERSION = 2;
    constexpr qint64 MAX_MESSAGE_SIZE = 1024 * 1024;  // 1 MB
    constexpr int CONNECT_TIMEOUT_MS = 1000;
    constexpr int READ_TIMEOUT_MS = 5000;
}

// ============================================================================
// 文件路径
// ============================================================================
namespace Paths {
    inline QString configDir() {
        return QDir::homePath() + "/.config/lianwall";
    }
    
    inline QString configFile() {
        return configDir() + "/config.toml";
    }
    
    inline QString cacheDir() {
        return QDir::homePath() + "/.cache/lianwall";
    }
    
    inline QString thumbnailDir() {
        return cacheDir() + "/thumbnails";
    }
    
    inline QString weightsFile() {
        return cacheDir() + "/weights.json";
    }
    
    inline QString systemdServicePath() {
        return QDir::homePath() + "/.config/systemd/user/lianwall-gui.service";
    }
    
    /// 查找 lianwalld 可执行文件
    /// 搜索顺序：
    ///   1. /usr/bin/lianwalld          — AUR / pacman 系统安装
    ///   2. ~/.local/bin/lianwalld      — 用户本地安装
    ///   3. <自身所在目录>/lianwalld     — 开发 / 便携场景
    /// 若均未找到，回退到裸名 "lianwalld"（依赖 PATH）
    inline QString findLianwalld() {
        // 1. 系统路径
        const QString sys = QStringLiteral("/usr/bin/lianwalld");
        if (QFile::exists(sys))
            return sys;

        // 2. 用户本地路径
        const QString local = QDir::homePath() + QStringLiteral("/.local/bin/lianwalld");
        if (QFile::exists(local))
            return local;

        // 3. 与 GUI 同目录
        const QString sibling = QCoreApplication::applicationDirPath() + QStringLiteral("/lianwalld");
        if (QFile::exists(sibling))
            return sibling;

        // 4. 回退裸名，交给 PATH 解析
        return QStringLiteral("lianwalld");
    }
}

// ============================================================================
// 缩略图 - 多分辨率支持
// ============================================================================
namespace Thumbnail {
    // 分辨率档位
    enum class Quality {
        High,    // 1920x1080 - 大屏/4K 显示器预览
        Medium,  // 1280x720  - 常规显示器预览
        Low,     // 640x360   - 省流模式预览
        Tiny     // 320x180   - Library/Timeline/Status 列表
    };
    
    // 获取分辨率尺寸
    inline QPair<int, int> getSize(Quality q) {
        switch (q) {
            case Quality::High:   return {1920, 1080};
            case Quality::Medium: return {1280, 720};
            case Quality::Low:    return {640, 360};
            case Quality::Tiny:   return {320, 180};
            default:              return {1280, 720};
        }
    }
    
    // 获取分辨率后缀
    inline QString getSuffix(Quality q) {
        switch (q) {
            case Quality::High:   return "_1080p";
            case Quality::Medium: return "_720p";
            case Quality::Low:    return "_360p";
            case Quality::Tiny:   return "_180p";
            default:              return "_720p";
        }
    }
    
    // 从字符串解析
    inline Quality fromString(const QString &s) {
        if (s == "high" || s == "1080p") return Quality::High;
        if (s == "medium" || s == "720p") return Quality::Medium;
        if (s == "low" || s == "360p") return Quality::Low;
        if (s == "tiny" || s == "180p") return Quality::Tiny;
        return Quality::Medium;
    }
    
    // 转为字符串
    inline QString toString(Quality q) {
        switch (q) {
            case Quality::High:   return "high";
            case Quality::Medium: return "medium";
            case Quality::Low:    return "low";
            case Quality::Tiny:   return "tiny";
            default:              return "medium";
        }
    }
    
    constexpr int SEEK_SECONDS = 5;       // 从第 5 秒截取
    constexpr const char* FORMAT = "jpg";
    constexpr int QUALITY = 85;
}

// ============================================================================
// UI 常量
// ============================================================================
namespace UI {
    constexpr int RECONNECT_DELAY_MS = 1000;       // 重连延迟
    constexpr int MAX_HISTORY_SIZE = 100;          // 历史栈最大大小
    constexpr int COUNTDOWN_WARNING_SECS = 60;     // 倒计时警告阈值（秒）
    constexpr int DAEMON_TIMEOUT_SECS = 5;         // daemon 超时判定（秒）
}

}  // namespace LianwallGui
