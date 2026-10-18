# 13 图形系统与窗口系统

> 语言: [English](13-graphics.md) | **中文**
>
> 本章目标：理解像素如何出现在屏幕上，看懂 EwokOS 图形栈的分层：
> Framebuffer 驱动 → 图形库（g2d）→ 窗口系统（xwin）。
> 本章术语（Framebuffer/Mailbox/graph_t/g2d/blt/ARGB/合成…）见 [附录·术语表](99-glossary.zh.md)。

## 13.1 屏幕本质：一块共享内存

对 CPU 来说，屏幕就是一块按行存放像素颜色的内存——**帧缓冲（Framebuffer）**。
往这块内存写颜色值，屏幕对应位置就变色。图形学的起点如此朴素。

树莓派上这块内存归 GPU 管。CPU 想要它，得通过 **Mailbox（邮箱）**
跟 GPU 商量——往一个硬件邮箱寄存器里写一条"请求消息"，
GPU 处理后在邮箱里回一条"应答消息"：

```
CPU: "给我一块 1024x768、32位色 的帧缓冲"  → 写 MAILBOX 寄存器
GPU: 分配显存，回信"基地址=0x3E9F0000, 宽1024 高768"
CPU: 把该物理地址映射进自己的内存（SYS_MEM_MAP），开始写像素
```

邮箱协议实现在
[mailbox.c](../../machines/raspix/kernel/lib/bcm283x/src/mailbox.c)。
三个函数看懂就够：

```c
void bcm283x_mailbox_send(msg) {
    do { stat = *MAIL0_STATUS; } while(stat.full);  // 等邮箱不满
    *MAIL0_WRITE = *msg;                            // 把消息写进"写"寄存器
}
void bcm283x_mailbox_read(msg) {
    do { flush_dcache(); stat = *MAIL0_STATUS; } while(stat.empty); // 等不空
    *msg = *MAIL0_READ;                             // 从"读"寄存器取
}
void bcm283x_mailbox_call(msg) {   // 一次完整的"问→答"
    flush_dcache();
    bcm283x_mailbox_send(msg);
    bcm283x_mailbox_read(msg);
}
```

三个细节值得学：

- **`flush_dcache`**：消息缓冲在 CPU 缓存里，必须先刷回内存，
  GPU 才读得到——这是"CPU 和别的硬件共享内存"时的必修课；
- **轮询等状态位**：`empty`/`full` 标志位就是硬件在说"现在能不能收/发"；
- **channel（通道）**：邮箱里混着多种消息（帧缓冲、时钟、电源…），
  每条消息带通道号，读的时候按通道过滤。

这又是一个"查手册、写寄存器"的驱动例子，但主角换成了 GPU。

## 13.2 EwokOS 图形栈全景

```
┌──────────────────────────────────────────────────────────┐
│ 应用 / 窗口程序          （xwm 窗口、终端、游戏……）          │
├──────────────────────────────────────────────────────────┤
│ xwin: xserverd + xlib     （窗口、合成、事件分发，X 风格）   │
├──────────────────────────────────────────────────────────┤
│ graph 图形库 + g2dclient   （画点/线/矩形/位图/文字）        │
├──────────────────────────────────────────────────────────┤
│ g2dd / displaymand / fbdisplayd / fontd / consoled        │
│                       （显示驱动、2D 加速、字体、控制台）     │
├──────────────────────────────────────────────────────────┤
│ Framebuffer（GPU 显存）   ← mailbox 申请                   │
└──────────────────────────────────────────────────────────┘
```

全部是用户态进程 + IPC，延续微内核哲学。逐层看：

### ① 显示驱动：fbdisplayd

树莓派的显示驱动在
[machines/raspix/system/drivers/fbdisplayd/](../../machines/raspix/system/drivers/fbdisplayd/)，
它做两件事：通过 mailbox 申请帧缓冲；把 `/dev/disp0`
变成"屏幕"——任何进程通过 IPC 请求把画好的像素刷到屏幕。
`displaymand` 是上层的管理服务（多显示源仲裁、分辨率管理）。

### ② 图形库：graph + g2d

