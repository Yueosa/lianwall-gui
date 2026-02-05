#pragma once

#include <QString>
#include <QDir>
#include <QCoreApplication>

namespace LianwallGui {

// ============================================================================
// 版本信息
// ============================================================================
constexpr const char* APP_NAME = "LianWall";
constexpr const char* APP_VERSION = "4.0.0";
constexpr const char* APP_AUTHOR = "Lian";
constexpr const char* APP_GITHUB = "https://github.com/Yueosa/lianwall";
constexpr const char* APP_DESCRIPTION = "Selects wallpapers using the golden angle algorithm, sprinkled with Lian's magic";

// ============================================================================
// Socket 协议
// ============================================================================
namespace Protocol {
    constexpr const char* DEFAULT_SOCKET_PATH = "/tmp/lianwall.sock";
    constexpr quint32 PROTOCOL_VERSION = 1;
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
    
    // 内嵌的 lianwall 二进制文件路径
    inline QString embeddedLianwall() {
        // 优先查找与可执行文件同目录的 lianwall
        QString appDir = QCoreApplication::applicationDirPath();
        QString embedded = appDir + "/lianwall";
        if (QFile::exists(embedded)) {
            return embedded;
        }
        
        // 然后查找安装目录
        QString installed = "/usr/lib/lianwall-gui/lianwall";
        if (QFile::exists(installed)) {
            return installed;
        }
        
        // 最后回退到系统 PATH
        return "lianwall";
    }
}

// ============================================================================
// 缩略图
// ============================================================================
namespace Thumbnail {
    constexpr int WIDTH = 320;
    constexpr int HEIGHT = 180;
    constexpr const char* FORMAT = "jpg";
    constexpr int QUALITY = 85;
}

// ============================================================================
// UI 常量
// ============================================================================
namespace UI {
    constexpr int STATUS_POLL_INTERVAL_MS = 5000;  // 状态轮询间隔
    constexpr int RECONNECT_DELAY_MS = 1000;       // 重连延迟
    constexpr int MAX_HISTORY_SIZE = 100;          // 历史栈最大大小
}

}  // namespace LianwallGui
