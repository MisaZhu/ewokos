# 20 xwm 窗口管理器：机制与实现

> 本章目标：深入 EwokOS 窗口系统中最"微内核味"的一个设计——窗口管理器 xwm。
> 读完你将明白：为什么标题栏、边框、阴影不是 X 服务器画的；xwm 与 xserverd
> 之间的 IPC 协议长什么样；以及如何亲手写一个自己的窗口管理器（换肤）。
> 建议先读完 [第 09 章 IPC](09-IPC进程间通信.md) 和
> [第 13 章 图形系统](13-图形系统与窗口系统.md)。

## 20.1 xwm 是什么：把"策略"从服务器里赶出去

先回忆第 13 章的图形栈：`xserverd` 是窗口服务器，管窗口树、合成、输入分发。
但如果你翻遍 [xserverd 的源码](../../system/xwin/drivers/xserverd/)，
会发现一个奇怪的事实：**它不知道标题栏长什么样，甚至不知道标题栏有多高**。

窗口的"长相"——标题栏、关闭/最大化按钮、边框、圆角、阴影、桌面壁纸——
全部由另一个独立进程负责：**xwm（X Window Manager，窗口管理器）**。

```
┌─────────────┐  注册: dev_cntl(/dev/x, X_DCNTL_SET_XWM)   ┌──────────────┐
│   xserverd   │ ◄──────────────────────────────────────── │     xwm      │
│  (机制层)     │                                           │  (策略层)     │
│ 窗口树/合成/  │  反向 IPC: XWM_CNTL_DRAW_FRAME/...        │ 标题栏/按钮/  │
│ 输入/Z序      │ ────────────────────────────────────────► │ 边框/阴影/    │
└─────────────┘         （画在共享内存画布上）                │ 壁纸/主题     │
                                                           └──────────────┘
```

这是经典的**机制与策略分离（mechanism vs policy）**：

- **机制**（xserverd）：窗口怎么叠、事件发给谁、帧什么时候合成——这些规则
  稳定不变，放在服务器里；
- **策略**（xwm）：窗口"应该长什么样"——这是品味问题，天天想换，
  所以做成可替换的独立进程。

好处立竿见影：

1. **换肤 = 换进程**。仓库里现成四个窗口管理器：
   [opencde](../../system/xwin/xwm/opencde/)（CDE 风格，系统默认）、
   [ewokwm](../../system/xwin/xwm/ewokwm/)（圆角+阴影的现代风格）、
   `mac1984`/`openlook`（在 [sw.extra/x/xwm/](../../sw.extra/x/xwm/)，
   复古 Macintosh 与 OpenLook 风格）。改一行启动脚本就能整体换脸；
2. **xwm 崩了，桌面还在**。xserverd 检测到 xwm 死亡后自动退化成
   "无装饰模式"继续跑（20.7 节细讲），新 xwm 一启动立刻恢复——
   这就是微内核哲学在图形系统里的延续；
3. **写窗口管理器变成写普通应用**。不用碰服务器一行代码，
   继承一个 C++ 类、重载几个 draw 函数即可（20.6 节动手做）。

真实 X11 世界里 twm/mwm/i3 与 X Server 的关系与此如出一辙，
EwokOS 用不到 400 行的 [xwm.c](../../system/xwin/libs/x/src/xwm.c)
把这套思想完整复刻了一遍，是绝佳的学习样本。

## 20.2 注册与发现：xwm 如何"上岗"

xwm 是一个普通用户态进程，由启动脚本
[xinit.rd](../../system/xwin/etc/x/xinit.rd) 拉起：

```sh
@export XTHEME=opencde
@/bin/ipcserv /sbin/x/xwm_opencde     # 用 ipcserv 启动，等它注册完毕再继续

@/bin/x/xlauncher &                   # 然后才启动桌面启动器等应用
```

它的 `main` 函数出奇地短（[ewokwm/xwm.cc](../../system/xwin/xwm/ewokwm/xwm.cc)）：

