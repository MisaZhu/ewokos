# 22 xserverd 合成器：窗口绘制流程与脏区机制

> 语言: [English](22-xserverd-render.md) | **中文**
>
> 本章目标：把 xserverd 合成器的"一帧"从头到尾拆开——客户端怎么把画好的
> 像素交给服务器、服务器怎么判断哪些窗口需要重画、一个窗口的内容和装饰
> 如何落到屏幕缓冲上、最后又如何只把变化的矩形推给显示驱动。
> 读完你将能看懂 [xserverd/](../../system/xwin/drivers/xserverd/) 下
> `xwin.c` / `xrepaint.c` / `xrender.c` 三个文件里几乎每一行注释。
> 建议先读完 [第 13 章 图形系统](13-graphics.zh.md) 和
> [第 20 章 xwm 窗口管理器](20-xwm.zh.md)。

## 22.1 全景：一帧是怎么走到屏幕上的

第 20 章讲的是"窗口长什么样"（xwm 的策略）；本章讲的是"像素怎么搬"
（xserverd 的机制）。先给出整条流水线的鸟瞰图：

```
 客户端进程 (libx)                xserverd 主循环 (loop_step)                 fbdisplayd
┌────────────────┐    共享内存    ┌──────────────────────────────────┐          ┌──────────┐
│ on_repaint()   │──画进 ws_g──►│ ① x_poll_updates  接收发布的帧    │          │          │
│ xwin_repaint() │  置位握手标志  │ ② x_repaint       每个显示器合成  │──flush──►│ 推到面板  │
│  (阻塞/异步)    │◄─唤醒/放行───│    ├ draw_desktop  (问 xwm 画壁纸)│ 只带脏矩形│  (DMA)   │
└────────────────┘              │    ├ draw_win ×N   (自底向上)     │          └──────────┘
                                │    │   └ DRAW_FRAME (问 xwm 画装饰)│
                                │    └ 光标/拖拽轮廓 叠加            │
                                │ ③ display_flush   (锁外阻塞等待)   │
                                └──────────────────────────────────┘
```

这条流水线有三个贯穿全章的设计决定：

1. **合成器直接读客户端发布的那块缓冲**。服务器不再为每个窗口保留一份
   私有快照，客户端渲染完成的 `ws_g` 就是合成源。一帧少搬一次全窗口像素；
2. **合成结果直接写进扫描输出（scan-out）DMA 缓冲**。`display->g` 就是
   fbdisplayd 交出来的那块内存（`display_fetch_graph`），合成完只需告诉
   驱动"哪几个矩形变了"，驱动再按矩形推到面板；
3. **两级脏标记**。`display->dirty` 表示整屏重建（桌面 + 全部窗口自底向上
   重画），`display->need_repaint` 表示增量重画（只画 `win->dirty` 的窗口）。
   绝大多数帧走增量路径，整屏重建只在 Z 序、几何、透明窗口等变化时发生。

模块分工一表看清（对应 [xserverd.c](../../system/xwin/drivers/xserverd/xserverd.c)
文件头的注释）：

| 文件 | 职责 | 本章涉及的核心函数 |
|---|---|---|
| [xserver_dev.c](../../system/xwin/drivers/xserverd/xserver_dev.c) | vdevice 回调、主循环节拍 | `xserver_step` |
| [xwin.c](../../system/xwin/drivers/xserverd/xwin.c) | 窗口链表、帧接收握手、脏区传播 | `x_poll_updates` `x_accept_update` `x_update_commit` `mark_dirty` `covered_by_opaque_win` |
| [xrepaint.c](../../system/xwin/drivers/xserverd/xrepaint.c) | 单显示器重画流水线、脏矩形收集、光标 | `x_repaint` `x_repaint_add_dirty` `pack_dirty_rects` `x_cursor_redraw_now` |
| [xrender.c](../../system/xwin/drivers/xserverd/xrender.c) | 桌面/窗口/装饰的实际位块搬运 | `draw_win` `prepare_win_content` `blit_win_part` `refresh_shadows_above` |
| [xserver.h](../../system/xwin/drivers/xserverd/xserver.h) | 数据结构、跨模块内联判定 | `win_comp_src` `frame_cuts_ws` `win_bg_effect_active` |

## 22.2 画布与状态：先认清几块内存

### 每个窗口的四块画布

[xwin_t](../../system/xwin/drivers/xserverd/xserver.h) 里挂着四个 `graph_t*`，
全部由服务器用 `graph_new_shm` 分配、通过 [xinfo_t](../../system/xwin/libs/x/include/x/xcntl.h)
里的 shm id 发布给客户端和 xwm：

```
winr（窗口外框）
┌──────────────────────────────────┐
│ frame_g: 装饰环（标题栏/边框/阴影）│  尺寸 = winr，xwm 在上面画
│ ┌──────────────────────────────┐ │
│ │ ws_g : 客户端渲染目标        │ │  尺寸 = wsr，客户端 on_repaint 画在这
│ │ ws_g2: fps_async 交接缓冲    │ │  仅 fps_async=1 时存在，客户端把 ws_g 拷进来
│ └──────────────────────────────┘ │
│ backdrop: 窗口下方场景的干净快照  │  仅主题开 frame_blur 时存在，给 xwm 做毛玻璃
└──────────────────────────────────┘
```

