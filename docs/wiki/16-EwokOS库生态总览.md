# 16 EwokOS 库生态总览

> 本章目标：搞清楚 EwokOS 的"库"是怎么组织的——它们住在哪、分几层、
> 怎么被编译、怎么被链接进你的程序。这是"ewokos 系列"的第一篇，
> 后面 [17](17-EwokOS的libc.md) 讲 libc（C 运行时），[18](18-EwokOS常用库.md) 讲常用库。

前 15 章我们把内核、用户态、图形系统都跑通了。但你写一个真实程序时会发现：
光有 `printf` 不够用——你要读 JSON、要解压文件、要画一张 PNG、要发一个 HTTP 请求、
要操作 SD 卡上的 ext2 分区。这些能力不可能都塞进内核（微内核的原则就是"内核只管机制"），
它们以**用户态库**的形式存在。本章先给你一张完整的地图。

## 16.1 为什么 EwokOS 要自己搞一套库

在 Linux 上你 `#include <stdio.h>` 用的是 glibc，`-lpng` 用的是系统装好的 libpng。
EwokOS 不行，原因有三：

| 约束 | 后果 |
|------|------|
| **微内核 + 裸机交叉编译** | 没有现成的宿主 libc 可用，系统调用走 EwokOS 自己的 `svc` 表（第 08 章），不是 Linux ABI |
| **静态链接** | 绝大多数程序把库直接编进可执行文件，产出"自包含"的二进制，适合无动态加载器的环境 |
| **freestanding 构建** | 库用 `-ffreestanding`、`-nostdlib` 一类的选项编译，不能假设宿主系统提供任何服务 |

所以 EwokOS 采用**手动、源码级**的依赖管理：第三方库（zlib、FreeType、wolfSSL、openlibm……）
直接以源码形式放进仓库（vendored），用定制的 Makefile 交叉编译成静态库 `.a`，
再把头文件和库安装到统一的构建目录，供上层链接。没有 `apt`、没有 `vcpkg`、没有 lockfile。

## 16.2 库的分层地图

EwokOS 的库是**分层**的，下层不知道上层的存在，上层依赖下层：

```
        你的应用 / 命令 / 驱动
                 │
   ┌─────────────┼───────────────┬────────────────┐
   │             │               │                │
 网络库        窗口库           图形库           基础库
(network/)   (xwin/libs)     (gui/libs)     (basic/libs)
 socket        x / x++        graph/font     sd/ext2/fat32
 wolfssl       widget++       freetype/png   tinyjson/zlib
 libwebsockets                display/g2d    usb/gpio/c++
   │             │               │                │
   └─────────────┴───────┬───────┴────────────────┘
                         │
                     libc（C 运行时）
              libewoksys + libgloss + openlibm
                         │
                  EwokOS 内核（svc 系统调用）
```

对应到目录，就是五个"库聚集地"：

| 层 | 目录 | 职责 |
|----|------|------|
| **libc** | [system/basic/libc/](../../system/basic/libc/) | C 运行时：标准库胶水、系统调用封装、数学库 |
| **基础库** | [system/basic/libs/](../../system/basic/libs/) | 与硬件/文件系统/数据格式打交道的底层库 |
| **图形库** | [system/gui/libs/](../../system/gui/libs/) | 2D 绘图、字体、图像编解码、显示、输入 |
| **窗口库** | [system/xwin/libs/](../../system/xwin/libs/) | X 风格窗口系统客户端库与 C++ 控件工具包 |
| **网络库** | [system/network/libs/](../../system/network/libs/) | socket、TLS、HTTP、WebSocket、NTP |

> 构建顺序也正是这个依赖顺序：`basic` → `network` → `gui` → `xwin`。
> 第 02 章里 `make` 全量构建时，Makefile 就是按这个链条递归下去的。

## 16.3 一个库长什么样

EwokOS 的库目录高度统一，认识一个就认识了全部。以基础库里的 ext2 为例
（[system/basic/libs/ext2/](../../system/basic/libs/ext2/)）：

```
ext2/
├── include/          # 对外头文件（ext2/ext2fs.h、ext2/ext2head.h）
├── src/              # 与架构无关的源码
├── aarch64/          # AArch64 架构相关源码 + 编译产物（.o）
├── arm/              # ARM32 架构相关源码 + 编译产物
├── x86/              # x86 架构相关源码 + 编译产物
└── Makefile          # 编译成 libext2.a，安装到构建目录
```

要点：