```c
int main(int argc, char** argv) {
    EwokWM xwm;                       // 构造窗口管理器对象（填好函数表）
    xwm.loadTheme(getenv("XTHEME"));  // 按 XTHEME 环境变量加载主题
    xwm.run();                        // 进入服务循环，从此等着 xserverd 调用
    return 0;
}
```

`run()` 最终落到 [xwm_run()](../../system/xwin/libs/x/src/xwm.c)，
干两件关键的事：

```c
void xwm_run(xwm_t* xwm) {
    // 1. 把自己变成一个 IPC 服务（回顾第 09 章：ipc_serv_run 注册消息处理函数）
    ipc_serv_run(handle, NULL, xwm, IPC_NON_BLOCK);

    // 2. 向 xserverd（/dev/x 设备）自荐："我是窗口管理器！"
    dev_cntl("/dev/x", X_DCNTL_SET_XWM, NULL, NULL);

    while(true) usleep(100000);   // 主线程睡觉，IPC 线程干活
}
```

xserverd 收到 `X_DCNTL_SET_XWM` 后记下三样东西
（[xserver_dev.c](../../system/xwin/drivers/xserverd/xserver_dev.c)）：

```c
x->xwm_pid = from_pid;                     // 谁是 xwm
x->xwm_uuid = proc_get_uuid(from_pid);     // 它的"身份证"（防 pid 复用）
x->xwm_changed = true;                     // 标记：换了新 xwm，几何要重算
```

注意 `xwm_uuid`：pid 会被回收复用，万一 xwm 崩了、又有个无关进程
恰好拿到同一个 pid，服务器要是只认 pid 就会把绘制请求发给无辜者。
所以每次调用前都用
[check_xwm()](../../system/xwin/drivers/xserverd/xserverd.c) 验明正身：

```c
bool check_xwm(x_t* x) {
    if(x->xwm_pid < 0)
        return false;
    if(proc_check_uuid(x->xwm_pid, x->xwm_uuid) == x->xwm_uuid)
        return true;    // pid 还活着且确实是当年那个进程
    x->xwm_pid = -1;    // 死了/换人了：注销，进入无装饰模式
    return false;
}
```

**方向反转**是这里最值得玩味的一点：平时都是应用调用 xserverd，
而 xwm 注册之后，**xserverd 反过来成了 xwm 的客户端**——
每次要画窗口装饰时主动 `ipc_call` 过去。谁提供服务、谁发起调用，
在 IPC 的世界里完全解耦。

## 20.3 IPC 协议：七个命令讲完全部机制

xserverd 与 xwm 之间的全部通信只有 7 个命令
（[x/xwm.h](../../system/xwin/libs/x/include/x/xwm.h)），分两类：

| 命令 | 类型 | 作用 | 何时被调用 |
|---|---|---|---|
| `XWM_CNTL_GET_WIN_SPACE` | 查询 | 给定工作区矩形 wsr，算出加上装饰后的外框 winr | 窗口创建/改变大小/状态切换 |
| `XWM_CNTL_GET_FRAME_AREAS` | 查询 | 返回标题栏/关闭/最小化/最大化/缩放角 5 个热区矩形 | 几何变化后，服务器缓存做命中测试 |
| `XWM_CNTL_GET_MIN_SIZE` | 查询 | 窗口最小尺寸（别让用户把窗口拖没了） | 拖拽缩放时 |
| `XWM_CNTL_DRAW_FRAME` | 绘制 | 画一个窗口的全套装饰（标题栏、按钮、边框、阴影） | 窗口装饰变脏（frame_dirty）时 |
| `XWM_CNTL_DRAW_DESKTOP` | 绘制 | 画桌面背景（壁纸/图案） | 桌面变脏时 |
| `XWM_CNTL_DRAW_DRAG_FRAME` | 绘制 | 画拖动/缩放时的虚线轮廓 | 拖拽过程中每帧 |
| `XWM_CNTL_SET_THEME` | 配置 | 服务器把新主题参数推给 xwm | 运行时换主题 |