- **ws_g / ws_g2**：合成器读哪一块由 [win_comp_src()](../../system/xwin/drivers/xserverd/xserver.h) 决定——
  `fps_async && front_index == 1` 读 `ws_g2`，否则读 `ws_g`；
- **frame_g**：注意它**只装装饰环**。工作区那一块通常是空的，合成时工作区
  像素直接从客户端缓冲取——只有两种主题效果（失焦背景特效、半透明圆角边框）
  需要把工作区内容先拷进 `frame_g` 再让 xwm 在上面混合（22.6 节）；
- **backdrop**：毛玻璃边框要对"窗口下面是什么"做模糊，而屏幕缓冲在窗口
  位置上已经被窗口自己盖住了，所以服务器在每次下方内容重画时顺手抓一份
  干净快照（`capture_backdrop`）。

### 每个窗口的状态位

| 字段 | 含义 | 谁置位 | 谁清除 |
|---|---|---|---|
| `ready` | 客户端至少发布过一帧，可以参与合成 | `x_accept_update` | 重建几何时 |
| `dirty` | 内容变了，本帧要重画 | `x_accept_update`、`mark_dirty_confirm`、整屏重建 | `draw_win` 末尾 |
| `frame_dirty` | 装饰环要重画（要问 xwm） | 焦点切换、几何重建、`win_mark_frame_dirty` | `draw_win` 末尾 |
| `dirty_mark` | 脏区传播时的临时标记 | `mark_dirty` | `mark_dirty_confirm` |
| `shadow_valid` + `shadow_rect` | 该窗口的半透明部分（阴影带/圆角）已经在**当前位置**混合到屏幕上 | `draw_win` 混合完 | 换 Z 序、换焦点、整屏重建、被完全遮盖 |
| `not_ready_ms` `paint_ms` `accept_ms` `repaint_req_ms` | 几个超时计时器（22.10 节） | — | — |

### 每个显示器的状态

[x_display_t](../../system/xwin/drivers/xserverd/xserver.h)：

| 字段 | 含义 |
|---|---|
| `g` | 扫描输出画布（就是 fbdisplayd 的 DMA 缓冲），所有合成写到这里 |
| `dirty` | **整屏重建**标志：桌面 + 所有窗口自底向上全部重画 |
| `need_repaint` | **增量重画**标志：本步要跑一次 `x_repaint` |
| `desktop_rect` | 桌面区域（整屏） |
| `pending_flush` / `flush_inflight` | 本帧有变化待推送 / 推送进行中（保护脏矩形单槽，22.7 节） |
| `wait_ready` / `paint_wait` | 等窗口就绪 / 等客户端画完 的帧计数（有上限） |
| `cursor_task` | 光标位置变了但快速路径没画上，交给帧路径补画 |

两个入口函数把这两级脏标记的语义定死
（[xserverd.c](../../system/xwin/drivers/xserverd/xserverd.c)）：

```c
void x_dirty(x_t* x, int32_t display_index);        // dirty = need_repaint = true   → 整屏重建
void x_repaint_req(x_t* x, int32_t display_index);  // need_repaint = true            → 增量重画
```

读代码时看到 `x_dirty` 就要问一句"为什么这里非得整屏重建"——
每个调用点的注释基本都回答了这个问题。

## 22.3 客户端发布一帧：基于共享内存的 UPDATE 握手

老版本的 xserverd 靠一条 `XWIN_CNTL_UPDATE` IPC 通知服务器"我画完了"，
服务器再把工作区拷一份快照。现在这条 IPC 彻底没了：客户端和服务器只通过
`xinfo_t` 里几个 `volatile` 字段握手，热路径上不经过 vdevice 分发、
不查文件缓存、不拷像素。

### 阻塞模式（`fps_async = 0`，默认）

客户端侧 [xwin_repaint()](../../system/xwin/libs/x/src/xwin.c) 的骨架：

```c
void xwin_repaint(xwin_t* xwin) {
    pthread_mutex_lock(&xwin->painting_lock);
    xwin->xinfo->painting = 1;              // ① 声明"我在写这块缓冲"
    xwin->on_repaint(xwin, &g);             // ② 应用画进 ws_g（就是合成源！）

    xwin->xinfo->update_pid = thread_get_id();   // ③ 记下当前线程 pid（不是 getpid）
    xwin->xinfo->painting = 0;              // ④ 停笔
    __sync_synchronize();                   //    release 屏障：像素先于标志可见
    xwin->xinfo->update_requested = 1;      // ⑤ 发布
    while(xwin->xinfo->update_requested)
        proc_block_by(xwin->xinfo->win);    // ⑥ 停在这，直到服务器合成完放行

    xwin->xinfo->painting = 1;              // ⑦ 放行后立刻重新声明（帧缓冲式应用在两次 present 之间画）
    pthread_mutex_unlock(&xwin->painting_lock);
}
```

服务器侧分两步，**接收**在每步开头，**放行**在合成之后：

