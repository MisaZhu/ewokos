# 18 EwokOS 常用库（libs）

> 本章目标：盘点 EwokOS 在 libc 之上准备好的"轮子"——文件系统、图像编解码、字体、
> 2D 绘图、窗口控件、网络协议、C++ 支持。读完你会知道：想干某件事时，
> 该去哪个目录找哪个库、链接哪个 `-lxxx`。这是"ewokos 系列"的第三篇（终篇）。

[16](16-EwokOS库生态总览.md) 给了地图，[17](17-EwokOS的libc.md) 讲了最底层的 libc。
本章按"库聚集地"逐层介绍常用库。所有库都遵循 16.3 节讲的统一目录结构。

## 18.1 基础库（system/basic/libs）

与硬件、文件系统、数据格式打交道的底层库
（[system/basic/libs/](../../system/basic/libs/)）。构建清单见
[libs/Makefile](../../system/basic/libs/Makefile) 的 `DIRS`。

### 存储与文件系统

| 库 | 链接名 | 头文件 | 用途 |
|----|--------|--------|------|
| [sd](../../system/basic/libs/sd/) | `-lsd` | `sd/sd.h`、`sd/gpt.h`、`sd/partition.h` | SD 卡/块设备读写、GPT 与 MBR 分区表解析（第 10 章） |
| [ext2](../../system/basic/libs/ext2/) | `-lext2` | `ext2/ext2fs.h`、`ext2/ext2head.h` | ext2 文件系统读写（rootfs 就是 ext2） |
| [ext3](../../system/basic/libs/ext3/) | `-lext3` | `ext3/...` | ext3 文件系统（ext2 的日志增强版） |
| [fat32](../../system/basic/libs/fat32/) | `-lfat32` | `fat32/fat32fs.h`、`fat32/fat32head.h` | FAT32 文件系统（U 盘、SD 卡 FAT 分区） |
| [elf](../../system/basic/libs/elf/) | `-lelf` | `elf/elf.h` | ELF 可执行文件格式解析与加载（第 07 章 exec 用） |

> 依赖关系：`ext2`、`ext3` 都建立在 `sd` 提供的块设备访问之上，
> 所以 Makefile 里写了 `ext2 ext3: sd`。

### 数据与压缩

| 库 | 链接名 | 头文件 | 用途 |
|----|--------|--------|------|
| [tinyjson](../../system/basic/libs/tinyjson/) | `-ltinyjson` | `tinyjson/tinyjson.h` | 轻量 JSON 解析/生成。系统所有 `.json` 配置都靠它（第 10、13 章） |
| [zlib](../../system/basic/libs/zlib/) | `-lz` | `zlib.h` | 经典压缩库：deflate/inflate、gzip、PNG 的底层依赖 |
| [openlibm](../../system/basic/libs/openlibm/) | `-lopenlibm` | `math.h` | 数学库 `libm`（[17](17-EwokOS的libc.md) 章已介绍） |

### 硬件访问

| 库 | 链接名 | 头文件 | 用途 |
|----|--------|--------|------|
| [gpio](../../system/basic/libs/gpio/) | `-lgpio` | `gpio/gpio.h` | GPIO 引脚读写（点灯、读按键、 bit-bang 协议） |
| [usb](../../system/basic/libs/usb/) | `-lusb` | `usb/usb_defs.h`、`usb/bsp_usb.h`、`usb/usbhid.h`、`usb/usbhidsrv.h` | USB 主机栈与 HID（键鼠）设备支持 |

### C++ 支持

EwokOS 的 GUI 与不少应用用 C++ 写，C++ 运行时也在基础库里
（[system/basic/libs/c++/](../../system/basic/libs/c++/)）：

| 库 | 链接名 | 用途 |
|----|--------|------|
| `c++/c++` | （C++ 运行时） | C++ 语言运行时支撑（`cxx.cc`：构造、异常表、RTTI 等基础设施） |
| `c++/stl` | `-lewokstl` | EwokOS 精简版 STL（容器、算法） |
| `c++/object++` | （UniObject） | `UniObject`——对象与 JSON 的双向绑定/反射，依赖 tinyjson |

