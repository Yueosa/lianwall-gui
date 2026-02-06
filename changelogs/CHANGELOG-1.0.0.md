# Changelog - v1.0.0

**发布日期**: 2026-02-07

## 概述

完全重构 lianwall-gui，从基于 QProcess 调用 CLI 的旧架构迁移到 **QLocalSocket 直连 Daemon** 的异步通信架构。全新 QML 界面，实时响应 Daemon 事件。

---

## ✨ 新特性

### 通信架构 (Phase 1)

- **QLocalSocket 异步通信** — 通过 Unix Domain Socket (`/tmp/lianwall.sock`) 直连 lianwalld，行分隔 JSON 协议 V2
- **DaemonClient** — 完整的请求-响应匹配、事件订阅、指数退避自动重连
- **DaemonState** — 全局状态管理，订阅 `WallpaperChanged`/`ConfigChanged`/`TimeInfo` 事件，实时驱动 UI 更新
- **DaemonTypes** — 类型安全的协议层：`Request`/`Response`/`DaemonEvent`/`StatusPayload` 等
- **Daemon 自动拉起** — GUI 启动时若 daemon 未运行，自动查找并启动 `lianwalld`（搜索顺序：`/usr/bin` → `~/.local/bin` → 同目录 → PATH）

### 应用框架 (Phase 2)

- **Application 类** — 管理 DaemonClient / DaemonState / ConfigManager 生命周期
- **系统托盘** — QSystemTrayIcon 驻留，关闭窗口 = 隐藏到托盘（不退出进程）
- **托盘菜单** — 显示/隐藏、下一张、上一张、重载配置、退出
- **信号处理** — SIGINT/SIGTERM 优雅退出

### QML 界面 (Phase 3-6)

- **主题系统 (Theme.qml)** — MTF 旗帜色主题（蓝 #5BCEFA、粉 #F5A9B8、白 #FFFFFF），支持浅色/深色切换
- **导航栏 (NavBar)** — 4 页导航：仪表盘、壁纸库、设置、关于
- **仪表盘 (DashboardPage)** — 当前壁纸预览 + 状态信息 + 快捷操作（上/下一张、锁定、扫描）+ 倒计时进度条 + 显存监控
- **壁纸库 (LibraryPage)** — 网格视图浏览所有壁纸，搜索筛选（全部/已锁定/未锁定），懒加载
- **壁纸详情 (WallpaperDetailDialog)** — 大图预览 + 文件信息 + 操作（设为当前/锁定/打开目录）
- **缩略图引擎 (ThumbnailProvider)** — ffmpeg 视频帧提取（5s 标记），QImage 缩放，MD5 磁盘缓存 (`~/.cache/lianwall/thumbnails/`)，QSemaphore 并发控制
- **设置页 (SettingsPage)** — 6 个配置分区：路径与模式、动态壁纸引擎、静态壁纸引擎、显存监控、守护进程、界面设置
- **TagEditor 组件** — 标签式数组参数编辑器（mpvpaper_args/mpv_args/swww_args）
- **倒计时条 (CountdownBar)** — 服务端同步 + 本地 1s 递减 + 壁纸切换重置

### 配置管理 (Phase 6)

- **ConfigManager 重写** — 双重职责：Daemon 配置通过 DaemonClient GetConfig/SetConfig 读写，GUI 本地设置通过 QSettings
- **16 个 Q_PROPERTY** — 实时绑定 daemon 配置到 QML
- **ConfigChanged 事件驱动** — daemon 配置变更自动同步到 GUI
- **Systemd 路径显示** — `~/.config/systemd/user/lianwalld.service` 快捷打开

### 国际化 (Phase 7)

- **中文 (zh_CN)** — 97 条翻译，完整覆盖
- **英文 (en)** — 97 条翻译，完整覆盖
- **运行时语言切换** — 设置页可选中文/英文，`retranslate()` 即时生效

---

## 🏗️ 架构变更

| 项目 | 旧架构 | 新架构 |
|------|--------|--------|
| 通信方式 | QProcess 调用 CLI | QLocalSocket 异步直连 |
| 数据驱动 | 定时轮询 CLI 输出 | 事件订阅 (Subscribe) |
| 配置管理 | 读写 config.toml 文件 | GetConfig/SetConfig 协议命令 |
| 状态同步 | 手动刷新 | 实时事件推送 |
| 壁纸预览 | 无 | ffmpeg 缩略图 + 磁盘缓存 |
| Daemon 管理 | 手动启停 | 自动检测 + 拉起 |

---

## 📦 技术栈

- **C++17** / **Qt 6** (Core, Gui, Widgets, Quick, QuickControls2, Network, LinguistTools)
- **QML** 声明式 UI
- **CMake** 构建系统
- **ffmpeg** 视频缩略图提取

---

## 🔗 兼容性

- 要求 lianwall daemon **v5.0.0+**（Socket V2 协议）
- 推荐 lianwall daemon **v5.1.1**