分发代码就是一个朴素的 switch
（[xwm.c 的 handle()](../../system/xwin/libs/x/src/xwm.c)）——
和第 09 章你写过的 IPC 服务一模一样。

**查询类**命令体现"策略在 xwm，机制在服务器"的分工。
以最重要的 `GET_WIN_SPACE` 为例：应用说"我要一块 300x200 的工作区"，
服务器自己不会算外框多大，得问 xwm
（[xwin_cmd.c](../../system/xwin/drivers/xserverd/xwin_cmd.c)）；
xwm 按主题参数把标题栏、边框、阴影的空间加上去
（[XWM.cc 的 getWinSpace()](../../system/xwin/libs/x++/src/XWM.cc)）：

```c
void XWM::getWinSpace(int style, int state, grect_t* xr, grect_t* winr) {
    *winr = *xr;                             // 从工作区矩形出发
    if(有标题栏) {                            // 上方加一条标题栏
        winr->y -= xwm.theme.titleH;
        winr->h += xwm.theme.titleH;
    }
    if(有边框) {                              // 四周加边框，右下留阴影
        winr->x -= frameW;   winr->y -= frameW;
        winr->w += 2*frameW + shadow;
        winr->h += 2*frameW + shadow;
    }
    // 最大化/全屏状态下 frameW 和 shadow 按 0 计——贴边窗口没有装饰的容身之处
}
```

于是每个窗口有了两个矩形，贯穿整个窗口系统：

```
winr（窗口外框，xwm 说了算）
┌───────────────────────────────┐
│ 标题栏   [关] [大] [小]        │ ← titleH
│ ┌───────────────────────────┐ │
│ │                           │ │
│ │   wsr（工作区，应用只管这里）│ │
│ │                           │ │
│ └───────────────────────────┘ │
└───────────────────────────────┘▒ ← 右/下边缘的 shadow 带
 ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
```

应用永远只在 wsr 里画自己的内容；wsr 与 winr 之间的"装饰环"归 xwm。

**热区查询**（`GET_FRAME_AREAS`）则解决输入问题：用户点在标题栏上要拖动、
点在关闭按钮上要关窗，可这些区域的位置只有 xwm 知道。做法是服务器在窗口
几何变化后调用一次该命令，把 5 个矩形**缓存**在窗口结构里
（`win->r_title/r_close/r_min/r_max/r_resize`），
之后每次鼠标事件直接本地命中测试
（[xinput.c 的 get_win_frame_pos()](../../system/xwin/drivers/xserverd/xinput.c)）：

```c
static int get_win_frame_pos(x_t* x, xwin_t* win) {
    if(点在 win->r_close 内)  return FRAME_R_CLOSE;
    if(点在 win->r_min 内)    return FRAME_R_MIN;
    if(点在 win->r_max 内)    return FRAME_R_MAX;
    if(点在 win->r_title 内)  return FRAME_R_TITLE;
    if(点在 win->r_resize 内) return FRAME_R_RESIZE;
    return -1;
}
```

想一想为什么要缓存而不是每次鼠标移动都问 xwm——鼠标事件每秒几十上百个，
每个都跨进程 IPC 一趟，延迟和开销都不可接受。**几何是低频变化、
高频读取的数据，一次查询、本地缓存**，这是贯穿 EwokOS 的常用手法。

命中之后的事全在服务器本地完成（机制！）：点中标题栏进入
`X_win_DRAG_MOVE` 拖动状态、点中缩放角进入 `X_win_DRAG_RESIZE`、
松手时给窗口所属应用发 `XEVT_WIN_MOVE` / `XEVT_WIN_RESIZE` /
`XEVT_WIN_CLOSE` / `XEVT_WIN_MAX` 事件
（[xinput.c 的 mouse_xwin_handle()](../../system/xwin/drivers/xserverd/xinput.c)）。
**xwm 全程看不到任何输入事件**——它只负责"画"和"量"，
这让它简单到不可能出并发 bug。