```c
// xwin.c: 每步开头扫一遍所有窗口
void x_poll_updates(x_t* x) {
    for(每个窗口 win) {
        if(win->xinfo->update_requested) {
            __sync_synchronize();               // acquire 屏障，和客户端的 release 配对
            if(可见 && win_comp_src(win) != NULL)
                x_accept_update(x, win);        // 不拷贝！只做记账：ready=true, win_dirty()
            else
                x_update_commit(x, win);        // 看不见的窗口：别让客户端白等
        }
        ...超时处理见 22.10...
    }
}

// xrepaint.c: 合成完这个窗口后
draw_win(display->g, x, win, &win_dirty);
x_update_commit(x, win);                        // 清 update_requested，proc_wakeup_by(update_pid)
```

三处细节值得停下来想：

- **为什么可以不拷贝**：客户端从 ⑤ 到 ⑥ 停在 `proc_block_by` 里，
  它**物理上不可能**在服务器读 `ws_g` 时往里写。缓冲的所有权靠握手协议
  移交，而不是靠拷贝隔离；
- **为什么 `update_pid` 必须是线程 pid**：`proc_wakeup_by` 精确唤醒某个
  内核任务；`getpid()` 返回的是主线程，若应用在自己的渲染线程里调
  `xwin_repaint`（如 macemu 的 present 线程），唤醒错对象就永久卡死；
- **为什么放行放在 `draw_win` 之后而不是接收时**：`draw_win` 是这块缓冲的
  最后一个读者（它还把缓冲交给 xwm 画装饰）。若在接收时就放行，客户端会
  立刻开始画下一帧，合成器读到一半就被覆盖。

副作用是**客户端帧率被服务器帧率钳住**：画得比 fps 快的客户端会停在 ⑥
等下一步。这正是想要的节流——服务器永远按自己的节拍工作。

### 异步双缓冲模式（`fps_async = 1`）

x.json 里打开 `"fps_async": 1` 后服务器为每个窗口多分配一块 `ws_g2`。
客户端**始终**渲染到 `ws_g`（帧缓冲式应用如 SDL2 后端缓存了像素指针，
渲染目标不能交替），"翻转"变成一次显式拷贝
（[xwin_flip_locked()](../../system/xwin/libs/x/src/xwin.c)）：

```c
if(xwin->xinfo->update_requested != 0)   // 服务器还没消费上一帧 → 不能覆盖 ws_g2
    return false;
painting = 1;  barrier;
graph_blt(ws_g → ws_g2);                 // 拷进交接缓冲
front_index = 1;                         // 告诉服务器：读 ws_g2
painting = 0;  barrier;
update_requested = 1;                    // 发布，不阻塞，直接返回
```

服务器侧完全一样：`win_comp_src` 因 `front_index == 1` 选 `ws_g2`，
合成完 `x_update_commit` 清标志。客户端下次 present 时若发现标志还没清
（服务器慢了），**不能丢帧**——事件驱动的 widget 应用已经清掉自己的脏标记，
不会再画一次。于是 libx 记一个 `present_pending`，由事件循环里的
`xwin_retry_pending_presents()` 在服务器放行后补发。

两种模式对比：

| | 阻塞模式 | fps_async |
|---|---|---|
| 客户端帧率 | ≤ 服务器 fps | 自由 |
| 每帧像素搬运 | 0 次（合成直读 ws_g） | 1 次（ws_g → ws_g2） |
| 撕裂防护 | 客户端被 park | ws_g2 只在 `update_requested==0` 时写 |
| 内存 | 1 块工作区 | 2 块工作区 |

### `painting` 标志：合成器怎么知道缓冲"可读"

既然合成器直读客户端缓冲，就必须知道客户端此刻是不是正在往里写。
[win_src_stable()](../../system/xwin/drivers/xserverd/xrepaint.c) 的判定：

```c
if(update_requested || !painting)   // 已发布（客户端被 park）或没在画 → 稳定
    return true;
// 否则在画：等，但最多 X_PAINT_TIMEOUT_MS (100ms)
```

为什么要超时：一个画完最后一帧就闲下来的应用会停在 ⑦ 之后，`painting`
永远是 1。它的缓冲其实是完整的，不能让它把整屏重建拖死。

## 22.4 脏区传播：一个窗口变脏，谁跟着变脏

`x_accept_update` 最后一行调 `win_dirty()`，从这里开始脏标记沿 Z 序向上
传播。窗口链表 `win_head → win_tail` 就是自底向上的 Z 序（`push_win` 把
`SYSBOTTOM` 插头、`SYSTOP` 插尾、普通窗口插在 SYSTOP 之下）。

[mark_dirty()](../../system/xwin/drivers/xserverd/xwin.c) 的逻辑：

