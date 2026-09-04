# 19 EwokOS 扩展库（sw.extra 与 projects）

> 本章目标：盘点核心系统之外的两个"扩展"子模块——`sw.extra`（额外软件套件）与
> `projects`（更多应用）——里都带了哪些库：SDL2、curses、HTML 引擎、3D、物理、
> 多媒体框架……这是"ewokos 系列"的第四篇，接续 [18](18-EwokOS常用库.md) 的核心库。

[18](18-EwokOS常用库.md) 章末尾提到过 `sw.extra`。本章把它和 `projects` 一起讲清楚：
它们都是**独立的 git 子模块**，不属于核心 `system/` 构建链，按需启用，
里面 vendored 了一批"重量级"第三方库，供进阶应用（浏览器、视频播放器、3D、模拟器）使用。

## 19.1 两个扩展子模块

| 子模块 | 目录 | 定位 | 默认是否构建 |
|--------|------|------|--------------|
| **ewokos_extra** | [sw.extra/](../../sw.extra/) | 额外软件套件：SDL2 多媒体、curses 文本 UI，以及 vim/计算器/日历等应用 | 由 `make extra` 单独触发 |
| **evokes.apps** | [projects/](../../projects/) | 更多应用：浏览器、视频、屏保、软 3D，以及一堆模拟器与游戏 | `projects/Makefile` 的 `DIRS` 列出哪些就建哪些 |

两者都遵循 [16](16-EwokOS库生态总览.md) 章讲的"手动源码级依赖管理"：
第三方库以源码 vendored 进目录，用定制 Makefile 针对 EwokOS SDK 交叉编译成静态 `.a`，
再链接进各自的应用。它们**编译时依赖核心系统已装好的 SDK**
（`system/build_<arch>/<hw>/{include,lib}`），所以必须先构建完 `system/` 才能构建扩展。

> 与核心库的关键区别：核心库（`system/*/libs`）随 `make` 全量构建、进 rootfs；
> 扩展库是"选装件"，只有你要跑对应应用时才需要编。

## 19.2 sw.extra 的库

`sw.extra` 的库都在 [sw.extra/libs/](../../sw.extra/libs/)，构建清单见其
[Makefile](../../sw.extra/libs/Makefile)（`DIRS = curses SDL2`）。

### curses——文本终端 UI

[sw.extra/libs/curses/](../../sw.extra/libs/curses/) 是 NetBSD curses 的移植，
让你能在终端里画"字符界面"（菜单、面板、彩色文本），是 vim 这类全屏文本程序的基础。
它由两个静态库组成：

| 库 | 用途 |
|----|------|
| **libcurses** | 屏幕绘制：窗口、光标、属性、按键输入 |
| **libterminfo** | 终端能力处理：解析 terminfo 数据库、`tparm`/`tputs` 输出转义序列；并提供 termcap 兼容层（`tgetent`/`tgetstr`/`tgoto` 等） |

> 配套应用：[sw.extra/bin/curse_test/](../../sw.extra/bin/curse_test/) 是 curses 的测试程序，
> [sw.extra/bin/vim/](../../sw.extra/bin/vim/) 则是真正的"用户"。

### SDL2 家族——跨平台多媒体层

[sw.extra/libs/SDL2/](../../sw.extra/libs/SDL2/) 来自"Raspberry Pi Baremetal"移植，
是一整套 SDL2 及其官方扩展库（源码在 `libs/` 子目录，另有 `apps/`、`bin/`）：

| 库 | 链接名 | 用途 |
|----|--------|------|
| **SDL2** | `-lSDL2` | Simple DirectMedia Layer：统一的窗口/输入/音频/定时/线程抽象，游戏与多媒体程序的地基 |
| **SDL2_image** | `-lSDL2_image` | 图像加载：BMP/GIF/JPG/PNG/TGA/TIF/WEBP |
| **SDL2_mixer** | `-lSDL2_mixer` | 音频混音：多通道播放，支持 MP3/OGG/AIFF/VOC 等 |
| **SDL2_ttf** | `-lSDL2_ttf` | TrueType 字体渲染（基于 FreeType） |
| **SDL2_gfx** | `-lSDL2_gfx` | 图形图元（画线/圆/多边形）、rotozoom 缩放旋转、帧率控制 |

