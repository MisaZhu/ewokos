# 20 The xwm Window Manager: Mechanism and Implementation

> Language: **English** | [中文](20-xwm.zh.md)
>
> Goal: go deep into the most "microkernel-flavored" design in EwokOS's
> window system — the window manager xwm. After reading you'll understand:
> why the title bar, borders, and shadows are not drawn by the X server;
> what the IPC protocol between xwm and xserverd looks like; and how to
> write your own window manager by hand (reskinning).
> Recommended reading first: [Ch. 09 IPC](09-ipc.md) and
> [Ch. 13 The Graphics System](13-graphics.md).

## 20.1 What xwm Is: Kicking "Policy" Out of the Server

Recall the graphics stack from Ch. 13: `xserverd` is the window server,
managing the window tree, compositing, and input dispatch. But if you
rummage through [xserverd's source](../../system/xwin/drivers/xserverd/),
you'll find a strange fact: **it doesn't know what a title bar looks like —
it doesn't even know how tall one is**.

A window's "look" — the title bar, the close/maximize buttons, the borders,
the rounded corners, the shadows, the desktop wallpaper — is all the
responsibility of another independent process: **xwm (the X Window
Manager)**.

```
┌─────────────┐  register: dev_cntl(/dev/x, X_DCNTL_SET_XWM) ┌──────────────┐
│   xserverd   │ ◄───────────────────────────────────────── │     xwm      │
│ (mechanism)  │                                            │   (policy)   │
│ window tree/ │  reverse IPC: XWM_CNTL_DRAW_FRAME/...      │ title bar/   │
│ compositing/ │ ─────────────────────────────────────────► │ buttons/     │
│ input/Z-order│        (drawn on shared-memory canvases)    │ borders/     │
└─────────────┘                                            │ shadows/     │
                                                           │ wallpaper/   │
                                                           │ themes       │
                                                           └──────────────┘
```

This is the classic **separation of mechanism and policy**:

- **Mechanism** (xserverd): how windows stack, who events go to, when a
  frame is composited — these rules are stable and stay in the server;
- **Policy** (xwm): what a window "should look like" — a matter of taste
  you want to change every day, so it's made a replaceable independent
  process.

The benefits are immediate:

1. **Reskinning = swapping a process**. The repo ships four ready-made
   window managers:
   [opencde](../../system/xwin/xwm/opencde/) (CDE style, the system
   default), [ewokwm](../../system/xwin/xwm/ewokwm/) (a modern style with
   rounded corners + shadows), `mac1984`/`openlook` (in
   [sw.extra/x/xwm/](../../sw.extra/x/xwm/), retro Macintosh and OpenLook
   styles). Change one line of a boot script and the whole face changes;
2. **If xwm crashes, the desktop survives**. After detecting xwm's death,
   xserverd automatically degrades into "undecorated mode" and keeps
   running (details in §20.7); the moment a new xwm starts, everything
   recovers — the microkernel philosophy continued into the graphics
   system;
3. **Writing a window manager becomes writing an ordinary app**. No need to
   touch a single line of the server — inherit one C++ class and override a
   few draw functions (hands-on in §20.6).

In the real X11 world, the relationship between twm/mwm/i3 and the X Server
is exactly the same; EwokOS replicates this idea completely in under 400
lines of [xwm.c](../../system/xwin/libs/x/src/xwm.c) — a perfect study
sample.

## 20.2 Registration and Discovery: How xwm "Takes Office"

xwm is an ordinary userland process, brought up by the boot script
[xinit.rd](../../system/xwin/etc/x/xinit.rd):

```sh
@export XTHEME=opencde
@/bin/ipcserv /sbin/x/xwm_opencde     # started via ipcserv; wait for its registration before continuing

@/bin/x/xlauncher &                   # only then start the desktop launcher and other apps
```

Its `main` function is surprisingly short
([ewokwm/xwm.cc](../../system/xwin/xwm/ewokwm/xwm.cc)):

```c
int main(int argc, char** argv) {
    EwokWM xwm;                       // construct the window-manager object (fills the function table)
    xwm.loadTheme(getenv("XTHEME"));  // load the theme per the XTHEME environment variable
    xwm.run();                        // enter the service loop; from here on, wait for xserverd's calls
    return 0;
}
```

`run()` ultimately lands in [xwm_run()](../../system/xwin/libs/x/src/xwm.c),
doing two key things:

```c
void xwm_run(xwm_t* xwm) {
    // 1. turn itself into an IPC service (recall Ch. 09: ipc_serv_run registers the message handler)
    ipc_serv_run(handle, NULL, xwm, IPC_NON_BLOCK);

    // 2. volunteer to xserverd (the /dev/x device): "I'm the window manager!"
    dev_cntl("/dev/x", X_DCNTL_SET_XWM, NULL, NULL);

    while(true) usleep(100000);   // the main thread sleeps; the IPC thread works
}
```

Upon receiving `X_DCNTL_SET_XWM`, xserverd records three things
([xserver_dev.c](../../system/xwin/drivers/xserverd/xserver_dev.c)):

```c
x->xwm_pid = from_pid;                     // who the xwm is
x->xwm_uuid = proc_get_uuid(from_pid);     // its "ID card" (guards against pid reuse)
x->xwm_changed = true;                     // mark: a new xwm has arrived; geometry must be recomputed
```

Note `xwm_uuid`: pids get recycled and reused — if xwm crashed and some
unrelated process happened to grab the same pid, a server that only trusted
the pid would send drawing requests to an innocent. So before every call,
[check_xwm()](../../system/xwin/drivers/xserverd/xserverd.c) verifies the
identity:

```c
bool check_xwm(x_t* x) {
    if(x->xwm_pid < 0)
        return false;
    if(proc_check_uuid(x->xwm_pid, x->xwm_uuid) == x->xwm_uuid)
        return true;    // the pid is alive and is indeed that same process
    x->xwm_pid = -1;    // dead / replaced: deregister and enter undecorated mode
    return false;
}
```

The **reversal of direction** is the most delicious point here: normally
apps call xserverd, but once xwm registers, **xserverd conversely becomes
xwm's client** — actively `ipc_call`ing over every time a window decoration
needs drawing. Who provides the service and who initiates the call are
completely decoupled in the IPC world.

## 20.3 The IPC Protocol: Seven Commands Tell the Whole Mechanism

All communication between xserverd and xwm consists of just 7 commands
([x/xwm.h](../../system/xwin/libs/x/include/x/xwm.h)), in two classes:

| Command | Class | Effect | When it's called |
|---|---|---|---|
| `XWM_CNTL_GET_WIN_SPACE` | query | given the workspace rect wsr, compute the outer frame winr with decorations added | window creation / resizing / state switches |
| `XWM_CNTL_GET_FRAME_AREAS` | query | return the 5 hot-zone rects: title bar / close / minimize / maximize / resize corner | after geometry changes; the server caches them for hit testing |
| `XWM_CNTL_GET_MIN_SIZE` | query | the window's minimum size (don't let the user drag a window into nothing) | while drag-resizing |
| `XWM_CNTL_DRAW_FRAME` | draw | draw a window's full set of decorations (title bar, buttons, borders, shadow) | when the decorations are dirty (frame_dirty) |
| `XWM_CNTL_DRAW_DESKTOP` | draw | draw the desktop background (wallpaper/pattern) | when the desktop is dirty |
| `XWM_CNTL_DRAW_DRAG_FRAME` | draw | draw the dashed outline while dragging/resizing | every frame during a drag |
| `XWM_CNTL_SET_THEME` | config | the server pushes new theme parameters to xwm | when changing themes at runtime |

