# G2D: Zero-Copy Canvas Sharing and Contiguous Physical Memory

This document describes how the EwokOS 2D graphics path (`g2dclient` ↔
`g2dd`) achieves **zero-copy** pixel transfer between applications and the
G2D device service, and how the kernel allocates and shares
**physically contiguous memory** for canvases that hardware engines need.

## 1. Architecture Overview

```
┌────────────────────────────┐          ┌────────────────────────────┐
│ client process             │          │ g2dd (device service)      │
│ (app / xwin / g2dtest)     │          │                            │
│                            │          │                            │
│  graph_t (shm-backed)      │          │  stateless: owns NO canvas │
│    ┌──────────────────┐    │  IPC     │                            │
│    │ pixel buffer     │    │ dev_cntl │  attach ─ operate ─ detach │
│    │ = shm segment    │◄───┼──────────┼──► shmat(same segment)     │
│    └────────┬─────────┘    │ only the │                            │
│             │              │ canvas   │  bsp_g2d_* (cpu or hw 2d)  │
│             ▼              │ id is    │                            │
│   shmat() mapping at       │ sent     │  operates IN PLACE on the  │
│   fixed SHM window VA      │          │  client's own pixels       │
└────────────────────────────┘          └────────────────────────────┘
          both processes map the SAME physical pages, same virtual address
```

Key components:

| Component | Path | Role |
|---|---|---|
| `g2dclient` | `system/gui/libs/g2dclient/` | client library: allocates shm canvases, sends requests to `/dev/g2d` |
| `g2dd` | `system/gui/drivers/g2dd/` (generic), per-machine variants under `machines/*/system/drivers/g2dd/` | device service: attaches to client canvases and executes ops |
| `graph` | `system/gui/libs/graph/` (`graph.c`, `graph_g2d.c`) | graphics library; `graph_new_shm()` creates zero-copy capable canvases |
| kernel shm | `kernel/kernel/src/mm/shm.c` | keyed shared memory, `IPC_CONTIG` contiguous slab backing |
| kernel dma | `kernel/kernel/src/mm/dma.c`, `svc.c` | `sys_dma` contiguous pool, `dma_alloc` / `SYS_MEM_MAP` |

The design is **stateless on the driver side**: `g2dd` keeps no internal
canvas. Every request struct (`g2d_fill_req_t`, `g2d_blit_req_t`,
`g2d_rotate_req_t`, `g2d_scale_to_req_t`) carries its canvases inside a
`g2d_canvas_t`, identified either by a **keyed shm segment id** or by a
**DMA buffer address**. The driver attaches, operates in place, and
detaches. See `system/gui/libs/g2dclient/include/g2dclient/g2dclient.h`.

```c
typedef struct {
    int32_t shm_id;     /* shm canvas id (> 0), ignored when dma != 0 */
    uint8_t dma;        /* 0: shm canvas, !=0: dma canvas (addr)      */
    uint8_t contig;     /* backing memory physically contiguous        */
    uint8_t reserved[2];
    uint32_t size;      /* segment size, must be >= w*h*4             */
    uint32_t w, h;
    ewokos_addr_t addr; /* dma buffer address when dma != 0           */
} g2d_canvas_t;
```

## 2. How Zero-Copy Works

### 2.1 The pixel buffer IS the shared segment

In the classic client/server graphics model the client renders into a local
buffer and the server copies pixels out of it (or vice versa). EwokOS
skips the copy entirely: the client allocates its pixel buffer **inside a
System V style shm segment**, and the device process maps **the same
physical pages**. Data never moves — only the *identifier* of the buffer
travels in the IPC request.

Client side (`system/gui/libs/graph/src/graph.c`, `graph_new_shm_row()`):

```c
/* the graph_t header is heap-allocated; only the pixels go into shm */
size = w * h * sizeof(uint32_t);
shm_id = shmget(key, size, 0666 | IPC_CREAT | IPC_EXCL | IPC_CONTIG);
pixels = (uint32_t*)shmat(shm_id, 0, 0);   /* cpu can draw directly  */
graph_init(ret, pixels, w, h);
ret->shm_id = shm_id;                       /* kept for g2d requests  */
```