```
win 变脏
  │
  ├─ 遍历 win 之上的每个可见窗口 top：
  │     r = top->winr ∩ check_r         check_r = 主题有 alpha 边框或阴影 ? win->winr : win->wsr
  │     if r 非空 → top->dirty_mark = true   （被压住的区域要重画）
  │     if r == check_r（win 被 top 完全盖住）
  │        && top 不透明 && top 不需要读桌面 && top 无背景特效
  │        → break                            （再往上的窗口看不到 win，不用传播）
  │
  ├─ mark_dirty_confirm：把 dirty_mark 转成 dirty，并按窗口性质升级：
  │     need_repaint_desktop(v)  → x_dirty(display)   （透明窗口 / 背景特效：要整屏重建）
  │     frame_cuts_ws(v)         → v->frame_dirty     （半透明圆角边框：装饰要重混合）
  │
  └─ 递归对 win->next 做同样的事
```

然后 `win_dirty` 对 `win` 自己做同样的升级判定，最后 `x_repaint_req(display)`
——注意默认只是**增量**请求，整屏重建只由 `need_repaint_desktop` 触发。

两个判定函数的语义
（[xwin.c](../../system/xwin/drivers/xserverd/xwin.c)）：

```c
bool need_repaint_desktop(x, win) {          // 谁的重画需要先把桌面画一遍？
    if(win->xinfo->alpha) return true;       // 窗口内容本身半透明：要混合到干净背景上
    if(NO_FRAME) return false;
    if(win_bg_effect_active(x, win)) return true;  // 失焦背景特效：要把桌面混进整个窗口
    return false;                            // 主题的圆角/阴影 → 不需要！合成器单独处理
}

bool need_repaint_frame(x, win) {            // 桌面重画后谁的装饰要重画？
    if(NO_FRAME && !alpha) return false;
    if(win_bg_effect_active) return true;
    if(theme.frameAlpha && !win_edge_to_edge(win)) return true;  // 半透明边框与下方混合
    return false;
}
```

`need_repaint_desktop` 的注释特别强调了一个曾经的性能坑：它必须用
`win_bg_effect_active()`（主题开了特效 **且** 窗口失焦 **且** 没有
`NO_BG_EFFECT` 风格），而不是只看主题——大多数系统应用都带 `NO_BG_EFFECT`，
只看主题会让每个失焦窗口都升级成整屏重建。

用一个三窗口场景走一遍：

```
Z 序（自底向上）：  A（不透明）   B（不透明，压住 A 右半）   C（半透明 alpha，压住 B 左上角）

A 发布新帧：
  mark_dirty(A):  B ∩ A 非空 → B.dirty_mark；B 不完全盖 A，继续；C ∩ A 非空 → C.dirty_mark
  confirm:        B.dirty；C.dirty，且 C.alpha → need_repaint_desktop → x_dirty(display)  ← 升级成整屏重建
  结果：本帧整屏重建：桌面 → A → B → C

B 发布新帧（C 不存在时）：
  mark_dirty(B):  上面没人
  win_dirty:      B 不透明、无特效 → 不升级
  结果：增量重画：只 draw_win(B)，然后 refresh_shadows_above 修补 B 之上窗口的阴影
```

## 22.5 一帧的合成：`x_repaint` 逐步拆解

[x_repaint()](../../system/xwin/drivers/xserverd/xrepaint.c) 是每个显示器
每步跑一次的核心。按代码顺序拆成十步：

**① 门禁**

```c
if(!display->need_repaint) return;            // 没人要求重画
if(display_busy(&display->display)) return;   // fbdisplayd 还在推上一帧：写进去会撕裂，下步再试
if(!all_win_ready(x)) {                       // 有可见窗口还没发布首帧
    if(display->wait_ready++ < X_WAIT_READY_MAX) return;   // 最多等 4 帧
}
if(display->dirty && 有窗口 !win_src_stable) {           // 整屏重建时不能跳过正在画的窗口（会留洞）
    if(display->paint_wait++ < X_WAIT_READY_MAX) return;   // 也最多等 4 帧
}
display->need_repaint = false;
```

`display_busy` 读的是 fbdisplayd 那块 ctrl 共享内存里的 `busy` 字节，
驱动推帧期间置 1。因为合成直接写 scan-out 缓冲，这一步是防撕裂的第一道闸。

**② 藏光标**

光标不是窗口，而是一层叠加：`hide_cursor()` 把上次保存的"光标下方像素"
（`cursor.saved`）贴回去，记下旧矩形 `cursor_old_rect`。

**③ 擦掉拖拽轮廓**

如果上一步画了拖拽轮廓（`drag_band_valid`），把保存的场景带 `drag_band`
贴回去并记为脏矩形。整屏重建时跳过贴回（反正要重画）。

**④ 桌面**

```c
if(display->dirty) {
    if(!covered_by_opaque_win(x, NULL, display_index, &display->desktop_rect)) {  // 被全屏不透明窗口盖住就省了
        if(draw_desktop(x, display_index) == 0)      // XWM_CNTL_DRAW_DESKTOP，xwm 画壁纸到 display->g
            加脏矩形(desktop_rect)
        else
            desktop_retry = true;                    // xwm 拒绝：什么都没画，保持 dirty 下帧重试
    }
}
```

注释里记录了一个真实 bug：早期版本在 xwm 调用失败时先填黑再返回，结果
黑色永远留在面板上（因为 `display->dirty` 帧末会被清）。现在的原则是
**没画就别刷**。

**⑤ 窗口循环（自底向上）**