The dispatch code is a plain switch
([handle() in xwm.c](../../system/xwin/libs/x/src/xwm.c)) — exactly like
the IPC services you wrote in Ch. 09.

The **query-class** commands embody the division "policy in xwm, mechanism
in the server". Take the most important one, `GET_WIN_SPACE`: an app says
"I want a 300x200 workspace"; the server can't compute the outer frame
itself, so it asks xwm
([xwin_cmd.c](../../system/xwin/drivers/xserverd/xwin_cmd.c)); xwm adds
the space for the title bar, borders, and shadow per the theme parameters
([getWinSpace() in XWM.cc](../../system/xwin/libs/x++/src/XWM.cc)):

```c
void XWM::getWinSpace(int style, int state, grect_t* xr, grect_t* winr) {
    *winr = *xr;                             // start from the workspace rect
    if(has_title_bar) {                      // add a title bar on top
        winr->y -= xwm.theme.titleH;
        winr->h += xwm.theme.titleH;
    }
    if(has_frame) {                          // add borders on all sides, leaving shadow at bottom-right
        winr->x -= frameW;   winr->y -= frameW;
        winr->w += 2*frameW + shadow;
        winr->h += 2*frameW + shadow;
    }
    // in maximized/fullscreen state, frameW and shadow count as 0 —
    // an edge-to-edge window has no room for decorations
}
```