> SDL2 在 EwokOS 上把"窗口/输入/音频"映射到系统的 xwin、键鼠驱动与音频服务上，
> 让原本为 Linux/Windows 写的 SDL 程序得以移植运行。`sw.extra/libs/SDL2/` 是其 SDK/安装区。

### 其它（应用与子模块）

`sw.extra` 还带了应用与工具，虽不是"库"但值得一并知道：
[apps/](../../sw.extra/apps/)（calculator 计算器、calendar 日历）、
[bin/](../../sw.extra/bin/)（vim、doubao、curse_test）、
[x/xwm](../../sw.extra/x/)（一个窗口管理器）。
此外 `.gitmodules` 里声明了 **mario_vm**（一个 JavaScript 虚拟机子模块），按需拉取。

## 19.3 projects 的库

`projects`（[projects/](../../projects/)）里每个应用往往自带一个 `libs/`，
放它专属的第三方库。这些库**不在核心构建链里**，跟着各自应用一起编。

> 注意：`projects/Makefile` 的 `DIRS = macemu nesemu soft3d doom cards mine previous`，
> 即默认构建模拟器和游戏；`browser`、`saver`、`video` 是**可选/实验性**应用，
> 需要时单独进入其目录构建。

### browser——网页浏览

[projects/browser/libs/](../../projects/browser/libs/)：

| 库 | 用途 |
|----|------|
| **litehtml** | 轻量 HTML/CSS 渲染引擎：解析并排版网页，输出可绘制的文档树 |
| **gumbo** | Google 的 HTML5 解析器（随 litehtml 一起，负责把 HTML 文本解析成树） |
| **widget++** | 浏览器项目本地的控件变体（配合 litehtml 做网页视图） |

对应应用：[projects/browser/apps/xBrowser](../../projects/browser/apps/)。

### saver——屏保（2D 物理 + GL 数学）

[projects/saver/libs/](../../projects/saver/libs/)：

| 库 | 用途 |
|----|------|
| **cglm** | OpenGL 数学库（C 版）：向量、矩阵、四元数、仿射变换、投影等，3D/2D 图形计算的基础 |
| **ferox** | 2D 刚体物理引擎（`FR_` 前缀 API）：世界、碰撞检测（broadphase）、动力学、摩擦力 |

对应应用：[projects/saver/apps/saver](../../projects/saver/apps/)。

### soft3d——软件 3D 渲染

[projects/soft3d/libs/](../../projects/soft3d/libs/)：

| 库 | 用途 |
|----|------|
| **portablegl** | 用纯 C 实现的 OpenGL 3.x 核心轮廓"软渲染器"——没有 GPU 也能跑 3D |
| **imgui** | Dear ImGui：即时模式 GUI，常用于 3D 程序的调试面板/工具界面 |
| **glcommon** | 配合 portablegl 的通用 GL 辅助（数学、mesh、纹理加载等） |

对应应用（[projects/soft3d/apps/](../../projects/soft3d/apps/)）：`3ddemo`、`gears`、`matrix`、
`Fireworks`、`imgui_demo` 等经典 3D 演示。

### video——视频播放

[projects/video/libs/](../../projects/video/libs/)：

| 库 | 用途 |
|----|------|
| **ffmpeg** | 业界标准多媒体框架：解封装、解码音视频（`lib/` 是完整 ffmpeg 源码 + EwokOS 定制构建 `ewok.mk`） |
| **widget++** | 视频项目本地的控件变体：在核心 `widget++`（见 [18](18-EwokOS常用库.md).3）之上补充两个"重量级"控件——**WidgetWebview**（网页视图，嵌入浏览器内核渲染网页）与 **WidgetVideo**（视频播放） |