```c
for(win = win_head; win; win = win->next) {
    if(!win->ready || !visible || 不在本显示器) continue;
    if(display->dirty) { win->dirty = true; win->shadow_valid = false; }   // 整屏重建：人人重画，阴影重混

    if(!win->dirty && !win->frame_dirty) continue;

    if(win != win_drag && covered_by_opaque_win(x, win, display_index, &winr)) {
        // 被上方不透明窗口完全盖住：画了也是白画
        win->dirty = win->frame_dirty = false;
        win->shadow_valid = false;
        x_update_commit(x, win);           // 但要把缓冲还给客户端，别让它白等
    }
    else if(!display->dirty && !win_src_stable(x, win)) {
        paint_retry = true;                // 增量路径下客户端正在画：屏幕上还是旧帧，下步再取
    }
    else {
        draw_win(display->g, x, win, &win_dirty);          // 22.6 节
        if(!display->dirty)
            refresh_shadows_above(x, win, &win_dirty);     // 修补上方窗口被这次重画抹掉的半透明部分
        加脏矩形(win_dirty);
        x_update_commit(x, win);                           // 最后一个读者读完，放行客户端
    }
}
```

**遮挡剔除**（`covered_by_opaque_win`）判定"上方是否有一个不透明窗口的
工作区完全包含本矩形"。贴边窗口（最大化/全屏）用 `winr` 判定——它的标题栏
是 xwm 画的实心装饰，同样能遮盖。

**⑥ 拖拽轮廓叠加**

正在拖动/缩放窗口时，不重画窗口本身，只画一个轮廓：

```c
get_drag_frame_rect(x, &r);
r 向外扩 frameW 像素;                    // xwm 的 graph_frame 画在矩形外侧，不扩就擦不干净（残影）
graph_blt(display->g → drag_band);       // 先存下轮廓下面的场景
draw_drag_frame(x, display_index);       // XWM_CNTL_DRAW_DRAG_FRAME
加脏矩形(r);
```

一步拖拽的成本 = 两次小带位块（擦旧 + 存新）+ 一次 xwm 轮廓 + 一小块 flush，
而不是整屏重画。

**⑦⑧ 画光标、记光标脏矩形**

`refresh_cursor()`：先把新位置下方的像素存进 `cursor.saved`，再画光标位图。
旧矩形和新矩形都进脏矩形表。

**⑨ 更新显示器状态**

```c
display->dirty = desktop_retry;          // 通常清零；xwm 拒画桌面时保持
if(paint_retry) display->need_repaint = true;   // 有窗口欠一帧
```

**⑩ 交付脏矩形**

```c
if(do_flush && dirty_num > 0) {
    pack_dirty_rects(dirty_rects, dirty_num, display_dirty, DISPLAY_DIRTY_MAX);  // 22.7 节
    display_set_dirty(&display->display, display_dirty, display_num);
    display->flush_inflight = true;
    display->pending_flush = true;       // 真正的 flush IPC 在锁外做
}
```

## 22.6 `draw_win`：一个窗口怎么落到屏幕上

[draw_win()](../../system/xwin/drivers/xserverd/xrender.c) 是 `xrender.c`
的主角，把一个窗口的内容 + 装饰按主题混到 `display->g` 上。分三段。

### 第一段：准备装饰环 `prepare_win_content`

```c
win_mark_frame_dirty(x, win);        // 背景特效随内容变 / 桌面重画后半透明边框要重画
if(毛玻璃 && (整屏重建 || 位置变了)) capture_backdrop(x, win, &winr);   // 抓干净背景

prepare_win_content:
    if(frame_dirty) clear_frame_ring(win);              // 只清工作区外的环（半透明主题要从 0 开始混）
    if((背景特效 || frame_cuts_ws) && (dirty || frame_dirty))
        graph_blt(win_comp_src(win) → frame_g 的工作区位置);   // 只有这两种情况把内容拷进 frame_g
    if(!frame_dirty) return;                            // 装饰没变：frame_g 里的旧画面直接复用，xwm 不被打扰
    if(NO_FRAME 且无特效) return;  if(全屏 且无特效) return;
    ipc_call_wait(xwm_pid, XWM_CNTL_DRAW_FRAME, {shm_id, w, h, xinfo, top?});   // 第 20 章
```

关键的省钱点：**`frame_g` 是持久的**。窗口内容变而装饰不变时
（最常见的情况——应用在刷新自己的画面），`frame_dirty == false`，
xwm 一次都不用被调用。

### 第二段：分源位块 `blit_win_part`

[blit_win_part()](../../system/xwin/drivers/xserverd/xrender.c) 把一个
frame 坐标系里的矩形 `d` 贴到屏幕上，**工作区部分从客户端缓冲取、
环的部分从 `frame_g` 取**：

```
     frame_g（只有环有内容）           win_comp_src（客户端缓冲）
   ┌────────────────────┐            ┌──────────────┐
   │ top 带              │            │              │
   │ ┌────────────────┐ │            │  整块工作区    │
   │ │  (空)          │ │  合成 ──►  │              │
   │ └────────────────┘ │            └──────────────┘
   │ left 带 / right 带  │
   │ bottom 带           │
   └────────────────────┘
   d ∩ 工作区  ← 从客户端缓冲    d − 工作区 ← 拆成最多 4 条带从 frame_g 取
```

