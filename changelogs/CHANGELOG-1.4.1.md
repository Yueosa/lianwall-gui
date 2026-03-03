# Changelog - v1.4.1

**发布日期**: 2026-03-03

## 概述

Bug 修复版本，修复两处视觉/交互缺陷。

---

## 🔴 Bug 修复

### 1. VRAM 后端下拉框切换静默无效

**文件**: `qml/pages/SettingsPage.qml`

**问题**: 设置页"显存监控 → 后端"下拉框（`StyledSelect`）使用了 `onActivated` 信号处理器，但 `StyledSelect` 是自定义内联组件，其信号为 `selected`，不存在 `activated` 信号。QML 引擎对监听不存在的信号**静默忽略**，不产生任何报错，导致点击下拉框切换后端后 `ConfigManager.setVramBackend()` 永远不被调用，后端配置无法更改。

**修复**: 将 `onActivated` 改为 `onSelected`，与同文件其他 `StyledSelect` 用法保持一致。

```qml
// 修复前
onActivated: function(idx) {
    ConfigManager.setVramBackend(model[idx].value)
}

// 修复后
onSelected: function(idx) {
    ConfigManager.setVramBackend(model[idx].value)
}
```

---

### 2. Library 页缩略图遮挡容器圆角

**文件**: `qml/pages/LibraryPage.qml`

**问题**: 壁纸网格卡片（`thumbCard`）设置了 `radius` 圆角并使用 `clip: true` 裁剪子元素，但 QML 的 `clip` 属性只按**矩形包围盒**裁剪，不尊重 `radius`。导致内部缩略图 `Image` 的四个角（尤其是左上、右上）直接覆盖了容器的圆角视觉区域，显示为直角。

**修复**: 移除 `clip: true`，改用 `QtQuick.Effects.MultiEffect` 掩码裁剪：

- 新增不可见的 `cardMask`（`Rectangle` + `layer.enabled`），与 `thumbCard` 同尺寸同圆角，提供 alpha 遮罩形状
- `thumbCard` 开启 `layer.enabled: true`，通过 `MultiEffect { maskEnabled: true; maskSource: cardMask }` 将整张卡片（图片、文件名、状态标记）按圆角形状正确裁剪

`QtQuick.Effects` 模块随 `qt6-declarative`（Qt ≥ 6.5）内置，无需引入额外依赖。