对应应用：[projects/video/apps/VideoPlayer](../../projects/video/apps/)、
库自带的 [bin/mp4player.c](../../projects/video/libs/ffmpeg/bin/)。

### 模拟器与游戏（无独立 libs）

`projects` 里的 **macemu / nesemu / minivmac / previous**（模拟器）与
**doom / cards / mine**（游戏）**不自带 `libs/` 目录**：它们直接链接核心库
（`EWOK_LIBC` / `EWOK_LIB_GRAPH` / `EWOK_LIB_X`），个别需要多媒体能力的
（如 `previous` 链接了 `sw.extra` 的 SDL2 家族）再额外链扩展库。
它们的源码本身就是“移植 + 适配 EwokOS”的范例，适合进阶阅读。

## 19.4 扩展库速查表

| 我想…… | 用这个库 | 在哪 |
|--------|----------|------|
| 做全屏文本/字符 UI（菜单、面板） | curses（libcurses + libterminfo） | `sw.extra/libs/curses` |
| 用 SDL 写游戏/多媒体（跨平台） | SDL2 + image/mixer/ttf/gfx | `sw.extra/libs/SDL2` |
| 在窗口里嵌网页/视频控件 | widget++（WidgetWebview/WidgetVideo） | `projects/video/libs/widget++` |
| 渲染 HTML/CSS 网页 | litehtml + gumbo | `projects/browser/libs` |
| 做 3D/2D 图形数学运算 | cglm | `projects/saver/libs` |
| 加 2D 物理（碰撞、刚体） | ferox | `projects/saver/libs` |
| 无 GPU 跑 3D（软件 OpenGL） | portablegl + glcommon | `projects/soft3d/libs` |
| 给 3D 程序加调试 UI | imgui（Dear ImGui） | `projects/soft3d/libs` |
| 解码/播放音视频 | ffmpeg | `projects/video/libs` |

## 19.5 怎么用扩展库

扩展库不进核心 rootfs，用法是"针对 SDK 交叉编译 + 链接进你的应用"：

1. **先构建核心系统**，确保 SDK（`system/build_<arch>/<hw>/{include,lib}`）已就绪；
2. **进入对应库/应用目录 `make`**：扩展库的 Makefile 会 `include` 平台的 `make.rule`，
   用 `-isystem $(SDK_DIR)/include` 找核心头、`-L $(SDK_DIR)/lib` 找核心库；
3. **链接时把扩展库的 `.a` 加进去**，例如 SDL 程序在 `EWOK_LIB_GRAPH`/`EWOK_LIB_X`
   之外再链 `-lSDL2 -lSDL2_image -lSDL2_mixer ...`。

> 顶层可用 `make extra`（在 `sw.extra`）触发扩展套件构建；
> `projects` 则按其 `Makefile` 的 `DIRS` 或单独进入子目录构建。

## 19.6 本章小结

- **sw.extra**（ewokos_extra 子模块）：`curses`（文本 UI）、`SDL2` 家族
  （SDL2/image/mixer/ttf/gfx 多媒体），外加 vim/计算器/日历等应用与 mario_vm（JS 虚拟机）子模块；
- **projects**（evokes.apps 子模块）：库随应用走——browser 用 `litehtml`+`gumbo`，
  saver 用 `cglm`+`ferox`，soft3d 用 `portablegl`+`imgui`+`glcommon`，video 用 `ffmpeg`
  与 `widget++` 扩展控件（WidgetWebview/WidgetVideo）；模拟器与游戏则直接链接核心库；
- 扩展库是**选装件**：不进核心构建链，针对已装好的 SDK 交叉编译，按需链接；
- 至此"ewokos 系列"覆盖了从 libc、核心库到扩展库的完整库版图。

想动手？挑一个扩展应用（比如 `soft3d` 的 `gears`）读它的 Makefile 和源码，
看它是怎么把 portablegl 接到 EwokOS 的 graph/xwin 上的——这是理解"库如何落地"的最好例子。