只有背景特效或 `frame_cuts_ws` 生效时（整幅画面已经在 `frame_g` 里），
才退化成单源拷贝。

### 第三段：按主题分四条路径

`draw_win` 中间那一大段 if/else 是整个文件最难读的部分，其实只在回答
一个问题：**哪些像素是半透明的，它们能不能再混合一次？**

半透明像素（阴影带、圆角外侧的抗锯齿弧、半透明窗口内容）用 `graph_blt_alpha`
混到背景上。如果背景没变而你把它**再混合一次**，alpha 会叠 alpha——阴影
越来越黑、圆角越来越脏，直到下次整屏重建才恢复（用户看到的是"闪一下"）。
所以每个窗口用 `shadow_valid + shadow_rect` 记住"我的半透明部分已经在
这个位置混合过了"，位置没变就**只刷不透明像素**。

| 路径 | 条件 | 每次内容更新做什么 | 每次"落位"（位置/Z 序/焦点变）做什么 |
|---|---|---|---|
| (a) 半透明窗口 | `xinfo->alpha` | 环的直边普通拷贝；中间和四角只拷 **不透明像素**（`blit_win_area_opaque`） | 整幅 `blt_alpha` 一次，记 `shadow_valid` |
| (b) 不透明 + 阴影 | `theme.shadow > 0 && !frameAlpha` | `inner`（去掉右/下阴影带）普通 `blit_win_part` | 右带、下带 `blt_alpha` 一次 |
| (c) 半透明圆角边框 | `frame_cuts_ws` | 环的四条直边普通拷贝，`mid` 走 `blit_win_part`；四个角方块只拷不透明像素 | 四角 + 阴影带 `blt_alpha` 一次 |
| (d) 其他 | — | 整个 `dmg` 普通 `blit_win_part` | — |

路径 (c) 里为什么四角要"只拷不透明像素"：圆角半径 `round` 常常大于边框宽
`frameW`，角方块里既有半透明的弧形抗锯齿，也有**实实在在的工作区内容和
标题栏**。内容变了必须跟着刷，弧又不能再混——逐像素看 alpha 是否为 0xff
是唯一两全的办法。

`shadow_valid` 在哪里失效（也就是"落位"的定义）：

- `push_win`（Z 序变化）、`try_focus/x_unfocus`（边框换色）：`shadow_valid = false`；
- `x_repaint` 整屏重建：所有窗口 `shadow_valid = false`；
- 被上方窗口完全盖住时：`shadow_valid = false`（那里的阴影已被覆盖）；
- `memcmp(shadow_rect, winr)` 不等：窗口移动或缩放。

### 收尾：修补上方窗口 `refresh_shadows_above`

增量重画一个下方窗口后，它刚重画的区域里可能压着上方窗口的阴影带或圆角弧
——那些半透明像素被新内容抹掉了。[refresh_shadows_above()](../../system/xwin/drivers/xserverd/xrender.c)
沿链表向上，对每个上方窗口：

- 先 `capture_backdrop`（这块区域现在恰好是干净背景，毛玻璃要用）；
- 半透明窗口：把与重画区域相交的部分整幅重新 `blt_alpha`；
- 其他窗口：若 `shadow_valid`，只把**重画区域 ∩ 阴影带/角方块**重新混合
  ——绝不碰没被抹掉的部分（再混就叠 alpha）；若 `!shadow_valid`，
  直接标 `dirty` 让它自己整体重画。

整屏重建时不需要这一步：每个窗口都自底向上重画，自己把带子混到新鲜背景上。

## 22.7 脏矩形：从 16 个到 4 个再到面板

### 收集：`x_repaint_add_dirty`

`x_repaint` 用一个最多 16 项的数组收集本帧碰过的屏幕矩形
（[x_repaint_add_dirty()](../../system/xwin/drivers/xserverd/xrepaint.c)）：

1. 裁到屏幕内；
2. 已被某项完全包含 → 丢弃；
3. 与某项相交或相邻 → 并进去，然后**级联**：合并后的矩形可能又与别的项
   相交，循环再并，直到稳定。注释里记录了一个交换删除（swap-with-last）时
   索引 `i` 跟丢的 bug，是写这类合并算法的经典陷阱；
4. 表满 → 并进第 0 项。

### 打包：`pack_dirty_rects`

fbdisplayd 的控制块只有 `DISPLAY_DIRTY_MAX = 4` 个槽
（[display.h](../../system/gui/libs/display/include/display/display.h)），
超过就得把整屏推过去。所以 [pack_dirty_rects()](../../system/xwin/drivers/xserverd/xrepaint.c)
做贪心合并：对每个多出来的矩形，找"并进去后面积增长最小"的槽并入。

### 推送：ctrl 共享内存与 `flush`

```c
typedef struct {            // display_ctrl_t，fbdisplayd 与 xserverd 共享
    uint8_t  busy;          // 驱动推帧时置 1（x_repaint 门禁读它）
    uint8_t  dirty_num;     // 本次 flush 的脏矩形个数，0 = 整屏
    grect_t  dirty[4];
} display_ctrl_t;
```