> 依赖链：`c++/stl` 和 `c++/object++` 都依赖 `c++/c++`；`object++` 还依赖 `tinyjson`。

## 18.2 图形库（system/gui/libs）

2D 绘图、字体、图像编解码、显示与输入
（[system/gui/libs/](../../system/gui/libs/)）。构建清单见
[gui/libs/Makefile](../../system/gui/libs/Makefile)。这一层整体由 `EWOK_LIB_GRAPH` 打包链接。

### 绘图核心

| 库 | 链接名 | 头文件 | 用途 |
|----|--------|--------|------|
| [graph](../../system/gui/libs/graph/) | `-lgraph` | `graph/graph.h`、`graph_image.h`、`graph_ex.h`、`rgb24.h`、`rgb15.h`、`uv12.h` | **2D 图形核心**：画点/线/矩形、位块传输（blt）、alpha 混合、颜色空间转换，并能加载 PNG/JPEG/GIF/TGA/SVG（第 13 章） |
| [g2dclient](../../system/gui/libs/g2dclient/) | `-lg2dclient` | `graph/graph_g2d.h` | 2D 图形**硬件加速**客户端（把 blt/fill offload 给 g2d 驱动） |
| [libiconbuf](../../system/gui/libs/libiconbuf/) | `-liconbuf` | `iconbuf/iconbuf.h` | 图标缓冲：加载/缓存图标资源 |

### 字体

| 库 | 链接名 | 头文件 | 用途 |
|----|--------|--------|------|
| [font](../../system/gui/libs/font/) | `-lfont` | `font/font.h` | 文字渲染：把字符串画成位图，管理字体/字号/颜色 |
| [freetype](../../system/gui/libs/freetype/) | `-lfreetype` | （FreeType） | 业界标准的 TrueType/OpenType 字体光栅化引擎，`font` 的底层 |

### 图像编解码

| 库 | 链接名 | 用途 |
|----|--------|------|
| [libpng](../../system/gui/libs/libpng/) | `-lpng` | PNG 编解码（依赖 zlib） |
| [libjpeg](../../system/gui/libs/libjpeg/) | `-ljpeg` | JPEG 解码 |
| [libgif](../../system/gui/libs/libgif/) | `-lgif` | GIF 解码 |
| [libtga](../../system/gui/libs/libtga/) | `-ltga` | TGA（Targa）图像 |
| [libsvg](../../system/gui/libs/libsvg/) | `-lsvg` | SVG 矢量图渲染（基于 plutovg） |

### 显示与终端

| 库 | 链接名 | 头文件 | 用途 |
|----|--------|--------|------|
| [display](../../system/gui/libs/display/) | `-ldisplay` | `display/display.h` | 显示设备抽象（分辨率、色深、翻转） |
| [displayman](../../system/gui/libs/displayman/) | `-ldisplayman` | `displayman/displayman.h` | 显示管理（多输出/动态时序） |
| [displayd](../../system/gui/libs/displayd/) | （驱动） | — | 显示守护进程侧库 |
| [textgrid](../../system/gui/libs/textgrid/) | `-ltextgrid` | `textgrid/textgrid.h`、`text_content.h` | 字符网格：终端的"行列 + 字符"模型 |
| [gterminal](../../system/gui/libs/gterminal/) | `-lgterminal` | `gterminal/gterminal.h` | 图形终端：把 textgrid 用 font/graph 画出来 |

### 输入与音频

| 库 | 链接名 | 用途 |
|----|--------|------|
| [keyb](../../system/gui/libs/keyb/) | `-lkeyb` | 键盘输入 |
| [mouse](../../system/gui/libs/mouse/) | `-lmouse` | 鼠标输入 |
| [minimp3](../../system/gui/libs/minimp3/) | — | MP3 解码（单头文件） |
| [libogg](../../system/gui/libs/libogg/) / [libvorbis](../../system/gui/libs/libvorbis/) | `-logg`/`-lvorbis` | OGG 容器与 Vorbis 音频解码 |