From then on, every window has two rectangles, running through the entire
window system:

```
winr (the window's outer frame; xwm has the final say)
┌───────────────────────────────┐
│ title bar   [close] [max] [min]│ ← titleH
│ ┌───────────────────────────┐ │
│ │                           │ │
│ │   wsr (the workspace; the  │ │
│ │   app only cares about here)│ │
│ │                           │ │
│ └───────────────────────────┘ │
└───────────────────────────────┘▒ ← the shadow band on the right/bottom edges
 ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
```

The app only ever draws its content inside wsr; the "decoration ring"
between wsr and winr belongs to xwm.

The **hot-zone query** (`GET_FRAME_AREAS`) solves the input problem:
clicking on the title bar should drag, clicking the close button should
close — but only xwm knows where these areas are. The approach: after a
window's geometry changes, the server calls this command once and **caches**
the 5 rectangles in the window structure
(`win->r_title/r_close/r_min/r_max/r_resize`); afterwards every mouse event
is hit-tested locally
([get_win_frame_pos() in xinput.c](../../system/xwin/drivers/xserverd/xinput.c)):

```c
static int get_win_frame_pos(x_t* x, xwin_t* win) {
    if(the point is inside win->r_close)  return FRAME_R_CLOSE;
    if(the point is inside win->r_min)    return FRAME_R_MIN;
    if(the point is inside win->r_max)    return FRAME_R_MAX;
    if(the point is inside win->r_title)  return FRAME_R_TITLE;
    if(the point is inside win->r_resize) return FRAME_R_RESIZE;
    return -1;
}
```

Think about why caching instead of asking xwm on every mouse move — mouse
events come at dozens to hundreds per second; a cross-process IPC round
trip for each would be unacceptable in both latency and overhead.
**Geometry is low-frequency-change, high-frequency-read data: query once,
cache locally** — a recurring technique throughout EwokOS.

Everything after a hit is done locally in the server (mechanism!): hitting
the title bar enters the `X_win_DRAG_MOVE` dragging state; hitting the
resize corner enters `X_win_DRAG_RESIZE`; on release, the app owning the
window receives `XEVT_WIN_MOVE` / `XEVT_WIN_RESIZE` / `XEVT_WIN_CLOSE` /
`XEVT_WIN_MAX` events
([mouse_xwin_handle() in xinput.c](../../system/xwin/drivers/xserverd/xinput.c)).
**xwm never sees any input event the whole time** — it is only responsible
for "drawing" and "measuring", which keeps it so simple that concurrency
bugs are impossible.

## 20.4 Zero-Copy Drawing: Three Shared-Memory Canvases

The draw-class commands are the performance-sensitive path. Recall Ch.
13's arithmetic: one fullscreen frame is about 8MB of pixels — copying over
IPC is death. So all of xwm's drawing happens on **shared-memory
canvases**, and IPC only carries an shm_id plus window info:

```
the xserverd process                         the xwm process
┌──────────────────────┐                   ┌──────────────────────┐
│ display->g (the whole-│◄── the same physical memory ──►│ desktop_g (shmat-mapped)│
│  screen canvas)       │                   │                      │
│ win->frame_g          │◄── the same physical memory ──►│ frame_g   (shmat-mapped)│
│  (decoration layer)   │                   │                      │
│ ws_g published by     │◄── the same physical memory ──►│ ws_g      (shmat-mapped)│
│  the client           │                   │                      │
└──────────────────────┘                   └──────────────────────┘
         ▲                                          │
         └────── IPC only passes shm_id + xinfo_t ───┘
```

The three canvases each have their job:

1. **desktop_g — the fullscreen compositing canvas**. This is the very
   memory xserverd uses for compositing and ultimately flushes to the
   screen. Drawing the desktop wallpaper, the drag outline, and window
   shadows (shadows fall *outside* the window!) all happen directly on it.
   The xwm side caches it
   ([fetch_desktop_graph()](../../system/xwin/libs/x/src/xwm.c)): as long
   as the shm_id and size are unchanged, the previous mapping is reused —
   no shmat/shmdt per frame;

