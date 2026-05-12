# Changelog - v1.4.3

> 发布日期：2026-05-13

## 概述

1.4.3 是一轮针对壁纸库与切换交互的整合稳定性修复，重点处理缩略图刷新失效、整个卡片容器空洞、图片加载卡顿，以及快速连续操作导致的状态错乱。

---

## 修复内容

### 1. 壁纸库缩略图刷新令牌

**问题**：
- Library 页面缩略图 URL 固定
- QML 图片缓存与缩略图提供器缓存叠加后，列表刷新时可能继续显示旧缩略图或失败状态

**修复**：
- 为 `WallpaperListModel` 添加 `refreshToken`
- 每次重新加载壁纸列表后递增令牌
- Library 页缩略图 URL 附带 `?t=<refreshToken>` 强制重新请求

---

### 2. 详情预览与列表刷新行为对齐

**问题**：
- 详情弹窗预览与库页缩略图刷新策略不一致
- 切换后可能仍显示旧图或旧视频首帧

**修复**：
- 视频详情预览同样接入缩略图刷新令牌
- 图片详情预览追加 fragment 触发重新加载

---

### 3. GUI 命令防连点

**问题**：
- `next`、`prev`、`setMode`、`toggleLock` 可在前一次请求未返回时继续发送
- 快速点击会在 daemon 命令队列中堆积请求，增加界面与实际状态错位概率

**修复**：
- 为应用级壁纸切换命令添加在飞门闩
- 为壁纸库内的 `setAsCurrent` / `toggleLock` 添加按路径去重

---

### 4. 清理 DashboardPage 残留文本

**问题**：
- `DashboardPage.qml` 混入了非 QML 文本，导致构建失败

**修复**：
- 移除残留文本，恢复 GUI 可编译状态

---

### 5. 修复壁纸库卡片整块空洞

**问题**：
- 库页中不是单纯的缩略图失效，而是整个卡片容器在滚动复用后消失
- 列表中会直接留下空缺位置

**修复**：
- 移除 Library 页卡片上的 `layer + MultiEffect + mask` 离屏渲染链路
- 改为更朴素的矩形裁剪结构，避免 delegate 在滚动复用中的场景图层失效

---

### 6. 修复图片库页白卡并优化加载路径

**问题**：
- 图片模式库页也走 `ThumbnailProvider`
- 与缓存刷新和懒加载叠加后更容易出现白卡或失败态残留
- 图片按原图尺寸解码，滚动加载时更容易卡顿

**修复**：
- Library 页图片缩略图改为直接 `file://` 加载
- 图片卡片按实际显示尺寸请求解码，降低滚动时的 CPU 与渲染压力
- 图片占位阶段不再为每张卡片启用 BusyIndicator 动画，仅视频缩略图保留加载旋转提示

---

### 7. 提升列表复用稳定性与 awww 文案一致性

**修复**：
- 为 `GridView` 增加缓存缓冲区，减轻视口边缘 delegate 频繁重建带来的抖动
- README 与 GUI 对外文案中的静态壁纸引擎名称统一为 `awww`

---

## 影响的文件

- `src/Application.h`
- `src/Application.cpp`
- `src/WallpaperListModel.h`
- `src/WallpaperListModel.cpp`
- `qml/pages/LibraryPage.qml`
- `qml/dialogs/WallpaperDetailDialog.qml`
- `qml/pages/DashboardPage.qml`
- `README.md`
- `CMakeLists.txt`

---

## 升级说明

建议与 `lianwall 5.5.0` / `lianwalld 5.5.0` 配套升级，以获得新的概率选择算法与更稳定的 GUI 联动。