## 20.4 零拷贝绘制：三块共享内存画布

绘制类命令是性能敏感路径。回忆第 13 章的账：一帧全屏像素约 8MB，
走 IPC 拷贝必死。所以 xwm 的所有绘制都发生在**共享内存画布**上，
IPC 里只传 shm_id 和窗口信息：

```
xserverd 进程                                xwm 进程
┌──────────────────────┐                   ┌──────────────────────┐
│ display->g  (整屏画布)│◄── 同一块物理内存 ──►│ desktop_g (shmat 映射)│
│ win->frame_g (装饰层) │◄── 同一块物理内存 ──►│ frame_g   (shmat 映射)│
│ 客户端发布的 ws_g     │◄── 同一块物理内存 ──►│ ws_g      (shmat 映射)│
└──────────────────────┘                   └──────────────────────┘
         ▲                                          │
         └────── IPC 只传 shm_id + xinfo_t ──────────┘
```

三块画布各司其职：

1. **desktop_g——整屏合成画布**。就是 xserverd 用来合成、最终刷上屏幕的
   那块内存。画桌面壁纸、画拖动轮廓、画窗口阴影（阴影落在窗口外面！）
   都直接画在它上面。xwm 端做了缓存
   （[fetch_desktop_graph()](../../system/xwin/libs/x/src/xwm.c)）：
   只要 shm_id 和尺寸没变就复用上次的映射，不必每帧 shmat/shmdt；

2. **frame_g——每窗口的装饰画布**。由服务器按 winr 尺寸分配
   （`xinfo_t.frame_g_shm_id` 带给 xwm），标题栏、按钮、边框都画在这里。
   为什么装饰要单独一层、不直接画在屏幕上？因为合成时装饰环和窗口内容
   要按 Z 序一起参与遮挡运算，装饰必须是窗口自己的一部分。
   而且它是持久的——窗口没变脏时，合成器直接复用上次画好的 frame_g，
   xwm 一次都不用被打扰；

3. **ws_g——客户端发布的工作区内容**。xwm 平时不碰它，只有两种主题效果
   需要读它：半透明圆角边框要与内容混合、失焦窗口的背景特效
   （毛玻璃/高斯模糊）要拿内容做模糊。注释里专门交代了防撕裂的前提：
   服务器只会在客户端"停笔"（painting 标志空闲、更新握手已停）时
   才发起 `DRAW_FRAME`，所以 xwm 读到的一定是完整的一帧。

`DRAW_FRAME` 请求携带的 [xinfo_t](../../system/xwin/libs/x/include/x/xcntl.h)
是整个协议的"名片"，把画装饰需要知道的一切都带全了：

```c
typedef struct {
    int32_t  frame_g_shm_id;    // 装饰画布的共享内存 id
    int32_t  ws_g_buffer_shm_id;// 工作区内容的共享内存 id
    uint32_t style;             // 风格位：NO_FRAME/NO_TITLE/NO_RESIZE/...
    uint32_t state;             // 状态：NORMAL/MAX/FULL_SCREEN/...
    bool     focused;           // 是否焦点窗口（决定用亮色还是灰色）
    grect_t  wsr;               // 工作区矩形（屏幕坐标）
    grect_t  winr;              // 窗口外框矩形（屏幕坐标）
    char     title[XWIN_TITLE_MAX]; // 标题文字
    ...
} xinfo_t;
```

## 20.5 一次完整的 DRAW_FRAME：装饰是怎么画出来的

把两端代码对起来读，就是一次完整的旅程。

**服务器端**（[xrender.c 的 prepare_win_content()](../../system/xwin/drivers/xserverd/xrender.c)）：

