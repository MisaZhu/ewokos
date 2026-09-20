# 13 The Graphics System and the Window System

> Language: **English** | [中文](13-graphics.zh.md)
>
> Goal: understand how pixels appear on the screen; read the layering of
> EwokOS's graphics stack: the framebuffer driver → the graphics library
> (g2d) → the window system (xwin).
> Terminology (Framebuffer/Mailbox/graph_t/g2d/blt/ARGB/compositing…) is in
> the [glossary](99-glossary.md).

## 13.1 The Screen's Essence: a Piece of Shared Memory

To the CPU, the screen is a piece of memory storing pixel colors row by row
— the **framebuffer**. Write color values into this memory and the
corresponding spots on the screen change color. The starting point of
graphics is this plain.

On the Pi, this memory belongs to the GPU. If the CPU wants it, it must
negotiate with the GPU through the **Mailbox** — write a "request message"
into a hardware mailbox register, and after processing, the GPU writes a
"reply message" back into the mailbox:

```
CPU: "give me a 1024x768, 32-bit-color framebuffer"  → write the MAILBOX register
GPU: allocates VRAM, replies "base=0x3E9F0000, width 1024, height 768"
CPU: maps that physical address into its own memory (SYS_MEM_MAP) and starts writing pixels
```

The mailbox protocol is implemented in
[mailbox.c](../../machines/raspix/kernel/lib/bcm283x/src/mailbox.c).
Three functions are enough to understand:

```c
void bcm283x_mailbox_send(msg) {
    do { stat = *MAIL0_STATUS; } while(stat.full);  // wait until the mailbox isn't full
    *MAIL0_WRITE = *msg;                            // write the message into the "write" register
}
void bcm283x_mailbox_read(msg) {
    do { flush_dcache(); stat = *MAIL0_STATUS; } while(stat.empty); // wait until not empty
    *msg = *MAIL0_READ;                             // fetch from the "read" register
}
void bcm283x_mailbox_call(msg) {   // one complete "ask → answer"
    flush_dcache();
    bcm283x_mailbox_send(msg);
    bcm283x_mailbox_read(msg);
}
```

Three details worth learning:

- **`flush_dcache`**: the message buffer lives in the CPU cache and must be
  flushed back to memory before the GPU can read it — a required lesson
  whenever "the CPU shares memory with other hardware";
- **Polling the status bits**: the `empty`/`full` flags are the hardware
  saying "can I receive/send right now";
- **Channels**: the mailbox mixes many kinds of messages (framebuffer,
  clocks, power…); each message carries a channel number, and reads filter
  by channel.

This is yet another "consult the manual, write the registers" driver
example — but the protagonist has changed to the GPU.

## 13.2 The EwokOS Graphics Stack Panorama

```
┌──────────────────────────────────────────────────────────┐
│ apps / window programs     (xwm windows, terminal, games…) │
├──────────────────────────────────────────────────────────┤
│ xwin: xserverd + xlib   (windows, compositing, event       │
│                         dispatch, X-style)                 │
├──────────────────────────────────────────────────────────┤
│ the graph library + g2dclient  (points/lines/rects/        │
│                                 bitmaps/text)              │
├──────────────────────────────────────────────────────────┤
│ g2dd / displaymand / fbdisplayd / fontd / consoled         │
│                (display driver, 2D accel, fonts, console)  │
├──────────────────────────────────────────────────────────┤
│ the Framebuffer (GPU VRAM)   ← requested via mailbox       │
└──────────────────────────────────────────────────────────┘
```

All of it is userland processes + IPC, continuing the microkernel
philosophy. Layer by layer:

### ① The Display Driver: fbdisplayd

The Pi's display driver is at
[machines/raspix/system/drivers/fbdisplayd/](../../machines/raspix/system/drivers/fbdisplayd/).
It does two things: request the framebuffer via the mailbox; and turn
`/dev/disp0` into "the screen" — any process can ask over IPC to flush its
finished pixels to the screen. `displaymand` is the upper-level management
service (arbitrating multiple display sources, managing resolutions).

### ② The Graphics Library: graph + g2d

The drawing primitives (lines, filled rectangles, bitmap copies, rotation
and scaling) live in
[system/gui/libs/graph/](../../system/gui/libs/graph/). First look closely
at the core data structure, the "canvas"
([graph.h](../../system/gui/libs/graph/include/graph/graph.h)):

```c
typedef struct {
    uint32_t* buffer;   // the pixel buffer (32-bit ARGB per pixel)
    int32_t w, h;       // width and height
    grect_t clip;       // the clip region: anything drawn outside is cut automatically
    bool need_free;
    int32_t shm_id;     // ★if the canvas comes from shared memory, its id goes here
    bool shm_contig;    // ★whether that shared memory is physically contiguous
} graph_t;
```

A canvas created by `graph_new()` uses ordinary `malloc` (enough for CPU
software rendering); but **if you want the hardware 2D engine to
accelerate, you must use `graph_new_shm()`** — making the canvas's `buffer`
land on a piece of shared memory and recording the `shm_id`. Why? Read on.

g2d's design is the part of this chapter most worth learning — the
**zero-copy shared canvas**:

```
the app process                     g2dd (the 2D acceleration service)
┌────────────────────┐           ┌────────────────────┐
│ graph_t canvas      │  IPC only  │ keeps no canvas    │
│  pixel buffer = shm │ ◄─canvas──►│ attach → draw in   │
│   (an shm segment)  │  id passes │ place → detach     │
└────────────────────┘           └────────────────────┘
      both sides map the same physical memory (even at the same virtual address)
```