画图原语（画线、填充矩形、位图拷贝、旋转缩放）在
[system/gui/libs/graph/](../../system/gui/libs/graph/)。
先看清"画布"这个核心数据结构（[graph.h](../../system/gui/libs/graph/include/graph/graph.h)）：

```c
typedef struct {
    uint32_t* buffer;   // 像素缓冲（每像素 32 位 ARGB）
    int32_t w, h;       // 宽高
    grect_t clip;       // 裁剪区：画超出的部分自动被剪掉
    bool need_free;
    int32_t shm_id;     // ★若画布来自共享内存，这里是它的 id
    bool shm_contig;    // ★这段共享内存是否物理连续
} graph_t;
```

`graph_new()` 创建的画布用普通 `malloc`（CPU 软件绘制够用）；
但**想让硬件 2D 引擎加速，必须用 `graph_new_shm()`**——
让画布的 `buffer` 落在一段共享内存上，并记下 `shm_id`。为什么？往下看。

g2d 的设计是本章最值得学的部分——**零拷贝共享画布**：

```
应用进程                          g2dd（2D 加速服务）
┌────────────────────┐           ┌────────────────────┐
│ graph_t 画布        │   IPC只传  │ 不保存任何画布      │
│  像素缓冲=共享内存   │ ◄─画布id─► │ attach → 就地绘制   │
│   （shm 段）        │           │ → detach           │
└────────────────────┘           └────────────────────┘
      双方映射同一块物理内存（甚至同一虚拟地址）
```

看 [graph_g2d.c](../../system/gui/libs/graph/src/graph_g2d.c) 的真实代码，
它精确体现了这套规则：

```c
int graph_g2d_avaliable(graph_t* g) {
    // 只有"共享内存后端 + 物理连续"的画布才能交给硬件加速
    if(g->shm_id <= 0 || !g->shm_contig)
        return 0;
    ...
}
// 真要加速时，把虚拟地址换算成物理地址告诉硬件：
canvas.phy = shm_contig_phy_addr(g->shm_id, (ewokos_addr_t)g->buffer);
```

三个关键点串起来：

1. **为什么要共享内存**：普通做法是应用把画布通过 IPC 复制给驱动，
   画完再复制回来——每帧几百 KB 拷两次，性能全毁。共享内存让
   双方直接操作同一块物理页，IPC 只传一个 `shm_id`；
2. **为什么要物理连续**：硬件 2D 引擎（以及 DMA）不认页表，
   它直接拿物理地址干活。普通 `malloc` 的内存物理上零散，
   所以内核提供连续内存池，`shm_contig_phy_addr`（第 05 章的 `shm.c`）
   负责把虚拟地址翻成物理地址；
3. **于是有了两种画布**：纯 CPU 画布（`graph_new`）灵活，
   硬件加速画布（`graph_new_shm` + 连续）快。图形库自动挑合适的后端。

提供加速的原语有 `graph_fill_g2d`（填充）、`graph_blt_g2d`（位图拷贝）、
`graph_blt_alpha_g2d`（带透明度的拷贝）、`graph_rotate_to_g2d`（旋转）等。

### ③ 窗口系统：xwin

`xserverd`（[system/xwin/](../../system/xwin/)）是 X 风格的窗口服务器：

- 维护窗口树，决定谁画在上面（**合成**）；
- 接收输入事件（键盘/鼠标/触摸），分发给对应窗口；
- 窗口程序通过 `xlib` 连接它：创建窗口、画图、收事件。

这里藏着一个第 09 章知识点的实战：**`xserverd` 用 `IPC_MULTI_TASK`
跑多线程服务**（[xserverd.c](../../system/xwin/drivers/xserverd/xserverd.c)）。
原因很现实：鼠标每秒产生几十次"移动"事件，若用单任务模式，
鼠标事件会和"重新合成整屏"这种慢操作排队，光标就卡了。
多线程池让鼠标事件有专属工作线程，立刻被处理——
**这就是第 09 章"线程池"最典型的应用场景**。

窗口管理器（xwm）负责标题栏、边框、拖动，支持多主题
（`ewokwm`、`mac1984`、`openlook` 等，由环境变量 `XTHEME` 选择）。
xwm 本身是一个独立的用户态进程，它与 xserverd 之间的 IPC 协议、
共享内存绘制机制以及“如何写一个自己的窗口管理器”，
在 [第 20 章](20-xwm.zh.md) 专章详解。xserverd 自己这一侧——客户端怎样把
画好的帧交给服务器、服务器如何只重画变化的窗口并只把脏矩形推给显示驱动——
在 [第 22 章](22-xserverd-render.zh.md) 专章详解。