> 依赖关系（摘自 Makefile）：`graph` 依赖各图像编解码库与 `g2dclient`；
> `font` 依赖 `freetype`；`display`/`libiconbuf` 依赖 `graph`；
> `textgrid`/`gterminal` 依赖 `font`+`graph`，且 `gterminal` 依赖 `textgrid`。

## 18.3 窗口库（system/xwin/libs）

X 风格窗口系统的客户端库与 C++ 控件工具包
（[system/xwin/libs/](../../system/xwin/libs/)）。构建链：`x` → `x++` → `widget++`，
整体由 `EWOK_LIB_X` 打包链接（第 13 章）。

| 库 | 链接名 | 头文件 | 用途 |
|----|--------|--------|------|
| [x](../../system/xwin/libs/x/) | `-lx` | `x/x.h`、`xwin.h`、`xevent.h`、`xwm.h`、`xtheme.h`、`xcntl.h` | 窗口系统 C 接口：创建窗口、收发事件、与窗口管理器/主题交互 |
| [x++](../../system/xwin/libs/x++/) | `-lx++` | `x++/X.h`、`XWin.h`、`XWM.h`、`XTheme.h` | `x` 的 C++ 封装，面向对象地操作窗口 |
| [widget++](../../system/xwin/libs/widget++/) | `-lwidget++` | `Widget/*.h` | **C++ 控件工具包**：现成的 UI 组件 |

`widget++` 提供的控件（`include/Widget/`）相当齐全：

```
Widget（基类）  Container（容器）  WidgetWin / RootWidget / WidgetX
Button  RoundButton  LabelButton  RoundLabelButton  Label  Image
Text  EditLine  List  ListBase  Scroller  Scrollable  Slider
Grid  Columns  Split  Splitter  Stage  Blank  SpriteAnim  SpriteWin
```

写一个带按钮、列表、输入框的图形应用，直接用这些控件拼装即可，不必从画像素开始。

## 18.4 网络库（system/network/libs）

网络协议与上层应用协议
（[system/network/libs/](../../system/network/libs/)）。构建清单见
[network/libs/Makefile](../../system/network/libs/Makefile)。

| 库 | 链接名 | 用途 |
|----|--------|------|
| [socket](../../system/network/libs/socket/) | `-lsocket` | BSD socket API：`socket`/`bind`/`connect`/`listen`/`accept`/`send`/`recv`，以及 `inet_ntop`/`inet_pton`/`inet_ntoa`。底层是用户态 netd 协议栈（第 09 章 IPC + 网络守护进程） |
| [ntpc](../../system/network/libs/ntpc/) | `-lntpc` | NTP 客户端：向时间服务器校时 |
| [wolfssl](../../system/network/libs/wolfssl/) | `-lwolfssl` | 嵌入式 TLS/SSL 库（wolfSSL），提供 HTTPS/加密能力 |
| [libtinyhttpsc](../../system/network/libs/libtinyhttpsc/) | `-ltinyhttpsc` | 轻量 HTTP 服务器/客户端，可跑在 wolfSSL 之上做 HTTPS |
| [libwebsockets](../../system/network/libs/libwebsockets/) | `-lwebsockets` | WebSocket 库：全双工实时通信 |

> 依赖关系：`ntpc`/`wolfssl`/`libwebsockets` 都需要 `socket` 先装好头文件；
> `libtinyhttpsc` 同时依赖 `socket` 与 `wolfssl`。

## 18.5 库速查表（按"我想干什么"查）