`graph_new_shm(w, h)` prefers an `IPC_CONTIG` segment (hardware path) and
falls back to a scattered-page segment when the contiguous slab is
exhausted. The returned `graph_t` is fully usable by ordinary CPU code as
well — the pointer returned by `shmat()` is plain readable/writable memory.

### 2.2 One fixed virtual address for every attacher

EwokOS maps every shm segment at a **kernel-chosen, fixed virtual address**
(the shm window starting at `SHM_BASE`, see `kernel/kernel/include/mm/mmu.h`).
`shmat()` ignores the caller-supplied address hint and returns that fixed
address (`system/basic/libc/libewoksys/src/sys/shm/shmat.c` →
`SYS_PROC_SHM_MAP` → `shm_proc_map()` in `kernel/kernel/src/mm/shm.c`).

Because the kernel maps the segment's physical pages into each attaching
process at the *same* virtual address, a pointer into the canvas is valid
in every process that attached it. No address translation or fixup is
needed when the request crosses the process boundary.

### 2.3 The driver operates in place

`g2dd` (`system/gui/drivers/g2dd/g2dd.c`) resolves a request canvas in
`g2d_attach()`:

```c
if(canvas->shm_id <= 0)
    return -1;
if(canvas->size < canvas->w * canvas->h * sizeof(uint32_t))
    return -1;                    /* never write past segment end */
p = shmat(canvas->shm_id, 0, 0);  /* maps the SAME physical pages  */
at->buffer = (uint32_t*)p;
```

Then it calls the machine backend (`bsp_g2d_fill`, `bsp_g2d_blt`,
`bsp_g2d_blt_alpha`, `bsp_g2d_rotate`, `bsp_g2d_scale_to`), which may use
scalar/NEON CPU code or real 2D hardware, and finally `shmdt()`s the
canvas. The result pixels are already visible to the client — there was no
copy in either direction.

The zero-copy dispatch layer (`system/gui/libs/graph/src/graph_g2d.c`)
only accepts shm-backed graphs (`g->shm_id > 0`): a canvas created with
plain `graph_new()` cannot be sent to the device, and there is
intentionally no CPU fallback in the `*_g2d` functions.

### 2.4 Request transport

Only small fixed-size request structs travel over IPC
(`g2d_send_struct()` → `dev_cntl_by_pid()` to the `/dev/g2d` vdevice
process). Payload pixels never enter the IPC message:

```
client: graph_new_shm() ── draw with CPU if wanted ──
        g2d_canvas(shm_id, size, w, h, contig)
        g2d_blit_shm(&req)  ── dev_cntl(pid, G2D_DEV_CNTL_BLIT, &req) ──►
g2dd:   shmat(req.dst.shm_id) / shmat(req.src.shm_id)
        bsp_g2d_blt(...)                       ← writes dst in place
        shmdt(src); shmdt(dst)
client: reads the result directly from its own canvas pointer
```

### 2.5 Lifetime management

- Segments are created with mode `0666` because the driver is an
  unrelated process (keyed, not family-only `IPC_PRIVATE`).
- Each allocation uses a **fresh key**
  (`0x47324430 | pid<<16 | seq`, retried up to 16 times on `IPC_EXCL`
  collisions): without `IPC_EXCL`, `shmget()` would return an existing
  segment **without resizing** it, and the 16-bit sequence space wraps in
  long-lived processes.
- The kernel tracks attach count (`refs`). Each `shmdt()` decrements it;
  when both client and driver have detached (`refs <= 0`), the segment is
  freed (`free_item()` in `kernel/kernel/src/mm/shm.c`) — contiguous
  segments return their physical run to the slab pool.
- `graph_dup()` preserves backing type: duplicating an shm-backed graph
  allocates a new shm canvas so the copy stays g2d-capable.
- **Beware use-after-detach**: after `shmdt()`/`graph_free()` the pointer
  is dangling; reading it causes a data abort. Never `graph_dup()` a
  canvas you have already detached.

## 3. Physically Contiguous Memory