```c
if(win->frame_dirty)
    clear_frame_ring(win);        // 只清工作区外的"装饰环"（透明主题要从 0 混合）

// 无边框窗口跳过 xwm；全屏窗口贴边到底，也没有装饰可画
if((style & XWIN_STYLE_NO_FRAME) && !背景特效)  return;
if(state == XWIN_STATE_FULL_SCREEN && !背景特效) return;

if(!check_xwm(x)) return;         // xwm 不在？跳过装饰，窗口裸奔

proto_t in;
PF->format(&in, "i,i,i,m",        // 打包：整屏画布 shm_id、宽、高、xinfo_t
    display->g_shm_id, display->g->w, display->g->h,
    win->xinfo, sizeof(xinfo_t));
ipc_call_wait(x->xwm_pid, XWM_CNTL_DRAW_FRAME, &in);   // 同步等 xwm 画完
```

**xwm 端**（[xwm.c 的 draw_frame()](../../system/xwin/libs/x/src/xwm.c)）
按固定顺序调度函数表里的回调，并处理所有风格/状态组合：

```c
映射三块画布 (desktop_g / frame_g / ws_g)
if(style 没有 NO_FRAME) {
    先调 get_title/get_close/get_max/get_min/get_resize/get_frame 算出各区域
    if(有标题栏 && 非全屏) {
        draw_title(...);                       // ① 标题栏
        if(可缩放) { draw_max(); draw_min(); } // ② 最大化/最小化按钮
        draw_close(...);                       // ③ 关闭按钮
    }
    if(非最大化 && 非全屏) {
        draw_frame(...);                       // ④ 边框（圆角在这里做）
        if(可缩放) draw_resize(...);           // ⑤ 右下角缩放柄
        if(主题带阴影) draw_shadow(...);       // ⑥ 阴影（画到 desktop_g 上）
    }
}
if(允许背景特效 && 失焦)
    draw_bg_effect(...);                       // ⑦ 失焦特效（毛玻璃等）
解除 frame_g / ws_g 的映射（desktop_g 缓存保留）
```

值得注意的工程细节：**风格位与状态的组合逻辑集中在这一处**。
`XWIN_STYLE_NO_TITLE` 只砍标题栏和按钮，边框照画；
`XWIN_STATE_MAX` 保留标题栏但去掉边框和阴影（贴边窗口没有装饰空间）；
`XWIN_STATE_FULL_SCREEN` 什么都不画。具体的 WM 实现（EwokWM 等）
不需要再操心这些组合——它们只管"给我个矩形我怎么画好看"。

## 20.6 实现方法：三层代码写一个窗口管理器

xwm 的实现被拆成清晰的三层，你只需要写最上面一层：

```
第三层  你的 WM     EwokWM / OpenCDEWM …   重载 draw/get 虚函数（~300 行）
第二层  x++ 基类    XWM.cc                 默认几何算法 + 默认画法 + 主题加载
第一层  libx 协议层  xwm.c                  IPC 服务、shm 映射、命令分发（C 函数表）
```

**第一层**是纯 C 的 [xwm_t 函数表](../../system/xwin/libs/x/include/x/xwm.h)——
17 个函数指针（8 个 get、9 个 draw/update），谁填了就调谁，
没填的用兜底逻辑。这一层与 C++ 无关，理论上你可以用纯 C 写 WM。

**第二层** [XWM 基类](../../system/xwin/libs/x++/src/XWM.cc)
把函数表逐一接到 C++ 虚函数上（构造函数里 `xwm.draw_title = draw_title;`
这样的静态跳板），并给出一套完整可用的默认实现：
默认标题栏布局（关闭按钮在左还是在右？`getClose/getMax/getMin` 说了算）、
默认按钮图案、默认矩形阴影、桌面图案的平铺/居中/适配三种模式。
**所以最小的 WM 一行绘制代码都不用写**——全用默认值就是一个方方正正的
灰色主题。

**第三层**看 [EwokWM](../../system/xwin/xwm/ewokwm/EwokWM.cc) 就够了，
它只重载了自己关心的部分：