2. **frame_g — the per-window decoration canvas**. Allocated by the server
   at winr's size (carried to xwm via `xinfo_t.frame_g_shm_id`); the title
   bar, buttons, and borders are drawn here. Why must decorations be a
   separate layer instead of drawn straight onto the screen? Because
   during compositing the decoration ring and the window content must
   participate together in the Z-order occlusion computation — the
   decorations must be part of the window itself. And it's persistent:
   when the window isn't dirty, the compositor directly reuses the
   previously drawn frame_g, and xwm isn't disturbed at all;

3. **ws_g — the workspace content published by the client**. xwm normally
   doesn't touch it; only two kinds of theme effects need to read it:
   translucent rounded borders must blend with the content, and the
   background effect of unfocused windows (frosted glass / Gaussian blur)
   needs the content to blur. The comments specifically spell out the
   tear-free precondition: the server only issues `DRAW_FRAME` when the
   client has "put down its pen" (the painting flag is idle and the update
   handshake has stopped), so what xwm reads is always a complete frame.

The [xinfo_t](../../system/xwin/libs/x/include/x/xcntl.h) carried by a
`DRAW_FRAME` request is the protocol's "business card", bringing along
everything needed to draw the decorations:

```c
typedef struct {
    int32_t  frame_g_shm_id;    // the decoration canvas's shared-memory id
    int32_t  ws_g_buffer_shm_id;// the workspace content's shared-memory id
    uint32_t style;             // style bits: NO_FRAME/NO_TITLE/NO_RESIZE/...
    uint32_t state;             // state: NORMAL/MAX/FULL_SCREEN/...
    bool     focused;           // is it the focused window (bright or gray colors)
    grect_t  wsr;               // the workspace rect (screen coordinates)
    grect_t  winr;              // the window outer-frame rect (screen coordinates)
    char     title[XWIN_TITLE_MAX]; // the title text
    ...
} xinfo_t;
```

## 20.5 One Complete DRAW_FRAME: How Decorations Are Drawn

Reading the code at both ends side by side is one complete journey.

**The server side**
([prepare_win_content() in xrender.c](../../system/xwin/drivers/xserverd/xrender.c)):

```c
if(win->frame_dirty)
    clear_frame_ring(win);        // clear only the "decoration ring" outside the workspace
                                  // (translucent themes must blend from 0)

// frameless windows skip xwm; fullscreen windows stick to the edges, no decorations to draw
if((style & XWIN_STYLE_NO_FRAME) && !bg_effect)  return;
if(state == XWIN_STATE_FULL_SCREEN && !bg_effect) return;

if(!check_xwm(x)) return;         // no xwm? skip decorations; the window runs naked

proto_t in;
PF->format(&in, "i,i,i,m",        // pack: the fullscreen canvas's shm_id, width, height, xinfo_t
    display->g_shm_id, display->g->w, display->g->h,
    win->xinfo, sizeof(xinfo_t));
ipc_call_wait(x->xwm_pid, XWM_CNTL_DRAW_FRAME, &in);   // synchronously wait for xwm to finish drawing
```

**The xwm side**
([draw_frame() in xwm.c](../../system/xwin/libs/x/src/xwm.c)) schedules
the callbacks in the function table in a fixed order, handling every
style/state combination:

```c
map the three canvases (desktop_g / frame_g / ws_g)
if(style doesn't have NO_FRAME) {
    first call get_title/get_close/get_max/get_min/get_resize/get_frame to compute the areas
    if(has a title bar && not fullscreen) {
        draw_title(...);                       // ① the title bar
        if(resizable) { draw_max(); draw_min(); } // ② maximize/minimize buttons
        draw_close(...);                       // ③ the close button
    }
    if(not maximized && not fullscreen) {
        draw_frame(...);                       // ④ the borders (rounded corners happen here)
        if(resizable) draw_resize(...);        // ⑤ the resize handle at bottom-right
        if(the theme has shadows) draw_shadow(...); // ⑥ the shadow (drawn onto desktop_g)
    }
}
if(bg effects allowed && unfocused)
    draw_bg_effect(...);                       // ⑦ the unfocused effect (frosted glass etc.)
unmap frame_g / ws_g (the desktop_g cache stays)
```

An engineering detail worth noting: **the combination logic of style bits
and states is concentrated in this one place**. `XWIN_STYLE_NO_TITLE` only
removes the title bar and buttons; the borders are still drawn.
`XWIN_STATE_MAX` keeps the title bar but removes the borders and shadow (an
edge-to-edge window has no decoration space). `XWIN_STATE_FULL_SCREEN`
draws nothing. Concrete WM implementations (EwokWM etc.) needn't worry
about these combinations anymore — they only care about "given a rectangle,
how do I draw it beautifully".