Hardware 2D engines (and DMA in general) often need buffers whose physical
pages are contiguous. Ordinary shm segments are backed by pages allocated
one at a time from the page allocator and are **not** guaranteed to be
physically contiguous. EwokOS solves this with two mechanisms.

### 3.1 `IPC_CONTIG`: reserved contiguous slab for shm segments

`IPC_CONTIG` (`0x01000000`, defined in
`system/basic/libc/libewoksys/include/sys/ipc.h`) is an EwokOS-specific
`shmget()` flag: *back the new segment with physically contiguous memory*.

**Boot-time reservation.** Contiguity can only be guaranteed if the memory
is reserved before the page allocator fragments physical memory, so the
kernel carves a slab at boot, right after the `sys_dma` window
(`kernel/kernel/src/hw_info.c`):

```c
_sys_info.shm_contig.phy_base = _sys_info.allocable_phy_mem_base;
_sys_info.shm_contig.size     = get_shm_contig_size();  /* default 4MB */
_sys_info.allocable_phy_mem_base += _sys_info.shm_contig.size;
```

The slab size is configurable in `/etc/kernel/kernel.conf`:

```
dma_size        = 134217728   # sys_dma pool (bytes); 0 = default
shm_contig_size = 16777216    # contiguous shm slab (bytes)
```

(`kernel/kernel/src/kernel_config.c` re-carves both windows from the
configuration.) The kernel direct-maps the slab (`P2V`) at startup so it
can zero it (`kernel/kernel/src/kernel.c`).

**Sub-allocation.** `kernel/kernel/src/mm/shm.c` manages the slab with a
first-fit table (`_shm_pool`, up to 64 entries) supporting split on
allocation and merge of physically adjacent free runs on release.

**Allocation path.** `shmget(key, size, 0666 | IPC_CREAT | IPC_EXCL | IPC_CONTIG)`:

1. `shm_pool_alloc(pages)` grabs a contiguous physical run from the slab.
   If the slab is unconfigured (`size == 0`) or exhausted, the call
   **fails strictly** — it never silently falls back to scattered pages,
   because hardware would then receive a non-contiguous buffer.
2. `shm_map_pages_contig()` maps the run into the kernel shm window
   (`AP_RW_D`, `PTE_ATTR_NOCACHE`) and zeroes it through the direct map.
3. The segment record stores `contig = 1` and `phy_base`. Free window
   blocks keep their backing type and are never reused across types.
4. When the client later calls `shmat()`, `shm_proc_map()` resolves the
   segment's window pages and maps the same physical run into the process
   (`PTE_ATTR_WRBACK`). Since the physical run is contiguous, the user
   buffer is contiguous too, and its physical base is the slab offset
   recorded in the segment.

### 3.2 The `sys_dma` pool (driver DMA buffers)

Drivers that need device-visible buffers use the `sys_dma` window instead:

- The kernel reserves a contiguous DMA pool at boot
  (`_sys_info.sys_dma.phy_base` / `.size`) and maps it identity-mapped
  plus at a private virtual window `DMA_V_BASE`.
- `dma_alloc(block, size)` (`SYS_DMA_ALLOC`, root only) sub-allocates from
  the pool and maps the buffer into the **calling process only**, as
  non-cacheable. `dma_phy_addr()` translates the returned vaddr to the
  physical address for programming hardware.