- `drawTitle`：标题文字居中、按焦点取亮/灰色
  （基类 `getColor()` 统一处理焦点变灰，失焦窗口自动褪色）；
- `drawClose/drawMax`：画成 macOS 风格的红/绿圆球；
- `getClose/getMax`：把按钮从右边挪到左边——**热区跟着挪**，
  服务器的命中测试自动生效，这就是几何查询协议的威力；
- `drawFrame`：调 `markFrameRound()` 用圆弧蒙版把四角抠透明，
  实现圆角窗口（注释提醒：要按去掉阴影带的 frame rect 抠，
  不然右下角会抠进阴影区）；
- `drawShadow`：真·投影几何——"阴影 = 窗口轮廓平移 (shadow, shadow)
  再减去窗口自身"，用整数 SDF 逐像素算 alpha，圆角处的月牙形
  阴影自然出现，不需要手工拼接直条和圆弧。

对比之下 [OpenCDEWM](../../system/xwin/xwm/opencde/OpenCDEWM.cc)
只有 114 行——CDE 风格的方角 3D 边框用默认几何就够了。

### 跟着做：写一个最小 WM

1. 建目录 `system/xwin/xwm/mywm/`，抄一份 ewokwm 的
   [Makefile](../../system/xwin/xwm/ewokwm/Makefile)，
   把目标名改成 `xwm_my`；
2. 写 `MyWM.h/cc`，先什么都不重载：

```cpp
#include <x++/XWM.h>
class MyWM : public Ewok::XWM {
public:
    MyWM(void) {
        xwm.theme.frameBGColor = 0xff336699;  // 只改个边框颜色
        xwm.theme.titleH = 28;
    }
};

int main(int argc, char** argv) {
    MyWM xwm;
    xwm.loadTheme(getenv("XTHEME"));
    xwm.run();
    return 0;
}
```

3. 改 `machines/<你的机器>/system/etc/x/xinit.rd`：

```sh
@export XTHEME=mywm
@/bin/ipcserv /sbin/x/xwm_my
```

4. `make && make sd && make run`，你的第一个窗口管理器上线了。
   之后想改标题栏，重载 `drawTitle`；想挪按钮，重载 `getClose`——
   每次只改一处，立刻能看到效果。

### 主题：数据与代码分离

同一个 WM 程序还能通过**主题文件**换参数，路径约定为
`/usr/x/themes/<主题名>/xwm/theme.json`
（如 [ewokwm 的 theme.json](../../system/xwin/usr/x/themes/ewokwm/xwm/theme.json)）：

```json
{
    "frame_bg_color": "0xffffaa22",
    "title_h": 20,
    "frame_width": 1,
    "shadow": 3,
    "round": 13,
    "frame_alpha": 1,
    "pattern": "/usr/system/images/wallpapers/wallpaper1.png"
}
```

加载路径值得注意：xwm 的 `loadTheme()` 并不自己读文件，而是请
**xserverd** 读（`X_DCNTL_LOAD_XWM_THEME`），再通过
`XWM_CNTL_SET_THEME` 推回给 xwm。绕这一圈是因为主题里的
`shadow`、`round`、`frame_alpha` 服务器合成时也要用
（哪些像素是半透明的、拖动轮廓要缩掉多大的阴影带），
**主题参数必须两边一致，所以由服务器做唯一权威源**
（[xtheme.c](../../system/xwin/drivers/xserverd/xtheme.c) 负责解析）。
光标图案也按主题目录 `<主题>/xwm/cursors/*.json` 加载。

## 20.7 容错：xwm 崩了怎么办

xwm 是普通进程，会崩、会被 kill、会升级重启。窗口系统的应对分三步，
每一步都能在代码里找到：

1. **探活**：所有对 xwm 的 `ipc_call` 之前都有 `check_xwm()`（20.2 节），
   uuid 不对立即注销；
