# Changelog - v1.1.0

**发布日期**: 2026-02-07

## 概述

界面体验与稳定性优化。新增主色调切换、单实例保护，修复路径打开与托盘逻辑，优化壁纸库刷新流程。

---

## ✨ 新特性

### 设置页增强

- **主色调选择** — 新增蓝色 (#5BCEFA) / 粉色 (#F5A9B8) 主色调切换，影响强调色、导航栏选中、按钮等所有 accent 元素
- **目录路径展示** — 设置页新增「配置目录」(~/.config/lianwall) 和「缓存目录」(~/.cache/lianwall) 显示，支持一键打开
- **自定义下拉选择器** — 语言切换从默认 Qt ComboBox 替换为自定义 `StyledSelect` 组件（基于 Popup），跟随主题配色

### 单实例保护

- **QLocalServer 单实例检测** — 防止多开，第二个实例自动激活已有窗口并退出
- **轻量化启动** — Application 构造拆分为构造器 + `init()`，第二个实例仅创建 QApplication 检测后即退出，不加载 QML/托盘等重量级组件

### 构建系统

- **build.sh 构建脚本** — 自动从 CMakeLists.txt 提取版本号，产物归档到 `build/<version>/`，生成 SHA256 校验和，`--deploy` 参数一键部署到 `~/.local/bin/`

---

## 🐛 修复

### 路径打开

- **openDirectory 空路径保护** — 路径为空时直接返回，不再触发无效的 `QDesktopServices::openUrl`
- **openDirectory 自动创建** — 路径不存在时自动 `mkpath` 创建目录后再打开，避免文件管理器报错

### 系统托盘

- **重启 Daemon 替代重载配置** — 托盘菜单「重载配置」改为「🔄 重启 Daemon」，通过 `shutdown()` + 自动拉起实现完整重启（不依赖 systemd）
- **快速重启** — shutdown 回调中立即重置 backoff 并 200ms 后启动新 daemon，整体重启时间从数秒缩短到 ~1-2 秒

---

## ⚡ 优化

### 壁纸库

- **刷新按钮合并操作** — Library 页面刷新按钮由单纯 `WallpaperModel.load()` 改为先 `daemonReloadConfig()` 再 `load()`，一键完成配置重载+列表刷新
- **按钮文案优化** — 刷新按钮从纯图标「🔄」改为「🔄 刷新」，语义更清晰

### 主题系统

- **动态 accent 颜色** — Theme.qml 的 `accent`/`accentHover`/`accentPressed`/`navBarSelected`/`borderFocus` 从硬编码蓝色改为根据 `accentScheme` 属性动态切换
- **ConfigManager 持久化** — 新增 `accentColor` 属性，通过 QSettings (`app/accentColor`) 持久化主色调偏好

---

## 📁 变更文件

| 文件 | 变更 |
|------|------|
| `src/main.cpp` | QLocalServer 单实例检测 |
| `src/Application.h/cpp` | `init()` 延迟初始化、托盘重启逻辑 |
| `src/ConfigManager.h/cpp` | `accentColor` 属性、`configDirPath()`/`cacheDirPath()` 方法、`openDirectory` 修复 |
| `src/DaemonClient.h/cpp` | `resetStartAttempt()`/`resetBackoff()`/`tryStartDaemon()` 公开 |
| `src/Constants.h` | 版本号 → 1.1.0 |
| `CMakeLists.txt` | 版本号 → 1.1.0 |
| `qml/Theme.qml` | `accentScheme` 属性、动态 accent 颜色 |
| `qml/main.qml` | accentScheme 绑定 |
| `qml/pages/SettingsPage.qml` | 主色调选择器、目录路径、StyledSelect 组件 |
| `qml/pages/LibraryPage.qml` | 刷新按钮合并 reloadConfig |
| `build.sh` | 新增构建脚本 |