## 20.6 The Implementation Method: Three Layers of Code for a Window Manager

xwm's implementation is split into three clear layers; you only need to
write the top one:

```
layer 3  your WM        EwokWM / OpenCDEWM …   override draw/get virtuals (~300 lines)
layer 2  the x++ base   XWM.cc                 default geometry + default drawing + theme loading
layer 1  libx protocol  xwm.c                  the IPC service, shm mapping, command dispatch (C function table)
```

**Layer 1** is the pure-C
[xwm_t function table](../../system/xwin/libs/x/include/x/xwm.h) — 17
function pointers (8 gets, 9 draws/updates); whoever fills one gets called,
and unfilled ones use fallback logic. This layer has nothing to do with
C++; in theory you could write a WM in pure C.

**Layer 2**, the [XWM base class](../../system/xwin/libs/x++/src/XWM.cc),
wires the function table one by one onto C++ virtual functions (static
trampolines like `xwm.draw_title = draw_title;` in the constructor), and
provides a complete, usable set of defaults: the default title-bar layout
(is the close button on the left or right? `getClose/getMax/getMin` have
the say), default button glyphs, the default rectangular shadow, and the
desktop pattern's tile/center/fit modes. **So a minimal WM doesn't need a
single line of drawing code** — with all defaults you get a plain square
gray theme.

**Layer 3**: reading [EwokWM](../../system/xwin/xwm/ewokwm/EwokWM.cc) is
enough — it overrides only the parts it cares about:

- `drawTitle`: title text centered, bright/gray by focus (the base class's
  `getColor()` uniformly handles focus graying — unfocused windows fade
  automatically);
- `drawClose/drawMax`: drawn as macOS-style red/green round balls;
- `getClose/getMax`: moves the buttons from the right side to the left —
  **the hot zones move along**, and the server's hit testing takes effect
  automatically. This is the power of the geometry-query protocol;
- `drawFrame`: calls `markFrameRound()` to cut the four corners transparent
  with an arc mask, implementing rounded windows (the comments remind you
  to cut by the frame rect *without* the shadow band, or the bottom-right
  corner will cut into the shadow area);
- `drawShadow`: true projection geometry — "shadow = the window's outline
  translated by (shadow, shadow), minus the window itself", computing alpha
  per pixel with an integer SDF; the crescent-shaped shadow at rounded
  corners appears naturally, no manual splicing of straight strips and arcs.

By contrast, [OpenCDEWM](../../system/xwin/xwm/opencde/OpenCDEWM.cc) is
only 114 lines — CDE-style square 3D borders are fine with the default
geometry.

### Follow Along: Write a Minimal WM

1. Create the directory `system/xwin/xwm/mywm/`, copy ewokwm's
   [Makefile](../../system/xwin/xwm/ewokwm/Makefile), and change the target
   name to `xwm_my`;
2. Write `MyWM.h/cc`, overriding nothing at first:

```cpp
#include <x++/XWM.h>
class MyWM : public Ewok::XWM {
public:
    MyWM(void) {
        xwm.theme.frameBGColor = 0xff336699;  // just change a border color
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

3. Edit `machines/<your machine>/system/etc/x/xinit.rd`:

```sh
@export XTHEME=mywm
@/bin/ipcserv /sbin/x/xwm_my
```

4. `make && make sd && make run` — your first window manager is online.
   Afterwards, to change the title bar, override `drawTitle`; to move the
   buttons, override `getClose` — one spot at a time, with instantly
   visible results.

### Themes: Separating Data from Code

The same WM program can also have its parameters swapped via a **theme
file**, by convention at `/usr/x/themes/<theme name>/xwm/theme.json` (e.g.
[ewokwm's theme.json](../../system/xwin/usr/x/themes/ewokwm/xwm/theme.json)):

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

The loading path is worth noting: xwm's `loadTheme()` doesn't read the file
itself — it asks **xserverd** to read it (`X_DCNTL_LOAD_XWM_THEME`), and
the result is pushed back to xwm via `XWM_CNTL_SET_THEME`. This detour
exists because the server also needs the theme's `shadow`, `round`, and
`frame_alpha` during compositing (which pixels are translucent, how large a
shadow band the drag outline must shrink by) — **theme parameters must
agree on both sides, so the server acts as the single authoritative
source** ([xtheme.c](../../system/xwin/drivers/xserverd/xtheme.c) does the
parsing). Cursor glyphs are also loaded from the theme directory
`<theme>/xwm/cursors/*.json`.

## 20.7 Fault Tolerance: What If xwm Crashes

xwm is an ordinary process — it can crash, be killed, or restart for an
upgrade. The window system's response has three steps, each findable in the
code:

1. **Liveness checking**: every `ipc_call` to xwm is preceded by
   `check_xwm()` (§20.2); a mismatched uuid deregisters immediately;
2. **Degradation**: without an xwm, `draw_desktop` draws a built-in dot
   pattern as the base
   ([draw_init_desktop() in xrender.c](../../system/xwin/drivers/xserverd/xrender.c));
   when `GET_WIN_SPACE` fails, it degrades to `winr = wsr` (windows run
   naked without decorations, but **keep working** — see the comment in
   [xwin_cmd.c](../../system/xwin/drivers/xserverd/xwin_cmd.c): failing
   here is worse than degrading, because window creation won't be retried);
3. **Self-healing**: after a new xwm registers, `xwm_changed` is set, and
   the server's main loop runs
   [xwin_revalidate_geometry()](../../system/xwin/drivers/xserverd/xwin_cmd.c)
   at **every step**: a decorated window can never have `winr == wsr`, so
   this equality is ironclad proof that "the geometry was computed during
   xwm's downtime" — ask `GET_WIN_SPACE` again, rebuild frame_g, re-fetch
   the hot zones, and the window automatically dresses back up. Healthy
   windows exit after a few memcmps, so the scanning cost is negligible.

This "liveness check → degrade → self-heal" trio is consistent with how
vfsd treats driver processes in Ch. 10 — a standard template for all
inter-service dependencies in EwokOS, well worth copying into your own
designs.

## 20.8 Exercises

1. Read [draw_frame() in xwm.c](../../system/xwin/libs/x/src/xwm.c) and
   draw a truth table of "style bits × state → which parts are drawn"
   (style: NO_FRAME/NO_TITLE/NO_RESIZE; state: NORMAL/MAX/FULL_SCREEN);
2. Complete the minimal WM from §20.6, then override `drawClose` to draw
   the close button as an "×";
3. Move the coordinates of `getClose` and `getMax` to the right side of the
   title bar, and verify the click hot zones move along (think: how many
   IPCs happened in between, and at which step was the cache refreshed);
4. With the system running, `kill` the xwm process and observe windows
   becoming undecorated but still usable; then manually `ipcserv
   /sbin/x/xwm_ewok` and watch the windows "automatically dress back up";
5. Food for thought: why does `DRAW_FRAME` use `ipc_call_wait` (synchronous
   waiting) instead of fire-and-forget? Hint: think about what the
   compositor does with frame_g next.

## 20.9 Summary

- xwm is an independent userland process, separated from xserverd as
  **mechanism vs. policy**: the server manages the window tree,
  compositing, and input; xwm only "measures sizes" and "draws
  decorations";
- Taking office: `ipc_serv_run` registers an IPC service →
  `X_DCNTL_SET_XWM` volunteers → the server records pid+uuid and afterwards
  calls xwm **in reverse** via `ipc_call`;
- The protocol has only 7 commands: 3 geometry queries
  (WIN_SPACE/FRAME_AREAS/MIN_SIZE) + 3 draws (FRAME/DESKTOP/DRAG_FRAME) +
  1 theme push;
- Zero-copy drawing: three shared-memory canvases — desktop_g (fullscreen),
  frame_g (decorations), ws_g (content); IPC only carries shm_id and
  xinfo_t;
- The hot zones (title/buttons/resize corner) are computed once by xwm,
  cached by the server, and hit-tested locally; xwm never sees input
  events;
- The implementation has three layers: libx's C function table → the x++
  XWM base class's defaults → your subclass overriding on demand; theme
  parameters travel via JSON files, loaded and distributed uniformly by the
  server;
- The fault-tolerance trio: uuid liveness check → undecorated degradation →
  self-healing per frame using `winr == wsr` as the telltale sign.

With this, the graphics system's last puzzle piece is in place. Look back
at Ch. 13's layering diagram: from a piece of bare memory requested via
the mailbox, to the graph library's brushes, to xserverd's compositing and
dispatch, to xwm dressing every window — every layer is an ordinary
userland process plus IPC. That is the beauty of a microkernel.