Look at the real code in
[graph_g2d.c](../../system/gui/libs/graph/src/graph_g2d.c) — it embodies
this rule precisely:

```c
int graph_g2d_avaliable(graph_t* g) {
    // only a canvas with "shared-memory backing + physical contiguity"
    // can be handed to the hardware for acceleration
    if(g->shm_id <= 0 || !g->shm_contig)
        return 0;
    ...
}
// when actually accelerating, convert the virtual address into a physical
// address for the hardware:
canvas.phy = shm_contig_phy_addr(g->shm_id, (ewokos_addr_t)g->buffer);
```

Stringing three key points together:

1. **Why shared memory**: the ordinary approach would have the app copy the
   canvas to the driver over IPC, then copy it back after drawing — several
   hundred KB copied twice per frame destroys performance. Shared memory
   lets both sides operate on the same physical pages directly; IPC only
   passes an `shm_id`;
2. **Why physical contiguity**: the hardware 2D engine (and DMA) doesn't
   understand page tables — it works directly with physical addresses.
   Ordinary `malloc` memory is physically scattered, so the kernel provides
   a contiguous-memory pool, and `shm_contig_phy_addr` (Ch. 05's `shm.c`)
   translates virtual addresses into physical ones;
3. **Hence two kinds of canvas**: the pure-CPU canvas (`graph_new`) is
   flexible; the hardware-accelerated canvas (`graph_new_shm` + contiguous)
   is fast. The graphics library picks the appropriate backend
   automatically.

The accelerated primitives include `graph_fill_g2d` (fill),
`graph_blt_g2d` (bitmap copy), `graph_blt_alpha_g2d` (copy with alpha),
`graph_rotate_to_g2d` (rotation), and more.

### ③ The Window System: xwin

`xserverd` ([system/xwin/](../../system/xwin/)) is an X-style window
server:

- It maintains the window tree and decides who draws on top
  (**compositing**);
- It receives input events (keyboard/mouse/touch) and dispatches them to
  the corresponding window;
- Window programs connect to it through `xlib`: create windows, draw,
  receive events.

Hidden here is a field battle of a Ch. 09 concept: **`xserverd` runs its
multi-threaded service with `IPC_MULTI_TASK`**
([xserverd.c](../../system/xwin/drivers/xserverd/xserverd.c)). The reason
is practical: a mouse produces dozens of "move" events per second; in
single-task mode, mouse events would queue behind slow operations like
"re-composite the whole screen", and the cursor would stutter. The thread
pool gives mouse events their own worker thread, handled instantly —
**this is the most typical application of Ch. 09's "thread pool"**.

The window manager (xwm) handles title bars, borders, and dragging, and
supports multiple themes (`ewokwm`, `mac1984`, `openlook`, etc., chosen by
the `XTHEME` environment variable). xwm itself is an independent userland
process; the IPC protocol between it and xserverd, the shared-memory
drawing mechanism, and "how to write your own window manager" are covered
in a dedicated chapter — [Ch. 20](20-xwm.md). xserverd's own side — how a
client hands a finished frame to the server, how the server repaints only the
windows that changed and pushes only the dirty rectangles to the display
driver — gets its own chapter too: [Ch. 22](22-xserverd-render.md).

### ④ The Splash Screen and the Console

- `splashd` + the `splash` command: the boot progress bar (recall the pile
  of `@/bin/splash -m "..." -p xx` lines in `init.rd`);
- `consoled`: the graphical console (a fullscreen terminal), configured in
  `/etc/console.json`.

## 13.3 Follow Along: Run a Graphics Program

Build the full system with graphics and run it:

```bash
> cd machines/raspix/system
> make          # the all target includes the x window system
> make sd
> make run      # QEMU will pop up a graphical window showing the Pi's "screen"
```

After the boot script finishes `xwin/init.rd`, you'll see the splash screen
→ the windowed desktop. The `projects/` directory also has ready-made
graphics applications for reference: Minesweeper (`mine`), Cards (`cards`),
Doom (`doom`), an NES emulator (`nesemu`)… Their Makefiles follow the same
routine as Ch. 12, just additionally linking the graphics library.

## 13.4 Exercises

1. Read [graph_g2d.h](../../system/gui/libs/graph/include/graph/graph_g2d.h)
   and list the primitives the graphics library provides (lines /
   rectangles / bitmaps / text…);
2. Referring to `projects/mine/`, write a minimal window program: create a
   window and fill the background color;
3. Food for thought: why must the "canvas" use shared memory instead of
   ordinary IPC? If the screen resolution is 1920x1080 with 32-bit color,
   how many bytes is one frame? At 60 frames per second? (Answer: ~8.3
   MB/frame; ordinary IPC copying twice per frame = 1 GB/second of
   pointless copying.)

## 13.5 Summary

- The screen = framebuffer memory; on the Pi it's requested from the GPU
  via the mailbox (the send/read/call trio, flush the cache first, filter
  by channel);
- The graphics stack is layered: display driver → the graph/g2d graphics
  library → the xwin window system;
- A `graph_t` canvas's `shm_id`/`shm_contig` decide whether it can take the
  hardware-acceleration path;
- g2d implements the zero-copy canvas with shared memory; the hardware
  engine requires physical contiguity (`shm_contig_phy_addr`);
- `xserverd` uses an `IPC_MULTI_TASK` thread pool to handle requests in
  parallel, keeping mouse latency low;
- Windows, themes, and event dispatch all live in userland — a crash won't
  take the system down.

To go deeper into the window system, read
[Ch. 20: the xwm window manager](20-xwm.md) first, then
[Ch. 22: the xserverd compositor](22-xserverd-render.md); next chapter: put
all of this onto a real SD card and light up your Pi.