| 我想…… | 用这个库 | 链接 |
|--------|----------|------|
| 读写 SD 卡 / 解析分区 | sd | `-lsd` |
| 读写 ext2/ext3/FAT32 文件 | ext2 / ext3 / fat32 | `-lext2` 等 |
| 解析/加载 ELF 可执行文件 | elf | `-lelf` |
| 解析 JSON 配置 | tinyjson | `-ltinyjson` |
| 压缩/解压（gzip、zip、PNG 底层） | zlib | `-lz` |
| 做数学运算（sin/cos/sqrt） | openlibm | `-lopenlibm` |
| 点灯 / 读 GPIO | gpio | `-lgpio` |
| 接 USB 键鼠/HID | usb | `-lusb` |
| 画点线面、贴图、alpha 混合 | graph | `-lgraph`（`EWOK_LIB_GRAPH`） |
| 硬件加速 2D blt/fill | g2dclient | `-lg2dclient` |
| 渲染 TrueType 字体/文字 | font + freetype | `-lfont -lfreetype` |
| 解码 PNG/JPEG/GIF/TGA/SVG | libpng/libjpeg/libgif/libtga/libsvg | `-lpng` 等 |
| 操作显示/framebuffer | display / displayman / fb | `-ldisplay` 等 |
| 做字符终端 | textgrid + gterminal | `-lgterminal` |
| 读键盘/鼠标 | keyb / mouse | `-lkeyb -lmouse` |
| 播放 MP3/OGG 音频 | minimp3 / libogg+libvorbis | — |
| 开窗口、收发窗口事件 | x / x++ | `-lx -lx++`（`EWOK_LIB_X`） |
| 用现成 UI 控件（按钮/列表/输入框） | widget++ | `-lwidget++` |
| 发 TCP/UDP 数据包 | socket | `-lsocket` |
| 做 HTTPS / 加密 | wolfssl + libtinyhttpsc | `-lwolfssl -ltinyhttpsc` |
| 做 WebSocket 实时通信 | libwebsockets | `-lwebsockets` |
| NTP 校时 | ntpc | `-lntpc` |
| 用 C++ / STL / 对象-JSON 绑定 | c++ / stl / object++ | `-lewokstl` 等 |

## 18.6 还有更多：sw.extra 里的第三方套件

本章列的是**核心系统自带**的库。仓库里还有一个独立的子模块
[sw.extra/](../../sw.extra/)，收录了更大的第三方套件与应用，例如：

- **SDL2**（[sw.extra/libs/SDL2/](../../sw.extra/libs/SDL2/)）——跨平台多媒体/游戏开发层；
- **curses**（[sw.extra/libs/curses/](../../sw.extra/libs/curses/)）——文本终端 UI（`libcurses` + `libterminfo`）；
- **vim**、计算器、日历等应用（`widget++` 的扩展控件则在 `projects` 里，见 [19](19-EwokOS扩展库.md)）。

它们同样以源码 vendored、用定制 Makefile 交叉编译，但属于"扩展软件"，
不在核心 `system/` 构建链里，按需启用。

## 18.7 本章小结

- **基础库**（`basic/libs`）：文件系统（sd/ext2/ext3/fat32/elf）、数据（tinyjson/zlib）、
  数学（openlibm）、硬件（gpio/usb）、C++（c++/stl/object++）；
- **图形库**（`gui/libs`）：绘图核心 graph + g2dclient、字体 font+freetype、
  图像编解码 png/jpeg/gif/tga/svg、显示 display/fb、终端 textgrid/gterminal、
  输入 keyb/mouse、音频 minimp3/ogg/vorbis，统一由 `EWOK_LIB_GRAPH` 链接；
- **窗口库**（`xwin/libs`）：x → x++ → widget++（丰富 C++ 控件），由 `EWOK_LIB_X` 链接；
- **网络库**（`network/libs`）：socket、ntpc、wolfssl、libtinyhttpsc、libwebsockets；
- 更大的第三方套件（SDL2、curses、vim……）在独立子模块 `sw.extra/`。

至此"ewokos 系列"三篇完结：你已经有了一张从 libc 到各类常用库的完整地图。
接下来做什么？回到 [15 调试与进阶](15-调试技巧与进阶路线.md)，或挑一个库读它的源码——
它们都不大，结构也清晰，非常适合作为进阶练习。
