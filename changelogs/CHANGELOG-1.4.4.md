# Changelog - v1.4.4

> 发布日期：2026-08-08

**配套**: lianwall ≥ 5.5.1

## 概述

1.4.4 聚焦交互可靠性与壁纸库性能：缩略图增加内存 LRU 预算、设置页输入与滚动条修复，并将壁纸切换的成功语义改为「事件驱动」——点击后显示「切换中」，以 `WallpaperChanged` / 模式变化收尾，而不把 IPC 超时当业务失败。

---

## 新功能 / 改进

### 1. 缩略图内存 LRU（96 MiB 预算）

**更新**：
- `ThumbnailProvider` 增加线程安全的字节预算 LRU，超额淘汰最旧条目
- 请求尺寸归档到标准档位，减少缓存碎片
- 图 / 视频统一走 `image://thumbnail/`；ffmpeg 使用输入 seek 加速截帧
- Library 加大 `cacheBuffer`，配合 `reuseItems`

**效果**：
- 滑出视口后短期内再滑回，优先命中内存缓存，减少闪白与重复解码

### 2. 壁纸切换「进行中」状态

**更新**：
- 新增 `LianwallApp.wallpaperSwitching`
- Dashboard 按钮与预览遮罩显示「切换中…」
- 以 `WallpaperChanged` / `modeChanged` / 成功 `Ok` 收尾；`Timeout` 不当业务错误弹窗

**配套**：
- 需 `lianwalld ≥ 5.5.1`，壁纸类命令不再因短超时误报 `Timeout`

### 3. 设置页与滚动条

**修复**：
- 去掉 `ScrollView` 嵌套 `Flickable` 的双层滚动
- 统一 `StyledScrollBar`
- 数字 / 文本输入框支持 Enter 提交（不再只能靠失焦）

### 4. 错误上屏补齐

**更新**：
- 壁纸库 / 配置写入失败会通过顶部错误横幅提示
- 过滤连接抖动与 timeout 类非业务提示

---

## 影响的文件

- `src/ThumbnailProvider.*`
- `src/Application.*` / `src/Constants.h`
- `qml/pages/{Library,Settings,Dashboard,About}Page.qml`
- `qml/components/StyledScrollBar.qml`
- `qml/main.qml` / `qml/dialogs/WallpaperDetailDialog.qml`

---

## 升级说明

- 建议同时升级 `lianwall` / `lianwalld` 到 **5.5.1**
- 仅升级 GUI 时，旧 daemon 仍可能发出 Timeout，但 GUI 会忽略其业务含义并等待事件