2. **退化**：没有 xwm 时，`draw_desktop` 画一个内置的点阵图案打底
   （[xrender.c 的 draw_init_desktop()](../../system/xwin/drivers/xserverd/xrender.c)），
   `GET_WIN_SPACE` 失败时退化为 `winr = wsr`（窗口无装饰裸奔，
   但**照常工作**——见 [xwin_cmd.c](../../system/xwin/drivers/xserverd/xwin_cmd.c)
   的注释：此刻失败比退化更糟，因为窗口创建不会重试）；
3. **自愈**：新 xwm 注册后置 `xwm_changed`，而服务器主循环
   **每一步**都跑 [xwin_revalidate_geometry()](../../system/xwin/drivers/xserverd/xwin_cmd.c)：
   一个有装饰的窗口不可能 `winr == wsr`，所以这个等式就是
   "几何是 xwm 宕机期间算的"的铁证——重新问一遍 `GET_WIN_SPACE`、
   重建 frame_g、重取热区，窗口自动恢复穿衣。健康窗口几次 memcmp
   就退出，扫描成本可忽略。

这套"探活→退化→自愈"三段式，和第 10 章 vfsd 对待驱动进程的思路一致，
是 EwokOS 里所有服务间依赖的标准处理模板，很值得抄进你自己的设计里。

## 20.8 动手练习

1. 读 [xwm.c 的 draw_frame()](../../system/xwin/libs/x/src/xwm.c)，
   画出一张"风格位 × 状态 → 画哪些部件"的真值表
   （style: NO_FRAME/NO_TITLE/NO_RESIZE；state: NORMAL/MAX/FULL_SCREEN）；
2. 完成 20.6 的最小 WM，然后重载 `drawClose`，把关闭按钮画成一个"×"；
3. 把 `getClose` 和 `getMax` 的坐标改到标题栏右侧，验证点击热区
   跟着移动（想想这中间发生了几次 IPC、缓存是在哪一步刷新的）；
4. 跑起系统后 `kill` 掉 xwm 进程，观察窗口变成无装饰但仍可用；
   再手动 `ipcserv /sbin/x/xwm_ewok`，观察窗口"自动穿回衣服"；
5. 思考题：为什么 `DRAW_FRAME` 用 `ipc_call_wait`（同步等待）而不是
   发完就走？提示：想想合成器下一步要拿 frame_g 做什么。

## 20.9 本章小结

- xwm 是独立用户态进程，与 xserverd 是**机制/策略分离**：
  服务器管窗口树、合成、输入；xwm 只管"量尺寸"和"画装饰"；
- 上岗流程：`ipc_serv_run` 注册 IPC 服务 → `X_DCNTL_SET_XWM` 自荐 →
  服务器记 pid+uuid，之后**反向** `ipc_call` 调用 xwm；
- 协议只有 7 个命令：3 个几何查询（WIN_SPACE/FRAME_AREAS/MIN_SIZE）+
  3 个绘制（FRAME/DESKTOP/DRAG_FRAME）+ 1 个主题推送；
- 绘制零拷贝：desktop_g（整屏）、frame_g（装饰）、ws_g（内容）
  三块共享内存画布，IPC 只传 shm_id 和 xinfo_t；
- 热区（标题/按钮/缩放角）由 xwm 一次算出、服务器缓存后本地命中测试，
  xwm 永远看不到输入事件；
- 实现分三层：libx 的 C 函数表 → x++ 的 XWM 基类默认实现 →
  你的子类按需重载；主题参数走 JSON 文件，由服务器统一加载分发；
- 容错三段式：uuid 探活 → 无装饰退化 → `winr == wsr` 作破绽逐帧自愈。

至此，图形系统的最后一块拼图补齐了。回头看第 13 章的分层图：
从 mailbox 要来的一块裸内存，到 graph 库的画笔，到 xserverd 的合成与分发，
再到 xwm 给每个窗口穿上衣服——每一层都是普通的用户态进程加 IPC，
这就是微内核之美。