- **`include/` 是公开接口**，`src/` 是实现。你的程序只 `#include <ext2/ext2fs.h>`；
- **按架构分子目录**（`aarch64/`、`arm/`、`x86/`）：架构相关的代码和 `.o` 产物各归各位，
  这样一份源码树可以同时为多个目标编译而互不干扰；
- **Makefile 把产物安装到统一构建目录**：头文件进 `build_<arch>/<hw>/include/`，
  静态库进 `build_<arch>/<hw>/lib/`。上层链接时只需 `-I .../include -L .../lib`。

## 16.4 库怎么被链接进你的程序

你几乎不用手动敲一长串 `-lxxx`。平台规则文件
（[system/platform/aarch64/make.rule](../../system/platform/aarch64/make.rule)）
预定义了三个"库组"变量，Makefile 里直接引用即可：

```make
# C 运行时——任何程序都要链接
EWOK_LIBC = --start-group -lewoksys -lc -lgloss --end-group

# 图形栈——用到画图/字体/图像/显示的程序链接
EWOK_LIB_GRAPH = -lfont -lfreetype -liconbuf -lgraph -lg2dclient \
                 -lpng -ljpeg -lgif -ltga -lsvg -lz -lopenlibm \
                 -ldisplayman -ldisplay -lkeyb -lmouse $(BSP_LFLAGS)

# 窗口栈——写 GUI 应用/控件的程序链接
EWOK_LIB_X = -lwidget++ -lx++ -lx -ltinyjson -lewokstl -ltinyhttpsc -lsocket
```

三个细节：

1. **`--start-group ... --end-group`**：告诉链接器在这组库里反复扫描直到没有新的未解析符号。
   因为 `libewoksys`、`libc`、`libgloss` 互相引用，单遍链接会漏符号，成组扫描省事；
2. **ARM32 多一个 `-lsoftfloat`**：32 位 ARM 目标用软件浮点（见 [17](17-EwokOS的libc.md) 章），
   AArch64/x86 有硬件浮点不需要；
3. **`$(BSP_LFLAGS)`**：把板级支持包（BSP）的库也带进来，不同机器（树莓派/QEMU virt）不一样。

所以第 12 章那个 `hello` 程序的链接行 `$(LD) ... $(EWOK_LIBC)`，
背后展开就是这一整套 C 运行时。要画图的程序再加 `$(EWOK_LIB_GRAPH)`，
要开窗口的再加 `$(EWOK_LIB_X)`。

## 16.5 库怎么被构建与安装

每一层的 `libs/` 目录都有一个"总 Makefile"，用 `DIRS` 列出本层所有库，
并声明库之间的头文件依赖。看基础库的
（[system/basic/libs/Makefile](../../system/basic/libs/Makefile)）：

```make
DIRS = sd ext2 ext3 fat32 elf tinyjson openlibm zlib gpio usb \
       c++/c++ c++/stl c++/object++

# 库之间的依赖：ext2/ext3 需要 sd 的头先装好
ext2 ext3: sd
c++/object++: tinyjson
c++/stl c++/object++: c++/c++
```

图形层（[system/gui/libs/Makefile](../../system/gui/libs/Makefile)）同理，
声明了 `graph` 依赖各图像编解码库、`font` 依赖 `freetype`、
`display`/`textgrid` 依赖 `graph` 等。构建系统据此决定编译顺序，
保证"被依赖的库先把头文件装好，依赖它的库才开编"。

> **一句话总结构建流**：每个库 `make` → 产出 `.a` + 安装头文件到 `build_<arch>/<hw>/` →
> 上层库/程序用 `-I/-L` 找到它们 → 最终 `make sd` 把 rootfs 打包进镜像（第 14 章）。

## 16.6 本章小结

- EwokOS 用**手动源码级**依赖管理：第三方库 vendored 进仓库，交叉编译成静态 `.a`；
- 库分**五层**：libc、基础库、图形库、窗口库、网络库，分别住在
  `basic/libc`、`basic/libs`、`gui/libs`、`xwin/libs`、`network/libs`；
- 每个库目录结构统一：`include/`（接口）+ `src/`（实现）+ 架构子目录（产物）+ `Makefile`；
- 链接靠三个预定义库组：`EWOK_LIBC` / `EWOK_LIB_GRAPH` / `EWOK_LIB_X`；
- 构建按依赖顺序递归，产物统一安装到 `build_<arch>/<hw>/{include,lib}`。

下一章：钻进最底层的那块拼图——EwokOS 的 **libc**，看 `printf` 是怎么一路走到内核的。
