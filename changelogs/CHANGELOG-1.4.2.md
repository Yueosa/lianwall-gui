# Changelog - v1.4.2

**发布日期**: 2026-03-25

## 概述

适配新静态壁纸引擎 awww，更新相关 UI 显示文本。

---

## 🔧 功能适配

### 静态壁纸引擎由 swww 更换为 awww

**文件**:
- `qml/pages/SettingsPage.qml`
- `translations/lianwall-gui_en.ts`
- `translations/lianwall-gui_zh_CN.ts`

**变更内容**: 将设置页中静态壁纸引擎参数标签由 `"swww 参数"` 更新为 `"awww 参数"`（中英文翻译同步更新），以匹配新的 awww 引擎。

后端逻辑（C++ `ConfigManager`、IPC 协议字段 `swww_args`）保持不变，仅修改用户可见的 UI 显示文本。