`display_set_dirty` 把矩形写进 ctrl，`display_flush` 发一条 `vfs_flush`
给驱动；驱动侧 [displayd.c 的 do_flush()](../../system/gui/libs/displayd/src/displayd.c)
取走矩形、清零 `dirty_num`、置 `busy = 1`，能按矩形推就按矩形推
（`flush_dirty`），否则整屏，最后 `busy = 0`。

**脏矩形是单槽的**——这就是 `flush_inflight` 存在的理由。22.8 节的光标
快速路径也会写这个槽；如果帧路径写好了 4 个矩形、还没等驱动读走，
光标路径把它覆盖成 2 个光标矩形，驱动就只推光标，被擦掉的拖拽轮廓
永远到不了面板（残影）。所以从 `display_set_dirty` 到 flush 落地整段时间
`flush_inflight = true`，快速路径看到它就退让。

### 为什么 flush 在锁外

看 [xserver_step()](../../system/xwin/drivers/xserverd/xserver_dev.c)：

```c
x_server_lock_enter();
    x_poll_updates(x);
    for each display: x_repaint(x, i);       // 只设 pending_flush，不发 IPC
x_server_lock_leave();

for each display:
    if(pending_flush) {
        display_flush(&display->display, true);   // 阻塞等驱动推完，锁外
        flush_inflight = false; pending_flush = false;
    }
```

`display_flush(…, true)` 要等驱动把像素推到面板——在真机（如 raspix 的
持续扫描帧缓冲）上这可能是几毫秒。若在锁内等，所有 IPC 处理线程
（鼠标输入、窗口命令）都要跟着停。`waiting = true` 又是必要的：合成写的
就是 scan-out 缓冲，下一帧不能在驱动还在读时开始覆盖。

## 22.8 绕开帧节拍：光标与拖拽的快速路径

主循环按 `fps`（默认 30）节拍走，意味着鼠标移动的视觉反馈最坏要等 33ms。
两条快速路径把最廉价的交互反馈从帧节拍里拆出来：

**光标即时重画** [x_cursor_redraw_now()](../../system/xwin/drivers/xserverd/xrepaint.c)
在**输入 IPC 处理线程**里直接调用（它持有服务器锁）：

```c
if(!active || flush_inflight || display_busy()) return false;   // 有帧在推：退让，留给帧路径
hide_cursor(x);   refresh_cursor(x);                              // 就两次小位块
display_set_dirty(2 个光标矩形);
display_flush(&display->display, false);                          // 非阻塞 flush（IPC_NON_RETURN）
```

它绝不阻塞、绝不在推帧期间碰缓冲。退让时 `cursor_task` 保持为真，
下一帧 `x_repaint` 补画。于是光标以**事件速率**而不是帧速率跟手。

**拖拽轮廓叠加**在 22.5 节 ⑥ 已经讲了机制；配套的是主循环节拍变化：

```c
uint32_t quantum = 1000 / fps;                       // 正常 33ms
if(x->current.win_drag != NULL && drag_state != 0)
    quantum = X_DRAG_STEP_MS;                        // 拖拽期间 8ms（~125Hz）
```

拖拽处理函数只调 `x_repaint_req` 而不是 `x_dirty`——拖拽期间窗口本身
不重画，只有轮廓在动。等松手时才发 `XEVT_WIN_MOVE/RESIZE` 给应用，
应用改几何、服务器重建 `frame_g`，这时才整屏重建一次。

## 22.9 主循环与锁：谁在什么时候碰这些状态

把 [xserver_step()](../../system/xwin/drivers/xserverd/xserver_dev.c) 完整看一遍：

```c
int xserver_step(vdevice_t* dev, void* p) {
    tik = now;
    x_server_lock_enter();                    // multi_task=0 时用 ipc_disable() 代替
        if(xwm_changed && check_xwm) x_dirty(-1);      // 换了 xwm：整屏重建
        for each win: xwin_revalidate_geometry(x, win); // 第 20 章的 winr==wsr 自愈
        check_wins(x);                                  // 回收进程已死的窗口
        x_poll_updates(x);                              // ① 接收发布的帧
        for each active display: x_repaint(x, i);       // ② 合成
    x_server_lock_leave();

    for each display with pending_flush:
        display_flush(&display->display, true);         // ③ 推屏（锁外阻塞）

    sleep 到 quantum 用满                                // 节拍：1000/fps 或拖拽时 8ms
}
```

`x_server_lock` 是一把粗粒度的服务器状态锁。`IPC_MULTI_TASK` 模式下
（第 13 章提过），输入事件、窗口命令由内核工作线程并发派发，它们与主循环
共享 `x_t`、窗口链表、事件池，所以每个 vdevice 回调都先拿锁。
锁内允许向 xwm / vfsd 发出站 IPC（它们不需要回头拿这把锁，不会成环），
但两件慢事被刻意放在锁外：阻塞 flush 和节拍睡眠——它们不能拖住输入派发。

