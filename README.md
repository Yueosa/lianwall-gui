# Lianwall GUI

<p align="center">
  <img src="icon.svg" width="128" height="128" alt="Lianwall Logo">
</p>

<p align="center">
  <b>Wayland 动态壁纸管理器的图形界面</b>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Platform-Wayland-blue?logo=wayland" alt="Wayland">
  <img src="https://img.shields.io/badge/Qt-6.x-green?logo=qt" alt="Qt6">
  <img src="https://img.shields.io/badge/License-MIT-yellow" alt="MIT">
</p>

## 简介

Lianwall GUI 是 [lianwall](https://github.com/Yueosa/lianwall) 动态壁纸管理器的图形界面前端，使用 Qt6/QML 开发。

### 功能特性

- 🎨 **壁纸预览** - 实时预览当前壁纸（支持视频播放）
- 📚 **壁纸库** - 网格视图浏览所有壁纸，支持筛选和搜索
- ⏰ **时间轴编辑器** - 可视化管理基于时间的壁纸切换
- 🌻 **向日葵可视化** - 直观展示壁纸空间分布
- 🖥️ **系统托盘** - 快速控制壁纸切换
- 🌐 **多语言支持** - 中文/英文界面
- 🚀 **开机自启** - systemd 用户服务支持

## 依赖

### 运行依赖

- Qt 6.x (Core, Gui, Widgets, Quick, QuickControls2, Multimedia, Network)
- ffmpeg (用于生成视频缩略图)
- [lianwall](https://github.com/Yueosa/lianwall) 守护进程

### 编译依赖

- CMake >= 3.16
- Qt6 开发包
- C++17 编译器

## 编译

```bash
# 克隆仓库
git clone https://github.com/Yueosa/lianwall-gui.git
cd lianwall-gui

# 编译
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# 安装（可选）
sudo cmake --install build
```

## Arch Linux

```bash
cd packaging
makepkg -si
```

## 使用

```bash
# 直接运行
./build/lianwall-gui

# 或者安装后
lianwall-gui
```

启动后：
1. GUI 会自动连接到 lianwall 守护进程
2. 如果守护进程未运行，可以通过托盘菜单启动
3. 主界面提供 6 个页面：首页、库、时间轴、状态、设置、关于

## 配置

GUI 应用设置存储在 `~/.config/lianwall-gui/settings.ini`：

| 设置 | 说明 | 默认值 |
|------|------|--------|
| `exitBehavior` | 退出行为 (ask/minimize/quit/shutdown) | ask |
| `language` | 界面语言 (en/zh_CN) | 系统语言 |
| `autostartEnabled` | 是否开机自启 | false |

lianwall 配置通过 CLI 命令管理，存储在 `~/.config/lianwall/config.toml`。

## 开机自启

在设置页面启用"开机自启"选项，会创建 systemd 用户服务：

```
~/.config/systemd/user/lianwall-gui.service
```

手动管理：

```bash
# 启用
systemctl --user enable lianwall-gui.service

# 启动
systemctl --user start lianwall-gui.service

# 查看状态
systemctl --user status lianwall-gui.service
```

## 截图

（待添加）

## 许可证

MIT License - 详见 [LICENSE](LICENSE) 文件

## 作者

Lian (Yueosa)