- Because the buffer mapping is private to the allocator, another process
  (e.g. `g2dd` attaching a client's DMA canvas) maps the same physical
  range with `SYS_MEM_MAP`. The kernel permits `SYS_MEM_MAP` only for
  ranges inside the `sys_dma` pool (or MMIO/carveouts, see
  `sys_mem_map()` in `kernel/kernel/src/svc.c`), with the same
  non-cacheable attribute to avoid cache aliasing.
- `g2dd` performs exactly this in `g2d_dma_map()`: a canvas with
  `dma != 0` carries the allocator's vaddr, and the driver translates it
  through `sys_get_sys_info()` and `SYS_MEM_MAP` before operating on it.
  Identity addresses inside the physical window are already visible to
  every process and need no mapping.

### 3.3 Telling the backend the buffer is contiguous

The `contig` flag travels inside `g2d_canvas_t`. The machine backend
(`bsp_g2d_*`) receives it for every source and destination buffer and can
choose the hardware path (which dereferences the buffer as one contiguous
physical range) or stay on the CPU path:

```c
bsp_g2d_blt(src_buf, src_contig, src_w, src_h, sx, sy, sw, sh,
            dst_buf, dst_contig, dst_w, dst_h, dx, dy, dw, dh);
```

Clients set it from `graph_t.shm_contig` (set at `graph_new_shm_row()`
time), so the information flows end to end:
`shmget(IPC_CONTIG)` → `graph_t.shm_contig` → `g2d_canvas_t.contig` →
`bsp_g2d_*(..., contig, ...)`.

## 4. Using It

### 4.1 Recommended: the `graph` API

```c
#include <graph/graph.h>
#include <graph/graph_g2d.h>

graph_t* canvas = graph_new_shm(800, 480);   /* zero-copy capable   */
/* ... ordinary CPU drawing works on canvas->buffer ... */

graph_fill_g2d(canvas, 0, 0, 800, 480, 0xffff0000);  /* offload ops */
graph_blt_g2d(dst, src, &src_rect, &dst_rect);

graph_free(canvas);  /* shmdt + segment released when refs reach 0 */
```

### 4.2 Raw `g2dclient` API

```c
#include <g2dclient/g2dclient.h>

int shm_id; uint32_t* pixels;
g2d_shm_alloc(480 * 272 * 4, &shm_id, &pixels);      /* keyed 0666 segment */

g2d_canvas_t dst = g2d_canvas(shm_id, 480*272*4, 480, 272, /*contig*/0);
g2d_fill_req_t req;
g2d_fill_req_init(&req, dst, g2d_rect(0, 0, 480, 272), 0xff223344);
g2d_fill_rect(&req);          /* dev_cntl to /dev/g2d, synchronous */

g2d_shm_free(pixels);         /* shmdt */
```

For DMA-backed canvases build the canvas with
`g2d_canvas_dma(addr, size, w, h)` where `addr` is the value returned by
`dma_alloc()`.

### 4.3 Configuration checklist

- `/etc/kernel/kernel.conf`: set `shm_contig_size` large enough for the
  biggest contiguous canvas (defaults to 4MB when unset). Contiguous
  `shmget()` **fails** when the slab is unconfigured or full.
- Ensure `g2dd` is running and registered as `/dev/g2d`
  (`has_g2d()` resolves the vdevice process id).
- Build order: `g2dclient` before `graph`; GUI apps link with
  `-lg2dclient` via the shared `EWOK_LIB_GRAPH` variable.

## 5. Design Notes and Gotchas

- **Validity convention**: `shm_id > 0` is the canonical validity check
  across the whole stack (graph, g2dclient, g2dd).
- **No CPU fallback** in the `graph_*_g2d` dispatchers: if the device
  path fails, the operation fails; callers that need fallback select
  between `*_cpu` and `*_g2d` themselves.
- **Size validation**: `g2d_attach()` rejects segments smaller than
  `w*h*4` so the driver can never write past the mapping.
- **No-resize semantics**: `shmget()` on an existing key returns the old
  segment without resizing — always allocate with a fresh key +
  `IPC_EXCL` (the library helpers already do this).
- **Cache attributes**: kernel window and DMA mappings are non-cacheable;
  process shm mappings are write-back. Since all userspace accesses go
  through a single shared mapping per process (same VA, same physical
  pages), CPU producer/consumer coherency is inherent to the page
  sharing. When mixing CPU writes with DMA hardware on the same buffer,
  prefer `IPC_CONTIG`/DMA paths defined by the BSP and follow the
  backend's coherency requirements.
- **Miyoo exception**: `machines/miyoo/system/drivers/g2dd/` implements a
  separate, hardware-specific protocol (SigmaStar GE / `mi_gfx` with
  `MI_SYS_MMA` allocation) and does not follow the stateless shm canvas
  model described here.