客户端与服务器之间**没有锁**，只有 `xinfo_t` 里的 volatile 标志 + 内存屏障
+ `proc_block_by/proc_wakeup_by` 组成的握手。这是整套设计跨进程零拷贝的前提，
也是每条 `__sync_synchronize()` 注释都写得那么长的原因。

## 22.10 时序常量一览

| 常量 | 值 | 位置 | 保护什么 |
|---|---|---|---|
| `fps` | 30（x.json） | `xserver_step` | 主循环节拍 |
| `X_DRAG_STEP_MS` | 8 | `xserver_dev.c` | 拖拽期间的节拍 |
| `X_WAIT_READY_MAX` | 4 帧 | `x_repaint` | 等窗口就绪 / 等客户端画完，不能无限等 |
| `X_NOT_READY_TIMEOUT_MS` | 500 | `xserver.h` | 一个卡住的窗口不能永远拖住全屏；也是向它重发 `XEVT_WIN_REPAINT` 的节奏 |
| `X_PAINT_TIMEOUT_MS` | 100 | `xrepaint.c` | `painting` 标志卡住（客户端闲下来）时停止等待 |
| `X_ACCEPT_TIMEOUT_MS` | 200 | `xwin.c` | 接收了但一直没合成的帧（显示器忙/窗口被盖）：把缓冲还给客户端、丢弃脏标记并请它再画一次（`x_accept_abandon`） |
| `X_REPAINT_DIRTY_MAX` | 16 | `xrepaint.c` | 每帧收集的脏矩形上限 |
| `DISPLAY_DIRTY_MAX` | 4 | `display.h` | 交给驱动的脏矩形上限 |

这些超时共同的思想：**不相关的进程绝不能互相拖死**。一个客户端卡住、
停笔不画、或者被盖住，最坏只是它自己的窗口显示旧内容，绝不能让整块屏幕
停下来等它。

## 22.11 动手练习

1. 在 `x_repaint` 里加一行日志打印 `dirty_num` 和每个矩形，然后分别做
   "拖动一个窗口""在终端里打字""切换焦点"，看每种操作产生几个脏矩形、
   有没有触发整屏重建（`display->dirty`）；
2. 把 x.json 的 `fps_async` 从 0 改成 1，用一个高帧率应用（如 `xDemo` 或
   nesemu）对比两种模式下服务器 fps 与应用 fps 的关系；
3. 读 `mark_dirty`，画出四个互相部分重叠的窗口，手算最底层窗口变脏后
   哪些窗口的 `dirty` 会被置位、传播在哪里 `break`；
4. 把 `draw_win` 路径 (b) 里 `bands_ok` 的判断改成永远 `false`，
   连续刷新一个带阴影的窗口，观察阴影为什么越来越黑——这就是 alpha 叠加；
5. 思考题：`x_update_commit` 若提前到 `x_poll_updates` 里调用，会出什么问题？
   提示：客户端被放行后马上会做什么，合成器此时在读哪块内存；
6. 思考题：`x_cursor_redraw_now` 为什么在 `flush_inflight` 时必须退让，
   而不是等一等再画？提示：它跑在哪个线程里。

## 22.12 本章小结

- 一帧的路径：客户端 `xwin_repaint` 画进共享内存 `ws_g` 并置位握手标志 →
  服务器 `x_poll_updates` 接收（不拷贝）→ `x_repaint` 自底向上合成到
  scan-out 缓冲 → `draw_win` 后 `x_update_commit` 放行客户端 →
  锁外 `display_flush` 只推脏矩形；
- 两级脏标记：`x_dirty` 整屏重建（桌面 + 所有窗口），`x_repaint_req` 增量
  重画（只画 `dirty` 窗口）；只有半透明窗口与失焦背景特效才会升级成整屏；
- 脏区沿 Z 序向上传播（`mark_dirty`），遇到完全遮盖的不透明窗口停止；
  被完全盖住的窗口直接剔除不画（`covered_by_opaque_win`）；
- 每窗口四块画布：`ws_g`（客户端渲染目标）、`ws_g2`（fps_async 交接）、
  `frame_g`（持久的装饰环，装饰不变时 xwm 不被调用）、`backdrop`（毛玻璃背景）；
- `blit_win_part` 分源搬运：工作区来自客户端缓冲，装饰环来自 `frame_g`；
- 半透明像素只在"落位"时混合一次（`shadow_valid/shadow_rect`），之后只刷
  不透明像素，避免 alpha 叠加发黑；下方窗口增量重画后由 `refresh_shadows_above`
  修补上方窗口被抹掉的半透明部分；
- 脏矩形 16 → 级联合并 → 贪心打包成 4 个 → 写进 ctrl 共享内存 → 驱动按矩形推；
  `flush_inflight` 保护这个单槽不被光标快速路径覆盖；
- 光标快速路径和拖拽轮廓叠加绕开帧节拍，让交互反馈跟手；
- 所有等待都有上限（`X_*_TIMEOUT_MS`），任何一个客户端都拖不死整块屏幕。

至此你已经能把第 13 章的"图形栈全景"、第 20 章的"窗口穿衣"和本章的
"像素搬运"连成一条完整的线：应用画一笔，屏幕上到底发生了什么，
每一步都能在源码里指出来。