### ④ 启动画面与控制台

- `splashd` + `splash` 命令：开机进度条（回顾 `init.rd` 里的一堆
  `@/bin/splash -m "..." -p xx`）；
- `consoled`：图形控制台（全屏终端），配置在 `/etc/console.json`。

### ⑤ 3D 与 OpenGL：soft3d + 内置的 mesa 编译器

上面讲的都是 2D。`projects/soft3d` 目录在同一套 framebuffer + g2d 管道之上
又搭了一个 **带真正 OpenGL ES 2.0 / EGL API 的 3D 栈**，也是 EwokOS 里唯一
携带真正 GPU 着色器编译器的地方。应用写的是标准 GLES2/EGL 代码
（`<GLES2/gl2.h>` + `<EGL/egl.h>`、`glCreateShader` / `glDrawArrays` /
`eglSwapBuffers`），完全不直接接触后端。

四个部件都在 [projects/soft3d/libs/](../../projects/soft3d/libs/) 下：

- **`gles`**（`libs/gles`）——标准 API 前端，编成 **`libGLESv2.a` +
  `libEGL.a`**，并安装常规的 `GLES2`/`GLES3`/`EGL`/`KHR` 头文件。它在光栅器
  之上叠了一个 CPU 上的 GLSL ES 解释器（`src/glsl`）和唯一的引擎适配层
  （`src/engine/pgl_engine.c`，也是唯一被允许 include `portablegl.h` 的编译
  单元）。应用只需链接 `-lEGL -lGLESv2`（外加 `glcommon` 与系统的 X/graph
  库）；`portablegl` 和 `v3dcompiler` 会用 MRI 脚本合并进 `libGLESv2.a`，作为
  **私有中间产物**对应用不可见。
- **`portablegl`**（`libs/gles/portablegl`）——一份精简、自包含的类
  OpenGL ES 2.x 光栅器（顶点/片元着色器、VBO、纹理、深度测试），渲染进一块
  `graph_t` 风格的帧缓冲。这是 GLES API 背后的引擎。
- **`glcommon`**（`libs/glcommon`）——应用共用的矩阵/向量数学与 GL 辅助工具。
- **`v3dcompiler`**（`libs/gles/v3dcompiler`）——**内置（vendored）的 mesa
  Broadcom V3D 编译器后端**（上游锁定 commit `f45825a0`，记录在
  [VENDORED.txt](../../projects/soft3d/libs/gles/v3dcompiler/VENDORED.txt)）。
  它跑的是 mesa 真实的 `GLSL → NIR → VIR → QPU` 流水线，**在设备上**把
  GLSL ES 3.10 着色器编译成 V3D 7.1 的 QPU 机器码，编成 `libv3dcompiler.a`。

普通目标上，这些 GLES 调用完全用 CPU 光栅化——由 `gles/src/glsl` 里的解释器
驱动 `portablegl`（有 NEON 就加速），不涉及 mesa 编译器。在 **树莓派 5** 上，
**同一个** 应用二进制还会把几何处理 offload 给 VideoCore VII GPU（V3D 7.1 /
BCM2712）；引擎适配层会在每次 draw 时选择走 CPU 还是 GPU，应用侧无需任何改动。
关键点在于：V3D 7.1 的 *binner* 在 BCM2712 上无法光栅化真实图元，所以
`soft3d` 把整个 变换 → 光照 → 光栅化 都放进由内置 mesa 编译器编译的
**CSD compute 着色器** 里做，再经由你在 ② 见过的同一个 `g2dd` 守护进程下发
（由 `PGL_G3D=yes` 门控，只在 `machines/raspi5/system/make.inc` 里导出）：

```
portablegl (pgl_g3d)  →  g2dclient  →  g2dd  →  bsp_g3d  →  vc_pgl3d
      →  machines/raspi5 .../videocore/src/v3d_3d.c  →  V3D CSD dispatch
```

