# Changelog - v1.1.1

**发布日期**: 2026-02-08

## 概述

配置路径统一，消除 Qt 默认 QSettings 路径与项目约定路径的不一致。

---

## 🐛 修复

### 配置路径统一

- **QSettings 路径修正** — GUI 本地设置（主题、强调色、语言等）从 Qt 默认路径 `~/.config/Lian/LianWall.conf` 迁移至项目约定路径 `~/.config/lianwall/gui.conf`，与 daemon 配置目录统一

---

## 📁 变更文件

| 文件 | 变更 |
|------|------|
| `src/ConfigManager.cpp` | QSettings 改用显式路径 `~/.config/lianwall/gui.conf`，新增 `guiSettings()` 辅助函数 |
