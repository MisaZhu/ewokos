# 22 The xserverd Compositor: Window Rendering Flow and Dirty Regions

> Language: **English** | [中文](22-xserverd-render.zh.md)
>
> Goal of this chapter: take one frame of the xserverd compositor apart end to
> end — how a client hands its finished pixels to the server, how the server
> decides which windows need repainting, how a window's content and decorations
> land on the screen buffer, and how only the changed rectangles get pushed to
> the display driver. When you are done you will be able to follow nearly every
> comment in `xwin.c` / `xrepaint.c` / `xrender.c` under
> [xserverd/](../../system/xwin/drivers/xserverd/).
> Read [Chapter 13 Graphics](13-graphics.md) and
> [Chapter 20 The xwm Window Manager](20-xwm.md) first.

## 22.1 The Big Picture: How One Frame Reaches the Screen

Chapter 20 was about *what a window looks like* (xwm's policy); this chapter is
about *how the pixels get moved* (xserverd's mechanism). Here is the whole
pipeline at a glance:

```
 client process (libx)          xserverd main loop (loop_step)                fbdisplayd
┌────────────────┐   shared mem  ┌──────────────────────────────────┐          ┌──────────┐
│ on_repaint()   │─paint ws_g──►│ ① x_poll_updates  accept frames   │          │          │
│ xwin_repaint() │ set handshake │ ② x_repaint       composite/disp  │──flush──►│ push to  │
│ (block/async)  │◄─wake/release│    ├ draw_desktop  (xwm paints bg)│ dirty    │ panel    │
└────────────────┘              │    ├ draw_win ×N   (bottom→top)   │ rects    │ (DMA)    │
                                │    │   └ DRAW_FRAME (xwm decorates)│ only     └──────────┘
                                │    └ cursor / drag outline overlay│
                                │ ③ display_flush   (blocking, off-lock)
                                └──────────────────────────────────┘
```

Three design decisions run through the whole chapter:

1. **The compositor reads the buffer the client published, directly.** The
   server keeps no private per-window snapshot any more; the client's finished
   `ws_g` *is* the composite source. One full-window pixel move less per frame;
2. **Compositing writes straight into the scan-out DMA buffer.** `display->g`
   is the very memory fbdisplayd handed out (`display_fetch_graph`). After
   compositing, the server only tells the driver *which rectangles changed*
   and the driver pushes those to the panel;
3. **Two levels of dirtiness.** `display->dirty` means a *full rebuild*
   (desktop + every window, bottom to top); `display->need_repaint` means an
   *incremental repaint* (only windows with `win->dirty`). The vast majority of
   frames take the incremental path; a rebuild happens only on z-order,
   geometry or translucent-window changes.

The module split, matching the header comment of
[xserverd.c](../../system/xwin/drivers/xserverd/xserverd.c):

| File | Role | Functions covered here |
|---|---|---|
| [xserver_dev.c](../../system/xwin/drivers/xserverd/xserver_dev.c) | vdevice callbacks, main-loop pacing | `xserver_step` |
| [xwin.c](../../system/xwin/drivers/xserverd/xwin.c) | window list, frame-accept handshake, dirty propagation | `x_poll_updates` `x_accept_update` `x_update_commit` `mark_dirty` `covered_by_opaque_win` |
| [xrepaint.c](../../system/xwin/drivers/xserverd/xrepaint.c) | per-display repaint pipeline, dirty rect collection, cursor | `x_repaint` `x_repaint_add_dirty` `pack_dirty_rects` `x_cursor_redraw_now` |
| [xrender.c](../../system/xwin/drivers/xserverd/xrender.c) | the actual blits of desktop / window / decorations | `draw_win` `prepare_win_content` `blit_win_part` `refresh_shadows_above` |
| [xserver.h](../../system/xwin/drivers/xserverd/xserver.h) | data structures, cross-module inline predicates | `win_comp_src` `frame_cuts_ws` `win_bg_effect_active` |

## 22.2 Canvases and State: Know Your Buffers First

### The four canvases of a window

[xwin_t](../../system/xwin/drivers/xserverd/xserver.h) carries four
`graph_t*`, all allocated by the server with `graph_new_shm` and published to
the client and xwm through the shm ids in
[xinfo_t](../../system/xwin/libs/x/include/x/xcntl.h):

```
winr (outer window rect)
┌──────────────────────────────────┐
│ frame_g: decoration ring          │  size = winr, xwm draws on it
│ ┌──────────────────────────────┐ │
│ │ ws_g : client render target  │ │  size = wsr, the client's on_repaint paints here
│ │ ws_g2: fps_async handoff     │ │  only with fps_async=1, client copies ws_g into it
│ └──────────────────────────────┘ │
│ backdrop: clean snapshot of the   │  only with theme frame_blur, given to xwm for
│           scene below the window  │  the frosted-glass frame
└──────────────────────────────────┘
```

- **ws_g / ws_g2**: which one the compositor reads is decided by
  [win_comp_src()](../../system/xwin/drivers/xserverd/xserver.h) —
  `ws_g2` when `fps_async && front_index == 1`, `ws_g` otherwise;
- **frame_g** holds **only the decoration ring**. The workspace area inside it
  is normally empty: at composite time the workspace pixels are taken straight
  from the client buffer. Only two theme effects (the unfocused background
  effect and a translucent rounded frame) copy the workspace into `frame_g`
  first so xwm can blend over it (§22.6);
- **backdrop**: a frosted frame has to blur "what is below the window", but at
  the window's position the screen buffer already holds the window itself. So
  the server grabs a clean snapshot whenever something below repaints
  (`capture_backdrop`).

### Per-window flags

| Field | Meaning | Set by | Cleared by |
|---|---|---|---|
| `ready` | the client has published at least one frame, may be composited | `x_accept_update` | geometry rebuild |
| `dirty` | content changed, repaint this frame | `x_accept_update`, `mark_dirty_confirm`, full rebuild | end of `draw_win` |
| `frame_dirty` | decoration ring must be redrawn (ask xwm) | focus change, geometry rebuild, `win_mark_frame_dirty` | end of `draw_win` |
| `dirty_mark` | temporary mark during dirty propagation | `mark_dirty` | `mark_dirty_confirm` |
| `shadow_valid` + `shadow_rect` | the window's translucent parts (shadow bands / corners) already sit blended on the display **at this placement** | `draw_win` after blending | z-order change, focus change, full rebuild, fully covered |
| `not_ready_ms` `paint_ms` `accept_ms` `repaint_req_ms` | timeout timers (§22.10) | — | — |

### Per-display state

[x_display_t](../../system/xwin/drivers/xserverd/xserver.h):

| Field | Meaning |
|---|---|
| `g` | the scan-out canvas (fbdisplayd's DMA buffer); every composite writes here |
| `dirty` | **full rebuild**: desktop + every window bottom to top |
| `need_repaint` | **incremental repaint**: run `x_repaint` this step |
| `desktop_rect` | the desktop area (whole screen) |
| `pending_flush` / `flush_inflight` | changes waiting to be pushed / push in progress (protects the single dirty-rect slot, §22.7) |
| `wait_ready` / `paint_wait` | frames spent waiting for windows to get ready / for painters to finish (bounded) |
| `cursor_task` | cursor moved but the fast path could not draw it; the frame path catches up |

Two entry points pin down the semantics of the two dirty levels
([xserverd.c](../../system/xwin/drivers/xserverd/xserverd.c)):

```c
void x_dirty(x_t* x, int32_t display_index);        // dirty = need_repaint = true   → full rebuild
void x_repaint_req(x_t* x, int32_t display_index);  // need_repaint = true            → incremental
```

Whenever you meet `x_dirty` in the code, ask "why does this need a full
rebuild?" — the comment at almost every call site answers it.

## 22.3 Publishing a Frame: the Shared-Memory UPDATE Handshake

Older xserverd versions used an `XWIN_CNTL_UPDATE` IPC to say "I'm done
painting", and the server then copied a snapshot of the workspace. That IPC is
gone: client and server now shake hands purely through a few `volatile` fields
in `xinfo_t` — no vdevice dispatch, no file-cache lookup, no pixel copy on the
hot path.

### Blocking mode (`fps_async = 0`, the default)

The skeleton of the client's [xwin_repaint()](../../system/xwin/libs/x/src/xwin.c):

```c
void xwin_repaint(xwin_t* xwin) {
    pthread_mutex_lock(&xwin->painting_lock);
    xwin->xinfo->painting = 1;              // ① "I am writing this buffer"
    xwin->on_repaint(xwin, &g);             // ② the app paints into ws_g (the composite source!)

    xwin->xinfo->update_pid = thread_get_id();   // ③ record the CURRENT THREAD pid (not getpid)
    xwin->xinfo->painting = 0;              // ④ pen down
    __sync_synchronize();                   //    release barrier: pixels visible before the flag
    xwin->xinfo->update_requested = 1;      // ⑤ publish
    while(xwin->xinfo->update_requested)
        proc_block_by(xwin->xinfo->win);    // ⑥ park here until the server composited and released

    xwin->xinfo->painting = 1;              // ⑦ re-claim right away (framebuffer-style apps paint between presents)
    pthread_mutex_unlock(&xwin->painting_lock);
}
```

The server side has two halves: **accept** at the start of every step,
**release** after compositing:

```c
// xwin.c: scan every window at the start of a step
void x_poll_updates(x_t* x) {
    for(each window win) {
        if(win->xinfo->update_requested) {
            __sync_synchronize();               // acquire barrier, pairs with the client's release
            if(visible && win_comp_src(win) != NULL)
                x_accept_update(x, win);        // no copy! just bookkeeping: ready=true, win_dirty()
            else
                x_update_commit(x, win);        // invisible: don't keep the client parked for nothing
        }
        ...timeouts, see §22.10...
    }
}

// xrepaint.c: right after compositing this window
draw_win(display->g, x, win, &win_dirty);
x_update_commit(x, win);                        // clear update_requested, proc_wakeup_by(update_pid)
```

Three details are worth pausing on:

- **Why no copy is needed**: between ⑤ and ⑥ the client sits in
  `proc_block_by`; it *physically cannot* write `ws_g` while the server reads
  it. Buffer ownership is handed over by protocol, not isolated by copying;
- **Why `update_pid` must be the thread pid**: `proc_wakeup_by` wakes one
  specific kernel task. `getpid()` returns the main thread; an app calling
  `xwin_repaint` from its own render thread (macemu's present thread, say)
  would wake the wrong context and hang forever;
- **Why the release comes after `draw_win` and not at accept time**: `draw_win`
  is the last reader of the buffer (it also hands it to xwm for the
  decorations). Releasing at accept would let the client start the next frame
  into the very buffer being composited.

A side effect is that **the client's frame rate is capped to the server's**: a
client painting faster than fps parks at ⑥ until the next step. That is exactly
the throttling we want — the server always runs at its own pace.

### Asynchronous double-buffered mode (`fps_async = 1`)

With `"fps_async": 1` in x.json the server allocates an extra `ws_g2` per
window. The client **always** renders into `ws_g` (framebuffer-style apps such
as the SDL2 backend cache the pixel pointer, so the render target cannot
alternate); the "flip" becomes an explicit copy
([xwin_flip_locked()](../../system/xwin/libs/x/src/xwin.c)):

```c
if(xwin->xinfo->update_requested != 0)   // server hasn't consumed the last frame → can't overwrite ws_g2
    return false;
painting = 1;  barrier;
graph_blt(ws_g → ws_g2);                 // copy into the handoff buffer
front_index = 1;                         // tell the server: read ws_g2
painting = 0;  barrier;
update_requested = 1;                    // publish, no blocking, return
```

The server side is unchanged: `win_comp_src` picks `ws_g2` because
`front_index == 1`, and `x_update_commit` clears the flag after the blit. If the
client finds the flag still set at its next present (server too slow) the frame
**must not be dropped** — an event-driven widget app has already cleared its
own dirty flags and will never paint that picture again. So libx records a
`present_pending`, and `xwin_retry_pending_presents()` in the event loop
republishes it once the server lets go.

The two modes compared:

| | blocking | fps_async |
|---|---|---|
| client fps | ≤ server fps | free |
| pixel moves per frame | 0 (composite reads ws_g) | 1 (ws_g → ws_g2) |
| tearing protection | client parked | ws_g2 written only while `update_requested==0` |
| memory | 1 workspace | 2 workspaces |

### The `painting` flag: how the compositor knows a buffer is readable

Since the compositor reads the client's buffer directly, it has to know whether
the client is writing it right now. The test in
[win_src_stable()](../../system/xwin/drivers/xserverd/xrepaint.c):

```c
if(update_requested || !painting)   // published (client parked) or not painting → stable
    return true;
// otherwise mid-frame: wait, but at most X_PAINT_TIMEOUT_MS (100ms)
```

Why the timeout: an app that goes idle after its last frame sits past ⑦ with
`painting` stuck at 1. Its buffer is actually complete; it must not stall a
full rebuild forever.

## 22.4 Dirty Propagation: One Window Changes, Who Else Gets Dirty

The last line of `x_accept_update` calls `win_dirty()`, and from there the
dirty mark propagates upward along the z-order. The window list
`win_head → win_tail` *is* the bottom-to-top z-order (`push_win` inserts
`SYSBOTTOM` at the head, `SYSTOP` at the tail, ordinary windows just below the
SYSTOP group).

The logic of [mark_dirty()](../../system/xwin/drivers/xserverd/xwin.c):

```
win becomes dirty
  │
  ├─ for every visible window top above win:
  │     r = top->winr ∩ check_r         check_r = theme has alpha frame or shadow ? win->winr : win->wsr
  │     if r non-empty → top->dirty_mark = true    (the part it covers must be repainted)
  │     if r == check_r (win fully hidden by top)
  │        && top opaque && top needs no desktop && top has no bg effect
  │        → break                                 (windows further up cannot see win)
  │
  ├─ mark_dirty_confirm: turn dirty_mark into dirty, escalating by window kind:
  │     need_repaint_desktop(v)  → x_dirty(display)   (translucent window / bg effect: full rebuild)
  │     frame_cuts_ws(v)         → v->frame_dirty     (translucent rounded frame: re-blend the ring)
  │
  └─ recurse on win->next
```

`win_dirty` then applies the same escalation to `win` itself and finally calls
`x_repaint_req(display)` — note this is only an **incremental** request by
default; a full rebuild is triggered solely by `need_repaint_desktop`.

The two predicates ([xwin.c](../../system/xwin/drivers/xserverd/xwin.c)):

```c
bool need_repaint_desktop(x, win) {          // whose repaint needs the desktop painted first?
    if(win->xinfo->alpha) return true;       // translucent content: must blend onto a clean background
    if(NO_FRAME) return false;
    if(win_bg_effect_active(x, win)) return true;  // unfocused bg effect: mixes the desktop into the window
    return false;                            // theme corners/shadow → NO! the compositor handles them alone
}

bool need_repaint_frame(x, win) {            // whose frame must be redrawn after a desktop repaint?
    if(NO_FRAME && !alpha) return false;
    if(win_bg_effect_active) return true;
    if(theme.frameAlpha && !win_edge_to_edge(win)) return true;  // translucent frame blends with what is below
    return false;
}
```

The comment on `need_repaint_desktop` records a real performance trap: it must
use `win_bg_effect_active()` (theme has the effect **and** the window is
unfocused **and** it lacks `NO_BG_EFFECT`), not the bare theme flag — most
system apps carry `NO_BG_EFFECT`, and testing the theme alone escalated every
unfocused one of them into a whole-display rebuild.

A three-window walk-through:

```
z-order (bottom→top):  A (opaque)   B (opaque, covers A's right half)   C (translucent alpha, covers B's top-left)

A publishes a frame:
  mark_dirty(A):  B ∩ A non-empty → B.dirty_mark; B does not fully cover A, continue; C ∩ A → C.dirty_mark
  confirm:        B.dirty; C.dirty, and C.alpha → need_repaint_desktop → x_dirty(display)  ← escalates to full rebuild
  result: full rebuild this frame: desktop → A → B → C

B publishes a frame (with no C):
  mark_dirty(B):  nothing above
  win_dirty:      B opaque, no effect → no escalation
  result: incremental: draw_win(B) only, then refresh_shadows_above repairs the shadows of windows above B
```

## 22.5 Compositing a Frame: `x_repaint` Step by Step

[x_repaint()](../../system/xwin/drivers/xserverd/xrepaint.c) runs once per
display per step. In code order, ten steps:

**① Gates**

```c
if(!display->need_repaint) return;            // nobody asked
if(display_busy(&display->display)) return;   // fbdisplayd still pushing the last frame: writing now tears, retry next step
if(!all_win_ready(x)) {                       // a visible window has not published its first frame
    if(display->wait_ready++ < X_WAIT_READY_MAX) return;   // wait at most 4 frames
}
if(display->dirty && some window !win_src_stable) {       // a rebuild cannot skip a mid-frame window (it would leave a hole)
    if(display->paint_wait++ < X_WAIT_READY_MAX) return;   // also at most 4 frames
}
display->need_repaint = false;
```

`display_busy` reads the `busy` byte in fbdisplayd's ctrl shared memory, set
while the driver pushes a frame. Since compositing writes the scan-out buffer
directly, this is the first tearing guard.

**② Hide the cursor**

The cursor is not a window but an overlay: `hide_cursor()` puts back the pixels
saved from under it (`cursor.saved`) and records the old rect
`cursor_old_rect`.

**③ Erase the drag outline**

If the previous step drew a drag outline (`drag_band_valid`), restore the saved
scene band `drag_band` and record it as dirty. During a full rebuild the restore
is skipped (the area is repainted anyway).

**④ Desktop**

```c
if(display->dirty) {
    if(!covered_by_opaque_win(x, NULL, display_index, &display->desktop_rect)) {  // skip if a fullscreen opaque window hides it
        if(draw_desktop(x, display_index) == 0)      // XWM_CNTL_DRAW_DESKTOP, xwm paints the wallpaper into display->g
            add dirty rect(desktop_rect)
        else
            desktop_retry = true;                    // xwm refused: nothing was painted, keep dirty and retry next frame
    }
}
```

The comment records a real bug: an earlier version filled black before
returning when the xwm call failed, and the black stayed on the panel forever
(because `display->dirty` is cleared at the end of the frame). The rule now:
**if nothing was painted, flush nothing.**

**⑤ The window loop (bottom to top)**

```c
for(win = win_head; win; win = win->next) {
    if(!win->ready || !visible || not on this display) continue;
    if(display->dirty) { win->dirty = true; win->shadow_valid = false; }   // rebuild: everyone repaints, shadows re-blend

    if(!win->dirty && !win->frame_dirty) continue;

    if(win != win_drag && covered_by_opaque_win(x, win, display_index, &winr)) {
        // fully hidden by an opaque window above: drawing it is pure waste
        win->dirty = win->frame_dirty = false;
        win->shadow_valid = false;
        x_update_commit(x, win);           // but hand the buffer back so the client is not parked for nothing
    }
    else if(!display->dirty && !win_src_stable(x, win)) {
        paint_retry = true;                // incremental path, client mid-frame: the screen still shows the last frame, take it later
    }
    else {
        draw_win(display->g, x, win, &win_dirty);          // §22.6
        if(!display->dirty)
            refresh_shadows_above(x, win, &win_dirty);     // repair translucent parts of windows above wiped by this repaint
        add dirty rect(win_dirty);
        x_update_commit(x, win);                           // last reader done: release the client
    }
}
```

**Occlusion culling** (`covered_by_opaque_win`) asks "is there one opaque
window above whose workspace fully contains this rect?". Edge-to-edge windows
(maximized/fullscreen) are tested with their `winr` — their title strip is
solid decoration drawn by xwm and covers just as well.

**⑥ Drag-outline overlay**

While a window is being moved or resized the window itself is not repainted;
only an outline is drawn:

```c
get_drag_frame_rect(x, &r);
inflate r by frameW pixels;              // xwm's graph_frame draws OUTSIDE the rect; without this the outline trails
graph_blt(display->g → drag_band);       // save the scene under the outline first
draw_drag_frame(x, display_index);       // XWM_CNTL_DRAW_DRAG_FRAME
add dirty rect(r);
```

One drag step costs two small band blits (erase old + save new), one xwm
outline call and a small flush — not a whole-display repaint.

**⑦⑧ Draw the cursor, record its rects**

`refresh_cursor()` saves the pixels under the new position into `cursor.saved`
and draws the cursor bitmap. Both the old and the new rect go into the dirty
list.

**⑨ Update the display state**

```c
display->dirty = desktop_retry;          // normally cleared; kept when xwm refused the desktop
if(paint_retry) display->need_repaint = true;   // a window still owes a frame
```

**⑩ Hand over the dirty rects**

```c
if(do_flush && dirty_num > 0) {
    pack_dirty_rects(dirty_rects, dirty_num, display_dirty, DISPLAY_DIRTY_MAX);  // §22.7
    display_set_dirty(&display->display, display_dirty, display_num);
    display->flush_inflight = true;
    display->pending_flush = true;       // the actual flush IPC runs outside the lock
}
```

## 22.6 `draw_win`: How One Window Lands on the Screen

[draw_win()](../../system/xwin/drivers/xserverd/xrender.c) is the star of
`xrender.c`: it blends one window's content + decorations onto `display->g`
according to the theme. Three parts.

### Part one: prepare the decoration ring, `prepare_win_content`

```c
win_mark_frame_dirty(x, win);        // bg effect follows content / translucent frame follows a desktop repaint
if(frosted && (rebuild || placement changed)) capture_backdrop(x, win, &winr);   // grab a clean backdrop

prepare_win_content:
    if(frame_dirty) clear_frame_ring(win);              // clear only the ring outside the workspace (alpha themes blend from 0)
    if((bg effect || frame_cuts_ws) && (dirty || frame_dirty))
        graph_blt(win_comp_src(win) → workspace slot of frame_g);   // only these two cases copy content into frame_g
    if(!frame_dirty) return;                            // decorations unchanged: reuse frame_g, xwm is not bothered
    if(NO_FRAME && no effect) return;  if(fullscreen && no effect) return;
    ipc_call_wait(xwm_pid, XWM_CNTL_DRAW_FRAME, {shm_id, w, h, xinfo, top?});   // Chapter 20
```

The key saving: **`frame_g` is persistent.** When content changes but the
decorations do not (the most common case — an app refreshing its own picture),
`frame_dirty == false` and xwm is never called.

### Part two: split-source blitting, `blit_win_part`

[blit_win_part()](../../system/xwin/drivers/xserverd/xrender.c) puts a rect
`d` (frame coordinates) onto the display, taking **the workspace part from the
client buffer and the ring from `frame_g`**:

```
     frame_g (only the ring holds pixels)   win_comp_src (client buffer)
   ┌────────────────────┐            ┌──────────────┐
   │ top band            │            │              │
   │ ┌────────────────┐ │            │  whole        │
   │ │  (empty)       │ │ composite► │  workspace    │
   │ └────────────────┘ │            └──────────────┘
   │ left / right bands  │
   │ bottom band         │
   └────────────────────┘
   d ∩ workspace ← from client buffer     d − workspace ← up to 4 bands from frame_g
```

Only while the background effect or `frame_cuts_ws` is active (the whole
picture already lives in `frame_g`) does it fall back to a single source.

### Part three: four paths by theme

The long if/else in the middle of `draw_win` is the hardest part of the file,
yet it answers a single question: **which pixels are translucent, and may they
be blended again?**

Translucent pixels (shadow bands, the anti-aliased arcs outside rounded
corners, translucent window content) are blended onto the background with
`graph_blt_alpha`. If the background did not change and you **blend them
again**, alpha stacks on alpha — shadows get darker, corners get muddier, until
the next full rebuild resets them (the user sees a "flicker"). So each window
remembers, via `shadow_valid + shadow_rect`, "my translucent parts are already
blended at this placement", and while the placement is unchanged it **refreshes
only opaque pixels**.

| Path | Condition | On every content update | On every "placement" (position / z-order / focus change) |
|---|---|---|---|
| (a) translucent window | `xinfo->alpha` | straight ring edges plain-copied; interior and corners copy **opaque pixels only** (`blit_win_area_opaque`) | whole picture `blt_alpha` once, record `shadow_valid` |
| (b) opaque + shadow | `theme.shadow > 0 && !frameAlpha` | `inner` (winr minus right/bottom shadow bands) plain `blit_win_part` | right and bottom bands `blt_alpha` once |
| (c) translucent rounded frame | `frame_cuts_ws` | four straight ring edges plain-copied, `mid` via `blit_win_part`; the four corner squares copy opaque pixels only | corners + shadow bands `blt_alpha` once |
| (d) everything else | — | whole `dmg` via plain `blit_win_part` | — |

Why path (c) copies "opaque pixels only" in the corners: the corner radius
`round` is often larger than the frame width `frameW`, so a corner square holds
both the translucent arc anti-aliasing **and real workspace content / title
bar**. The content must follow every update, the arc must not be re-blended —
testing each pixel for alpha == 0xff is the only way to get both.

Where `shadow_valid` is invalidated (i.e. what counts as a "placement"):

- `push_win` (z-order change), `try_focus/x_unfocus` (the frame recolours):
  `shadow_valid = false`;
- `x_repaint` full rebuild: every window's `shadow_valid = false`;
- fully covered by a window above: `shadow_valid = false` (the shadow there
  was overwritten);
- `memcmp(shadow_rect, winr)` differs: the window moved or resized.

### Wrap-up: repairing the windows above, `refresh_shadows_above`

After an incremental repaint of a lower window, the freshly repainted region
may lie under the shadow bands or corner arcs of windows above — those
translucent pixels were just wiped by new content.
[refresh_shadows_above()](../../system/xwin/drivers/xserverd/xrender.c) walks
up the list and, for each window above:

- first `capture_backdrop` (this region is exactly the clean backdrop the
  frosted frame needs right now);
- translucent window: re-`blt_alpha` the whole intersection with the repainted
  region;
- other windows: if `shadow_valid`, re-blend only **repainted region ∩ shadow
  bands / corner squares** — never touching what was not wiped (blending it
  again would stack alpha); if `!shadow_valid`, just set `dirty` and let the
  window repaint itself whole.

A full rebuild does not need this step: every window repaints bottom to top and
blends its own bands onto a fresh background.

## 22.7 Dirty Rectangles: From 16 to 4 to the Panel

### Collecting: `x_repaint_add_dirty`

`x_repaint` collects the screen rects it touched this frame in an array of at
most 16 ([x_repaint_add_dirty()](../../system/xwin/drivers/xserverd/xrepaint.c)):

1. clip to the screen;
2. fully contained in an existing entry → drop;
3. overlaps or touches an entry → union into it, then **cascade**: the grown
   rect may now overlap other entries, keep merging until stable. The comment
   records a bug where the index `i` got lost during a swap-with-last removal —
   a classic trap in this kind of merge loop;
4. list full → union into slot 0.

### Packing: `pack_dirty_rects`

fbdisplayd's control block has only `DISPLAY_DIRTY_MAX = 4` slots
([display.h](../../system/gui/libs/display/include/display/display.h)); more
than that and the whole frame has to be pushed. So
[pack_dirty_rects()](../../system/xwin/drivers/xserverd/xrepaint.c) merges
greedily: for each surplus rect, pick the slot whose union grows the least in
area.

### Pushing: the ctrl shared memory and `flush`

```c
typedef struct {            // display_ctrl_t, shared between fbdisplayd and xserverd
    uint8_t  busy;          // 1 while the driver pushes a frame (the x_repaint gate reads it)
    uint8_t  dirty_num;     // rects for this flush, 0 = whole frame
    grect_t  dirty[4];
} display_ctrl_t;
```

`display_set_dirty` writes the rects into ctrl, `display_flush` sends a
`vfs_flush` to the driver; on the driver side
[do_flush() in displayd.c](../../system/gui/libs/displayd/src/displayd.c)
takes the rects, zeroes `dirty_num`, sets `busy = 1`, pushes by rect when it
can (`flush_dirty`) or the whole frame otherwise, then `busy = 0`.

**The dirty list is a single slot** — that is the reason `flush_inflight`
exists. The cursor fast path of §22.8 writes the same slot; if the frame path
had written 4 rects and the driver had not read them yet, the cursor path could
overwrite them with 2 cursor rects, the driver would push only the cursor, and
the erased drag outline would never reach the panel (trails). So from
`display_set_dirty` until the flush lands, `flush_inflight = true` and the fast
path backs off when it sees it.

### Why the flush is outside the lock

Look at [xserver_step()](../../system/xwin/drivers/xserverd/xserver_dev.c):

```c
x_server_lock_enter();
    x_poll_updates(x);
    for each display: x_repaint(x, i);       // only sets pending_flush, sends no IPC
x_server_lock_leave();

for each display:
    if(pending_flush) {
        display_flush(&display->display, true);   // block until the driver finished pushing, off-lock
        flush_inflight = false; pending_flush = false;
    }
```

`display_flush(…, true)` waits for the driver to push the pixels to the panel
— several milliseconds on real hardware (raspix's continuously scanned
framebuffer, say). Waiting inside the lock would stall every IPC handler thread
(mouse input, window commands). Yet `waiting = true` is necessary: compositing
writes the scan-out buffer itself, and the next frame must not start
overwriting it while the driver is still reading.

## 22.8 Around the Frame Pace: the Cursor and Drag Fast Paths

The main loop runs at `fps` (default 30), so visual feedback for a mouse move
could wait up to 33ms. Two fast paths take the cheapest interactive feedback out
of the frame pace:

**Immediate cursor redraw**,
[x_cursor_redraw_now()](../../system/xwin/drivers/xserverd/xrepaint.c), called
directly from the **input IPC handler thread** (which holds the server lock):

```c
if(!active || flush_inflight || display_busy()) return false;   // a frame is being pushed: back off, the frame path draws it
hide_cursor(x);   refresh_cursor(x);                              // just two small blits
display_set_dirty(2 cursor rects);
display_flush(&display->display, false);                          // non-blocking flush (IPC_NON_RETURN)
```

It never blocks and never touches the buffer while a push is in flight. When it
backs off, `cursor_task` stays set and the next `x_repaint` draws the cursor.
The cursor thus tracks at **event rate** rather than frame rate.

**The drag-outline overlay** was covered in §22.5 ⑥; its companion is the
pacing change in the main loop:

```c
uint32_t quantum = 1000 / fps;                       // normally 33ms
if(x->current.win_drag != NULL && drag_state != 0)
    quantum = X_DRAG_STEP_MS;                        // 8ms (~125Hz) while dragging
```

The drag handler calls `x_repaint_req` rather than `x_dirty` — the window
itself is not repainted during a drag, only the outline moves. Only on release
does the app get `XEVT_WIN_MOVE/RESIZE`, change its geometry, the server
rebuilds `frame_g`, and one full rebuild follows.

## 22.9 Main Loop and Locking: Who Touches This State When

The whole of [xserver_step()](../../system/xwin/drivers/xserverd/xserver_dev.c):

```c
int xserver_step(vdevice_t* dev, void* p) {
    tik = now;
    x_server_lock_enter();                    // ipc_disable() instead when multi_task=0
        if(xwm_changed && check_xwm) x_dirty(-1);      // new xwm: full rebuild
        for each win: xwin_revalidate_geometry(x, win); // Chapter 20's winr==wsr self-healing
        check_wins(x);                                  // reap windows whose owner died
        x_poll_updates(x);                              // ① accept published frames
        for each active display: x_repaint(x, i);       // ② composite
    x_server_lock_leave();

    for each display with pending_flush:
        display_flush(&display->display, true);         // ③ push (blocking, off-lock)

    sleep until the quantum is used up                  // 1000/fps, or 8ms while dragging
}
```

`x_server_lock` is one coarse lock over the server state. In `IPC_MULTI_TASK`
mode (mentioned in Chapter 13) input events and window commands are dispatched
on concurrent kernel worker threads sharing `x_t`, the window list and the
event pools with the main loop, so every vdevice callback takes the lock first.
Outbound IPC to xwm / vfsd is allowed inside the lock (they never need this
lock back, so no cycle can form), but two slow things are deliberately kept
outside: the blocking flush and the pacing sleep — neither may stall input
delivery.

Between client and server there is **no lock** at all — only the volatile
flags in `xinfo_t`, memory barriers and the `proc_block_by/proc_wakeup_by`
handshake. That is what makes the cross-process zero-copy design possible, and
why every `__sync_synchronize()` carries such a long comment.

## 22.10 Timing Constants at a Glance

| Constant | Value | Where | Protects |
|---|---|---|---|
| `fps` | 30 (x.json) | `xserver_step` | main-loop pace |
| `X_DRAG_STEP_MS` | 8 | `xserver_dev.c` | pace while dragging |
| `X_WAIT_READY_MAX` | 4 frames | `x_repaint` | waiting for windows to get ready / painters to finish must be bounded |
| `X_NOT_READY_TIMEOUT_MS` | 500 | `xserver.h` | a stuck window may not hold the whole display hostage; also paces re-sending `XEVT_WIN_REPAINT` to it |
| `X_PAINT_TIMEOUT_MS` | 100 | `xrepaint.c` | stop waiting when `painting` is stuck (idle client) |
| `X_ACCEPT_TIMEOUT_MS` | 200 | `xwin.c` | an accepted frame that never got composited (display busy / window covered): hand the buffer back, drop the damage, ask the client to paint again (`x_accept_abandon`) |
| `X_REPAINT_DIRTY_MAX` | 16 | `xrepaint.c` | dirty rects collected per frame |
| `DISPLAY_DIRTY_MAX` | 4 | `display.h` | dirty rects handed to the driver |

The idea shared by all of them: **unrelated processes must never stall each
other.** A client that hangs, stops painting or gets covered shows stale
content in its own window at worst; it must never freeze the whole screen.

## 22.11 Exercises

1. Add a log line in `x_repaint` printing `dirty_num` and each rect, then try
   "drag a window", "type in a terminal", "switch focus" and see how many dirty
   rects each produces and whether a full rebuild (`display->dirty`) fires;
2. Flip `fps_async` in x.json from 0 to 1 and, with a high-frame-rate app
   (`xDemo` or nesemu), compare how server fps and app fps relate in the two
   modes;
3. Read `mark_dirty`, draw four partially overlapping windows and work out by
   hand which windows get `dirty` when the bottom one changes, and where the
   propagation `break`s;
4. Force `bands_ok` in `draw_win` path (b) to always be `false`, refresh a
   shadowed window repeatedly, and watch the shadow darken — that is alpha
   stacking;
5. Thought question: what goes wrong if `x_update_commit` is moved forward into
   `x_poll_updates`? Hint: what does the client do right after release, and
   which buffer is the compositor reading at that moment;
6. Thought question: why must `x_cursor_redraw_now` back off while
   `flush_inflight` instead of waiting a little and then drawing? Hint: which
   thread does it run on.

## 22.12 Summary

- The path of a frame: the client's `xwin_repaint` paints into the shared
  `ws_g` and sets the handshake flag → the server's `x_poll_updates` accepts
  (no copy) → `x_repaint` composites bottom to top into the scan-out buffer →
  `x_update_commit` after `draw_win` releases the client → an off-lock
  `display_flush` pushes only the dirty rects;
- Two dirty levels: `x_dirty` full rebuild (desktop + all windows),
  `x_repaint_req` incremental (dirty windows only); only translucent windows
  and the unfocused background effect escalate to a full rebuild;
- Damage propagates upward along the z-order (`mark_dirty`) and stops at a
  fully covering opaque window; fully covered windows are culled
  (`covered_by_opaque_win`);
- Four canvases per window: `ws_g` (client render target), `ws_g2` (fps_async
  handoff), `frame_g` (persistent decoration ring — xwm is not called while
  decorations are unchanged), `backdrop` (frosted-glass background);
- `blit_win_part` blits from two sources: workspace from the client buffer,
  ring from `frame_g`;
- Translucent pixels are blended once per "placement"
  (`shadow_valid/shadow_rect`), afterwards only opaque pixels are refreshed to
  avoid alpha stacking; after an incremental repaint below,
  `refresh_shadows_above` repairs the wiped translucent parts of windows above;
- Dirty rects: 16 → cascaded merge → greedy packing into 4 → ctrl shared
  memory → the driver pushes by rect; `flush_inflight` guards that single slot
  against the cursor fast path;
- The cursor fast path and the drag-outline overlay bypass the frame pace so
  interaction stays responsive;
- Every wait is bounded (`X_*_TIMEOUT_MS`); no single client can stall the
  display.

You can now connect Chapter 13's "graphics stack overview", Chapter 20's
"dressing the windows" and this chapter's "moving the pixels" into one
continuous line: an app draws a stroke, and every step of what happens on the
screen can be pointed to in the source.