这个 compute 光栅器分 **两趟**：第一趟对每个顶点做一次变换与光照（一个
invocation 处理一个顶点，写出紧凑的 32 字节记录）；第二趟光栅化，在 16×16
的 workgroup 里**每个 lane 负责一个三角形**，用 `atomicMin` 解决深度。这个
“一 lane 一三角形”的形状正是性能关键——V3D 7.1 的 CSD 对**每个 workgroup**
收固定约 3µs 的开销，所以把成千上万个“每三角形一个 workgroup”压成几十个，
正是 `teapot` demo 里 GPU 路径能反超 CPU 光栅器的原因。窗口缩放时 GPU 也不会
掉线：offload 帧缓冲按“只增不减”的几何余量分配（`pgl_g3d_resize`），于是一次
拖拽缩放会收敛成寥寥几次真实重分配、而不是每个事件一次，既不会耗尽内核的连续
共享内存池，也就不会悄悄把应用 latch 回 CPU。

`soft3d` 的应用（`teapot`、`gears`、`3ddemo`、`matrix`、`Fireworks`、
`imgui_demo`）以及 `shadertoy` 风格的全屏 compute demo 都会走到这些路径。
由于真实 QPU 硬件上的着色器 bug 很难调，编译器会先在主机上由
`dump_uniforms` oracle（`libs/gles/v3dcompiler` 下 `make -f Makefile.host`）验证：
它把每个着色器都编译一遍并检查 QPU 输出，之后才会在树莓派上真正运行。

## 13.3 跟着做：跑一个图形程序

构建带图形的完整系统并运行：

```bash
> cd machines/raspix/system
> make          # all 目标包含 x 窗口系统
> make sd
> make run      # QEMU 会弹出图形窗口显示树莓派的"屏幕"
```

启动脚本走完 `xwin/init.rd` 后，你会看到开机画面 → 窗口桌面。
`projects/` 目录里还有现成的图形应用可参考：扫雷（`mine`）、
纸牌（`cards`）、Doom（`doom`）、NES 模拟器（`nesemu`）……
它们的 Makefile 与第 12 章的套路一致，只是链接了图形库。

## 13.4 动手练习

1. 阅读 [graph_g2d.h](../../system/gui/libs/graph/include/graph/graph_g2d.h)，
   列出图形库提供的原语（画线/矩形/位图/文字……）；
2. 参考 `projects/mine/` 写一个最小窗口程序：创建窗口、填充背景色；
3. 思考题：为什么"画布"必须用共享内存而不能走普通 IPC？
   如果屏幕分辨率是 1920x1080 32 位色，一帧多少字节？每秒 60 帧呢？
   （答案：约 8.3MB/帧；普通 IPC 每帧拷两次 = 1GB/秒 的无谓拷贝。）

## 13.5 本章小结

- 屏幕 = 帧缓冲内存，树莓派通过 mailbox 向 GPU 申请（send/read/call 三步、
  先刷缓存、按通道过滤）；
- 图形栈分层：显示驱动 → graph/g2d 图形库 → xwin 窗口系统；
- `graph_t` 画布的 `shm_id`/`shm_contig` 决定它能否走硬件加速；
- g2d 用共享内存实现零拷贝画布，硬件引擎要求物理连续（`shm_contig_phy_addr`）；
- `xserverd` 用 `IPC_MULTI_TASK` 多线程池并行处理请求，保证鼠标低延迟；
- 窗口、主题、事件分发都在用户态，崩了也不会带垮系统；
- 3D/OpenGL 在 `projects/soft3d`：应用用的是标准 **OpenGL ES 2.0 / EGL** API
  （`libs/gles` 产出的 `libGLESv2`/`libEGL`），底层由 `portablegl` 光栅器支撑；
  内置的 **mesa** V3D 后端（`v3dcompiler`）在设备上把 GLSL 编成 QPU，树莓派 5
  上几何处理经 `g2dd` 以两趟 CSD compute 着色器 offload 给 VideoCore VII GPU。

想继续深入窗口系统，可以先读 [第 20 章 xwm 窗口管理器](20-xwm.zh.md)，
再读 [第 22 章 xserverd 合成器](22-xserverd-render.zh.md)；
下一章：把这一切真正放进一块 SD 卡，点亮你的树莓派。
