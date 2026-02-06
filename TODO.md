# LianWall GUI 重写 TODO

> 目标：使用 QLocalSocket 直连 daemon，替代 QProcess 调用 CLI 的旧架构  
> 原则：零轮询、不阻塞、限制并发资源操作  
> 仓库：GUI → [Yueosa/lianwall-gui](https://github.com/Yueosa/lianwall-gui) | Daemon → [Yueosa/lianwall](https://github.com/Yueosa/lianwall)

---

## Phase 1 — 通信基础层

- [ ] **1.1 DaemonTypes.h** — 协议类型定义
  > 定义所有 daemon 通信的 C++ 类型：请求构造函数、响应结构体、事件枚举  
  > 覆盖 DAEMON-API.md 全部 Query/Command/Subscribe/Event

- [ ] **1.2 DaemonClient** — 异步 Socket 通信 (`DaemonClient.h/.cpp`)
  > 基于 `QLocalSocket`，信号驱动，行分隔 JSON 协议  
  > 自动重连（指数退避）、请求-响应匹配、事件分发  
  > 公开信号：`connected()`, `disconnected()`, `eventReceived(DaemonEvent)`

- [ ] **1.3 DaemonState** — 响应式状态聚合 (`DaemonState.h/.cpp`)
  > 持有从 daemon 获取的所有数据的 Q_PROPERTY  
  > 监听 DaemonClient 事件自动更新（零轮询）  
  > 暴露给 QML 的只读属性：mode, current, nextSwitchSecs, vram, space 等

- [ ] **1.4 Constants.h 更新 + CMakeLists.txt**
  > 版本 → 5.1.1，新增 GUI GitHub 地址 `Yueosa/lianwall-gui`  
  > CMakeLists.txt 添加新源文件、Network 依赖（如需要）

---

## Phase 2 — 应用框架

- [ ] **2.1 Application 重写** (`Application.h/.cpp`)
  > 管理 DaemonClient + DaemonState 生命周期  
  > 系统托盘 (`QSystemTrayIcon`)：图标 `lianwall.svg`  
  > 托盘菜单：显示/隐藏窗口 | 下一张 | 上一张 | 重载 (`ReloadConfig`) | 退出  
  > 默认静默启动（只有托盘，不弹窗口）  
  > 关闭窗口 = 隐藏到托盘（不退出进程）

- [ ] **2.2 main.cpp 更新**
  > 注册 DaemonState 为 QML 单例上下文属性  
  > 设置 `QGuiApplication::setQuitOnLastWindowClosed(false)`

---

## Phase 3 — QML 骨架

- [ ] **3.1 main.qml 重写** — 主窗口 + 侧边导航栏
  > 左侧 NavBar（图标导航）：Dashboard / Library / Settings / About  
  > 右侧 StackLayout 切换页面  
  > 顶部状态指示器：daemon 连接状态（绿灯/红灯）

- [ ] **3.2 NavBar.qml** — 侧边导航组件
  > 4 个图标按钮，当前页高亮（使用 Theme 颜色）  
  > 底部显示 daemon 连接状态圆点

- [ ] **3.3 AboutPage.qml**
  > 居中显示 `lianwall.svg` Logo  
  > 应用名 "LianWall"，版本号  
  > 两个 GitHub 链接按钮：GUI 仓库 / Daemon 仓库  
  > 作者：Lian  
  > 底部声明：主题色来自跨性别骄傲旗配色，作者并非 MTF，纯粹因为喜欢这些颜色

---

## Phase 4 — Dashboard 仪表盘

- [ ] **4.1 DashboardPage.qml**
  > 当前壁纸静态预览（图片直接加载，视频使用缩略图/ffmpeg 静帧）  
  > 文件名 + 路径信息  
  > 倒计时进度条 (`CountdownBar`)：显示距下次切换的剩余时间  
  > 模式指示：Video / Image，显存状态（如 degraded 则显示警告）  
  > 快捷操作按钮：下一张 / 上一张 / 切换锁定

- [ ] **4.2 CountdownBar.qml** — 倒计时进度条组件
  > 圆弧或线性进度条，绑定 `DaemonState.nextSwitchSecs`  
  > 不轮询 daemon，本地 QML Timer 每秒递减显示值  
  > 收到 WallpaperChanged 事件时重置

---

## Phase 5 — Library 壁纸库

- [ ] **5.1 WallpaperListModel** (`WallpaperListModel.h/.cpp`)
  > 继承 `QAbstractListModel`  
  > 数据源：`GetSpace` API 返回的壁纸列表  
  > Roles: path, filename, locked, lastPlayed, angle, isCurrent, timeConstraints  
  > 支持 lock/unlock 操作（调用 daemon `ToggleLock`）

- [ ] **5.2 ThumbnailProvider** (`ThumbnailProvider.h/.cpp`)
  > 继承 `QQuickAsyncImageProvider`  
  > 视频：调用 ffmpeg/ffprobe 截取第 5 秒静帧（异步，限并发 ≤2）  
  > 图片：QImage 异步缩放  
  > 磁盘缓存 `~/.cache/lianwall/thumbnails/`，文件名 = hash + 分辨率后缀  
  > QML 使用 `image://thumbnail/<path>`

- [ ] **5.3 LibraryPage.qml + WallpaperDetailDialog.qml**
  > GridView 显示当前模式所有壁纸（缩略图 + 文件名）  
  > 顶部筛选栏：搜索框 + 锁定状态筛选  
  > 点击打开 WallpaperDetailDialog：大图预览、文件信息、lock/unlock、设为当前  
  > 不提供内建文件管理器，仅提供"打开目录"按钮 (`QDesktopServices::openUrl`)

---

## Phase 6 — Settings 设置

- [ ] **6.1 ConfigManager 重写** (`ConfigManager.h/.cpp`)
  > 通过 DaemonClient 读写配置（`GetConfig` / `SetConfig`）  
  > 缓存当前配置到 Q_PROPERTY（供 QML 绑定），daemon 事件驱动更新  
  > 离线回退：daemon 未连接时读写 `~/.config/lianwall/config.toml`  
  > 所有控件必须显示真实值（不允许空占位符）

- [ ] **6.2 SettingsPage.qml** — 4 个配置分区
  > **路径与模式**：mode 下拉框(Video/Image)，video_dir / image_dir（显示路径 + "打开目录"按钮）  
  > **动态壁纸引擎**：interval SpinBox（≥10 秒）、display 输入框、mpvpaper_args 编辑、mpv_args 编辑  
  > **静态壁纸引擎**：interval SpinBox（≥10 秒）、outputs 输入框、swww_args 编辑  
  > **显存监控**：enabled 开关、threshold_percent Slider (5.0–50.0)、recovery_percent Slider (20.0–80.0)、check_interval SpinBox (1–60)、cooldown_seconds SpinBox (10–600)  
  >  
  > 配置值的实际约束（来自 daemon 源码 `default.rs` 注释）：  
  > | 配置项 | 类型 | 范围/约束 | 默认值 |  
  > |--------|------|-----------|--------|  
  > | `interval` (video/image) | u64 | 10–86400 (配置注释)，代码仅验证 >0 | 600 |  
  > | `display` | String | "*" 或显示器名 | "*" |  
  > | `outputs` | String | "" 或逗号分隔 | "" |  
  > | `threshold_percent` | f32 | 5.0–50.0 (配置注释)，代码验证 0–100 | 25.0 |  
  > | `recovery_percent` | f32 | 20.0–80.0 (配置注释)，代码验证 0–100 | 40.0 |  
  > | `check_interval` | u64 | 1–60 (配置注释)，代码仅验证 >0 | 2 |  
  > | `cooldown_seconds` | u64 | 10–600 (配置注释)，代码无范围验证 | 30 |  
  > | `log_level` | String | error/warn/info/debug/trace | "info" |  
  >  
  > GUI 使用 **配置注释中的推荐范围**（更安全），而非代码的宽松验证。

- [ ] **6.3 TagEditor.qml** — 数组参数编辑组件
  > 用于编辑 mpvpaper_args / mpv_args / swww_args  
  > Tag 列表 + 添加/删除 + 输入验证

- [ ] **6.4 Systemd 路径显示**
  > Settings 页面底部显示 systemd 用户服务文件路径  
  > `~/.config/systemd/user/lianwalld.service`  
  > 两个按钮："打开服务文件"（xdg-open）+ "打开目录"

---

## Phase 7 — 国际化

- [ ] **7.1 翻译文件更新**
  > 更新 `lianwall-gui_zh_CN.ts` 和 `lianwall-gui_en.ts`  
  > 所有 UI 可见文本使用 `qsTr()` 包裹

---

## Future TODO（不在本次重写范围）

- [ ] 向量空间可视化（向日葵种子排列 + 黄金角动画）
- [ ] 时间线编辑器（PR 风格拖拽时间段）
- [ ] 视频壁纸实时预览（需解决性能问题）
- [ ] 通知集成（壁纸切换、显存警告等桌面通知）

---

## 性能约束

1. **零轮询**：所有状态更新由 daemon 事件推送驱动
2. **不阻塞 UI 线程**：Socket I/O、缩略图生成、文件扫描全部异步
3. **限制并发**：缩略图生成最多 2 个并发（避免 ffmpeg 吃满 CPU）
4. **关闭 = 隐藏**：窗口关闭只是隐藏到系统托盘，进程不退出
5. **懒加载**：LibraryPage 仅在切换到该页时才请求 GetSpace + 生成缩略图
