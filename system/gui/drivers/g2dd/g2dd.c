#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <sys/shm.h>
#include <ewoksys/proto.h>
#include <ewoksys/klog.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/sys.h>
#include <ewoksys/syscall.h>
#include <ewoksys/shm.h>
#include <sysinfo.h>
#include <bsp/bsp_g2d.h>
#include <g2d_arch.h>
#include <g2dclient/g2dclient.h>

/* stateless g2d service: the driver owns no canvas. every request
   carries its canvases (g2d_canvas_t) either as keyed shm segment ids
   the driver attaches to, or as dma addresses the driver mem-maps on
   attach. the driver operates in place and detaches shm canvases when
   done. */
#define G2DD_DEBUG 0


/* g2d request counter: each debug line reports how many requests were
   served in the ~1s window since the previous line at that call site
   (a per-second rate, not a running total); guarded by _g2d_task_lock */
static uint64_t _g2dd_req_cnt = 0;

/* g2dd runs multi-task (device_run with multi_task=true): dev_cntl
   handlers execute on concurrent ipc worker threads, so serialize the
   whole request with a plain mutex. the critical section covers the
   request counter, the rate-limited log state, the shm key PRNG and
   the bsp_g2d back end calls (simd/hardware paths may carry shared
   state). */
static pthread_mutex_t _g2d_task_lock = PTHREAD_MUTEX_INITIALIZER;

static inline void g2d_task_lock(void) {
	pthread_mutex_lock(&_g2d_task_lock);
}

static inline void g2d_task_unlock(void) {
	pthread_mutex_unlock(&_g2d_task_lock);
}

/* debug logs are rate-limited to one line per second per call site:
   g2dd services every frame, so per-request logging floods the
   console */
#if G2DD_DEBUG
#define G2DD_LOG(fmt, ...) do { \
	static uint64_t _last_ms = 0; \
	static uint64_t _last_cnt = 0; \
	uint64_t _now_ms = kernel_tic_ms(0); \
	if(_last_ms == 0 || _now_ms - _last_ms >= 1000) { \
		slog("[%llu/s] " fmt, (unsigned long long)(_g2dd_req_cnt - _last_cnt), ##__VA_ARGS__); \
		_last_ms = _now_ms; \
		_last_cnt = _g2dd_req_cnt; \
	} \
} while(0)
#else
#define G2DD_LOG(fmt, ...)
#endif

/* byte alignment the 1:1 blit width is split at: the aligned part goes
   to the back end, the sub-alignment tail stays on the cpu. any
   multiple of 4 bytes (a whole pixel group) works; */
#ifndef G2D_PITCH_ALIGN
#define G2D_PITCH_ALIGN 64
#endif
#if (G2D_PITCH_ALIGN % 4) != 0 || G2D_PITCH_ALIGN <= 0
#error G2D_PITCH_ALIGN must be a positive multiple of 4 (whole pixels)
#endif
#define G2D_PITCH_ALIGN_PX (G2D_PITCH_ALIGN / 4)

typedef struct {
	uint32_t* buffer;
	uint32_t width;
	uint32_t height;
	uint8_t dma; /* dma canvas: buffer is a dma address, no shmdt */
	uint8_t contig; /* backing memory physically contiguous */
	uint8_t cached; /* mapping is owned by the attach cache: detach only
	                   drops the cache reference, never shmdt()s */
	int32_t shm_id; /* segment a cached mapping is bound to */
	ewokos_addr_t phy; /* physical base of the buffer when contig */
} g2d_attached_t;

static int32_t g2d_norm_degree(int32_t degree) {
	return ((degree % 360) + 360) % 360;
}

/* map a dma canvas into this process. addr is the vaddr the allocator
   got from dma_alloc(): a slot in the sys_dma v window that the kernel
   only mapped into the allocator itself, so map the same vaddr here.
   identity dma addresses (inside the phy window) are already visible
   in every process and need no mapping */
static int32_t g2d_dma_map(ewokos_addr_t addr, uint32_t size, ewokos_addr_t* paddr) {
	sys_info_t sysinfo;

	sys_get_sys_info(&sysinfo);
	if(addr >= sysinfo.sys_dma.v_base &&
			(addr + size) <= (sysinfo.sys_dma.v_base + sysinfo.sys_dma.size)) {
		*paddr = addr - sysinfo.sys_dma.v_base + sysinfo.sys_dma.phy_base;
	}
	else if(addr >= sysinfo.sys_dma.phy_base &&
			(addr + size) <= (sysinfo.sys_dma.phy_base + sysinfo.sys_dma.size)) {
		*paddr = addr;
		return 0;
	}
	else
		return -1;

	if(syscall3(SYS_MEM_MAP, addr, *paddr, size) != addr)
		return -1;
	return 0;
}

/* attach cache: a compositor hands the very same handful of full-frame
   shm canvases to every single request (the scan-out buffer, each
   window's workspace snapshot, its frame graph), and shmat/shmdt of one
   is a per-page walk in the kernel - shm_proc_map() does a map_page()
   plus a flush_tlb_addr() for EVERY page and the unmap side does the
   same again. An 800x480 argb frame is 375 pages, so one blit attaching
   src and dst costs around 1500 page table updates and 1500 TLB
   invalidations to move 1.4MB: the mapping churn, not the copy, is what
   the engine spends its time on. Keeping the recent segments attached
   turns all of that into a table lookup.

   Two properties keep this correct:
   - a hit is revalidated with shmctl(IPC_SHM_IS_CONTIG), which fails
     once the segment is gone. Ids grow monotonically so a freed id is
     never handed out again, but shm_alloc() DOES reuse the shm window
     ADDRESS of a freed block for a new segment - a stale entry would
     then quietly serve a buffer belonging to somebody else.
   - entries are dropped again (idle TTL, plus LRU eviction when the
     table is full). graph_free() only shmdt()s and the kernel frees a
     segment once its last attach goes away, so holding a mapping
     forever would pin the segment and leak the physically contiguous
     slab the whole display stack allocates from. An entry still
     referenced by an in-flight request is never evicted.

   Everything runs under _g2d_task_lock (g2d_dev_cntl takes it before
   dispatching any handler), so the table needs no locking of its own. */
#define G2D_ATTACH_CACHE_MAX 32
#define G2D_ATTACH_CACHE_TTL_MS 250

typedef struct {
	int32_t shm_id;
	uint32_t* buffer;
	uint8_t contig;
	ewokos_addr_t phy;
	uint32_t bytes; /* segment size: the cache budgets what it pins */
	uint32_t refs; /* in-flight attaches; >0 pins the entry */
	uint8_t stale; /* segment died while pinned: drop it on the last release */
	uint64_t last_ms; /* kernel_tic_ms of the most recent attach */
	uint8_t used;
} g2d_attach_entry_t;

static g2d_attach_entry_t _g2d_attach_cache[G2D_ATTACH_CACHE_MAX];
static uint64_t _g2d_attach_sweep_ms = 0;

/* ON by default: a compositor hands the very same handful of full-frame
   canvases to every request, so nearly every attach is a table hit and
   the per-request page walk disappears. What keeps the pinning honest is
   g2d_cache_budget() below (a quarter of the contig slab), the idle TTL
   sweep and LRU eviction.

   `devcmd /dev/g2d cache 0` still drops back to the plain shmat/shmdt
   path without a rebuild - and drains the table when it does. That drain
   is not cosmetic: the kernel takes NO reference on a repeat shmat of an
   id this process already has mapped, so a bypass attach of a cached id
   is a no-op handing back the very address an entry owns, and its paired
   shmdt() would tear that mapping out of the page table while the entry
   still points at it. Bare `cache` reports the counters. */
static int32_t _g2d_cache_enabled = 1;
static uint64_t _g2d_cache_hits = 0;
static uint64_t _g2d_cache_misses = 0;
static uint64_t _g2d_cache_evicts = 0;
static uint64_t _g2d_cache_stale = 0;
static uint64_t _g2d_cache_bytes = 0;
/* TTL drops are accounted separately from evictions: an eviction means the
   table or its byte budget was full, a sweep drop means the entry simply
   went idle. Without this split a ~50% hit rate with `evict 0` cannot be
   told apart from a stream of never-seen-before segment ids. */
static uint64_t _g2d_cache_swept = 0;

/* the cache's shmat is a kernel reference, so every entry pins its
   segment (and its physically contiguous slab pages) even after the
   owner released it. an unbounded pin set starves fresh
   graph_new_shm(IPC_CONTIG) allocations: the fallback non-contig graphs
   are refused by g2d_check_graph and drop the compositor onto cpu
   copies over the non-cacheable shm window, which misses frame
   deadlines and leaves windows undrawn. cap the pinning at a quarter of
   the slab, resolved once from sysinfo. */
static uint64_t g2d_cache_budget(void) {
	static uint64_t budget = 0;
	sys_info_t si;

	if(budget == 0) {
		sys_get_sys_info(&si);
		budget = si.shm_contig.size / 4;
		if(budget == 0)
			budget = 4 * 1024 * 1024;
	}
	return budget;
}

/* tear an entry down: the mapping goes back to the kernel (which frees
   the segment once this last reference drops) and its bytes leave the
   pin budget */
static void g2d_cache_drop(g2d_attach_entry_t* e) {
	shmdt(e->buffer);
	_g2d_cache_bytes -= e->bytes;
	e->buffer = NULL;
	e->bytes = 0;
	e->stale = 0;
	e->used = 0;
}

static g2d_attach_entry_t* g2d_cache_find(int32_t shm_id) {
	for(uint32_t i = 0; i < G2D_ATTACH_CACHE_MAX; i++) {
		if(_g2d_attach_cache[i].used && _g2d_attach_cache[i].shm_id == shm_id)
			return &_g2d_attach_cache[i];
	}
	return NULL;
}

/* unpin the segments nobody has asked for within the TTL, so a canvas
   its owner already released stops blocking its own free. Throttled to
   one walk per TTL: on a 60fps compositor this runs a few times a
   second instead of once per canvas. */
static void g2d_cache_sweep(uint64_t now_ms) {
	if(_g2d_attach_sweep_ms != 0 &&
			(now_ms - _g2d_attach_sweep_ms) < G2D_ATTACH_CACHE_TTL_MS)
		return;
	_g2d_attach_sweep_ms = now_ms;

	for(uint32_t i = 0; i < G2D_ATTACH_CACHE_MAX; i++) {
		g2d_attach_entry_t* e = &_g2d_attach_cache[i];
		if(!e->used || e->refs > 0)
			continue;
		if((now_ms - e->last_ms) < G2D_ATTACH_CACHE_TTL_MS)
			continue;
		g2d_cache_drop(e);
		_g2d_cache_swept++;
	}
}

/* a free slot, else the least recently used unpinned one (detached to
   make room). NULL when every entry is in flight, which leaves the
   caller on the plain shmat/shmdt path - slow but always correct. */
static g2d_attach_entry_t* g2d_cache_alloc(void) {
	g2d_attach_entry_t* oldest = NULL;

	for(uint32_t i = 0; i < G2D_ATTACH_CACHE_MAX; i++) {
		g2d_attach_entry_t* e = &_g2d_attach_cache[i];
		if(!e->used)
			return e;
		if(e->refs > 0)
			continue;
		if(oldest == NULL || e->last_ms < oldest->last_ms)
			oldest = e;
	}
	if(oldest == NULL)
		return NULL;

	g2d_cache_drop(oldest);
	_g2d_cache_evicts++;
	return oldest;
}

static void g2d_cache_release(int32_t shm_id) {
	g2d_attach_entry_t* e = g2d_cache_find(shm_id);
	if(e == NULL || e->refs == 0)
		return;
	e->refs--;
	if(e->refs > 0 || !e->stale)
		return;
	/* last in-flight user of a segment that turned out to be gone: drop the
	   mapping right here, the sweep would keep skipping a pinned entry */
	g2d_cache_drop(e);
}

/* hand every unpinned mapping back at once. Called when the cache is
   switched off at runtime: the sweep only runs on the enabled path, so a
   table left standing would pin its segments forever and, as the comment
   on _g2d_cache_enabled spells out, the bypass path would then unmap
   buffers the entries are still pointing at. An entry with refs>0 belongs
   to the request in flight; g2d_cache_release drops it once that request
   is done with it. */
static void g2d_cache_drain(void) {
	for(uint32_t i = 0; i < G2D_ATTACH_CACHE_MAX; i++) {
		g2d_attach_entry_t* e = &_g2d_attach_cache[i];
		if(!e->used || e->refs > 0)
			continue;
		g2d_cache_drop(e);
	}
	_g2d_attach_sweep_ms = 0;
}

/* attach a request canvas; rejects undersized segments so the mapping
   can never be written past its end */
static int32_t g2d_attach(const g2d_canvas_t* canvas, g2d_attached_t* at) {
	void* p;

	if(canvas == NULL || at == NULL)
		return -1;
	if(canvas->w == 0 || canvas->h == 0)
		return -1;
	if(canvas->size < canvas->w * canvas->h * sizeof(uint32_t))
		return -1;

	/* at is a caller stack variable: every field detach looks at has to
	   be written here, cached/shm_id included */
	at->cached = 0;
	at->shm_id = -1;
	at->phy = 0;

	if(canvas->dma != 0) {
		/* dma canvas: addr is the dma buffer address in the allocator's
		   sys_dma v window; map it into this process before use */
		if(canvas->addr == 0)
			return -1;
		if(g2d_dma_map(canvas->addr, canvas->size, &at->phy) != 0)
			return -1;
		at->buffer = (uint32_t*)(uintptr_t)canvas->addr;
		at->width = canvas->w;
		at->height = canvas->h;
		at->dma = 1;
		at->contig = canvas->contig;
		return 0;
	}

	if(canvas->shm_id <= 0)
		return -1;

	uint64_t now_ms = kernel_tic_ms(0);

	if(_g2d_cache_enabled == 0) {
		/* bypass: plain attach, never published to the cache */
		p = shmat(canvas->shm_id, 0, 0);
		if(p == (void*)-1)
			return -1;
		at->buffer = (uint32_t*)p;
		at->width = canvas->w;
		at->height = canvas->h;
		at->dma = 0;
		at->contig = canvas->contig;
		at->phy = canvas->phy;
		if(at->contig != 0 && at->phy == 0)
			at->phy = shm_contig_phy_addr(canvas->shm_id, (ewokos_addr_t)p);
		return 0;
	}

	g2d_cache_sweep(now_ms);

	g2d_attach_entry_t* e = g2d_cache_find(canvas->shm_id);
	if(e != NULL) {
		/* the segment may have been freed and its window address handed
		   to a new one since this entry was filled: shmctl fails on a
		   gone id, and the contig backing is a property of the segment
		   so a mismatch means the entry describes something else */
		int32_t alive = shmctl(canvas->shm_id, IPC_SHM_IS_CONTIG, NULL);
		if(alive < 0 || (uint8_t)alive != e->contig) {
			_g2d_cache_stale++;
			if(e->refs > 0) {
				/* pinned by this very request (src and dst naming one
				   segment). Our own attach is a kernel reference, so the
				   segment cannot have been freed underneath us and the
				   mapping is good for the rest of this request. Falling
				   through to a bare shmat/shmdt is exactly what would
				   tear it down while the entry still points at it, so the
				   entry is reused and marked for the matching release to
				   drop. */
				e->stale = 1;
			}
			else {
				g2d_cache_drop(e);
				e = NULL;
			}
		}
	}

	if(e != NULL) {
		_g2d_cache_hits++;
		e->refs++;
		e->last_ms = now_ms;
		at->buffer = e->buffer;
		at->width = canvas->w;
		at->height = canvas->h;
		at->dma = 0;
		at->contig = e->contig;
		at->phy = e->phy;
		at->cached = 1;
		at->shm_id = canvas->shm_id;
		return 0;
	}

	p = shmat(canvas->shm_id, 0, 0);
	if(p == (void*)-1)
		return -1;
	_g2d_cache_misses++;
	at->buffer = (uint32_t*)p;
	at->width = canvas->w;
	at->height = canvas->h;
	at->dma = 0;
	at->contig = canvas->contig;
	/* contig shm canvases carry their physical base from the client
	   (the shm window is mapped at the same vaddr in every process,
	   so the client-side translation is valid here); resolve it here
	   when the client left it empty */
	at->phy = canvas->phy;
	if(at->contig != 0 && at->phy == 0)
		at->phy = shm_contig_phy_addr(canvas->shm_id, (ewokos_addr_t)p);

	/* make room inside the pin budget before adopting the mapping: shed
	   the least recently used unpinned entries first, and when nothing
	   is sheddable (or the segment alone exceeds the budget) this attach
	   simply stays uncached and detaches the old way */
	while(_g2d_cache_bytes + canvas->size > g2d_cache_budget()) {
		g2d_attach_entry_t* lru = NULL;
		for(uint32_t i = 0; i < G2D_ATTACH_CACHE_MAX; i++) {
			g2d_attach_entry_t* c = &_g2d_attach_cache[i];
			if(!c->used || c->refs > 0)
				continue;
			if(lru == NULL || c->last_ms < lru->last_ms)
				lru = c;
		}
		if(lru == NULL)
			break;
		g2d_cache_drop(lru);
		_g2d_cache_evicts++;
	}

	e = g2d_cache_alloc();
	if(e != NULL && _g2d_cache_bytes + canvas->size <= g2d_cache_budget()) {
		e->shm_id = canvas->shm_id;
		e->buffer = (uint32_t*)p;
		e->contig = canvas->contig;
		e->phy = at->phy;
		e->bytes = canvas->size;
		e->refs = 1;
		e->stale = 0;
		e->last_ms = now_ms;
		e->used = 1;
		_g2d_cache_bytes += canvas->size;
		at->cached = 1;
		at->shm_id = canvas->shm_id;
	}
	return 0;
}

static void g2d_detach(const g2d_attached_t* at) {
	if(at == NULL || at->buffer == NULL || at->dma != 0)
		return;
	if(at->cached != 0) {
		/* the cache owns the mapping and hands it to the next request;
		   tearing it down here is exactly the churn this avoids */
		g2d_cache_release(at->shm_id);
		return;
	}
	/* a temp surface from g2d_alloc_surface, or an attach the full cache
	   could not adopt: this process is the only one left holding it, so
	   the shmdt is what lets the kernel free the segment */
	shmdt(at->buffer);
}

/* window clipping at the driver boundary: every rect handed to bsp_g2d
   is computed exact and in-bounds here first, the back end does not
   guarantee coordinate validity (a hardware 2d engine may not clip at
   all). */

/* clip a rect to a w x h canvas; returns 0 when nothing is left */
static int32_t g2d_clip_rect(int32_t w, int32_t h,
		int32_t* x, int32_t* y, int32_t* rw, int32_t* rh) {
	if(*x < 0) { *rw += *x; *x = 0; }
	if(*y < 0) { *rh += *y; *y = 0; }
	if(*x + *rw > w) *rw = w - *x;
	if(*y + *rh > h) *rh = h - *y;
	return (*rw > 0 && *rh > 0);
}

/* clip the dst rect to the dst canvas, scaling the src rect with the
   same proportion so the src->dst mapping stays aligned; returns 0
   when nothing is left to draw. sw/sh/dw/dh must be > 0 on entry. */
static int32_t g2d_clip_dst(int32_t dst_w, int32_t dst_h,
		int32_t* sx, int32_t* sy, int32_t* sw, int32_t* sh,
		int32_t* dx, int32_t* dy, int32_t* dw, int32_t* dh) {
	if(*dx < 0) {
		int32_t cut = (int32_t)((int64_t)(-*dx) * *sw / *dw);
		*sx += cut; *sw -= cut;
		*dw += *dx; *dx = 0;
	}
	if(*dy < 0) {
		int32_t cut = (int32_t)((int64_t)(-*dy) * *sh / *dh);
		*sy += cut; *sh -= cut;
		*dh += *dy; *dy = 0;
	}
	if(*dx + *dw > dst_w) {
		int32_t over = *dx + *dw - dst_w;
		int32_t cut = (int32_t)((int64_t)over * *sw / *dw);
		*dw -= over; *sw -= cut;
	}
	if(*dy + *dh > dst_h) {
		int32_t over = *dy + *dh - dst_h;
		int32_t cut = (int32_t)((int64_t)over * *sh / *dh);
		*dh -= over; *sh -= cut;
	}
	return (*sw > 0 && *sh > 0 && *dw > 0 && *dh > 0);
}

/* alpha fill: thin wrapper over the arch back end's scalar/simd alpha
   fill (arch_g2d_fill_alpha); the caller hands in a rect already
   clipped to the canvas bounds, same blend math as the blit paths */
static int32_t g2d_fill_alpha(uint32_t* buf, int32_t bw, int32_t bh,
		int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	return bsp_g2d_fill_alpha(buf, bw, bh, x, y, w, h, color);
}

/* temp surfaces for the rotated path: backed by keyed shm segments the
   same way graph_new_shm() does it (keyed 0666, IPC_EXCL, a fresh key
   per attempt), so the backing is physically contiguous and the back
   end gets the resolved physical base for hardware 2d paths. the kernel
   frees the segment once every attached process has detached, so
   freeing is a plain g2d_detach() (shmdt). */

/* key generator for temp shm segments: g2dd runs at a fixed pid, so a
   pid-derived key space would repeat itself across restarts. seed a
   PRNG once from the wall clock plus address entropy and draw a fresh
   32-bit id per attempt; IPC_EXCL + retry still resolves any rare
   collision. */
static key_t g2d_tmp_shm_key(void) {
	static int32_t seeded = 0;

	if(seeded == 0) {
		seeded = 1;
		srand((unsigned int)(time(NULL) ^ (uintptr_t)&seeded));
	}
	return (key_t)(0x47324430u +
			((((uint32_t)rand() & 0xffffu) << 16) | ((uint32_t)rand() & 0xffffu)));
}

static int32_t g2d_alloc_surface(int32_t w, int32_t h, g2d_attached_t* surf) {
	uint32_t size;
	int32_t shm_id = -1;
	uint8_t contig = 0;
	void* p;

	if(surf == NULL || w <= 0 || h <= 0)
		return -1;
	memset(surf, 0, sizeof(*surf));

	size = (uint32_t)w * (uint32_t)h * sizeof(uint32_t);
	/* a fresh random key per attempt: IPC_EXCL fails on key collisions */
	for(int32_t i = 0; i < 4 && shm_id <= 0; i++) {
		shm_id = shmget(g2d_tmp_shm_key(), (int)size,
				0666 | IPC_CREAT | IPC_EXCL | IPC_CONTIG);
	}
	if(shm_id > 0) {
		contig = 1;
	}
	else {
		/* contig slab unconfigured or exhausted: fall back to a plain
		   segment; the back end then works on the virtual pointer */
		for(int32_t i = 0; i < 4 && shm_id <= 0; i++) {
			shm_id = shmget(g2d_tmp_shm_key(), (int)size,
					0666 | IPC_CREAT | IPC_EXCL);
		}
	}
	if(shm_id <= 0)
		return -1;

	p = shmat(shm_id, 0, 0);
	if(p == (void*)-1) {
		/* destroy the never-attached segment so it cannot leak */
		shmctl(shm_id, IPC_RMID, NULL);
		return -1;
	}

	surf->buffer = (uint32_t*)p;
	surf->width = (uint32_t)w;
	surf->height = (uint32_t)h;
	surf->dma = 0;
	surf->contig = contig;
	if(contig != 0)
		surf->phy = shm_contig_phy_addr(shm_id, (ewokos_addr_t)p);
	return 0;
}

/* cpu fallback for widths below G2D_PITCH_ALIGN: dispatches to the
   platform arch engine's 1:1 blit (arch_g2d_blt / arch_g2d_blt_alpha,
   shipped in libgraph.a on every target). the software engine works on
   the virtual pointers and ignores phy/contig (passed 0 here), and its
   NEON blocks + scalar/padded tail handle any width, same blend math as
   the simd paths. the back end clips the rect against both canvas
   bounds. */
static int32_t g2d_cpu_blt(uint32_t* dst_buf, int32_t dst_w, int32_t dst_h,
		int32_t dx, int32_t dy, int32_t w, int32_t h,
		const uint32_t* src_buf, int32_t src_w, int32_t src_h,
		int32_t sx, int32_t sy, uint8_t use_alpha, uint8_t alpha) {
	if(use_alpha != 0)
		return arch_g2d_blt_alpha((uint32_t*)src_buf, 0, 0, src_w, src_h, sx, sy, w, h,
				dst_buf, 0, 0, dst_w, dst_h, dx, dy, w, h, alpha);
	return arch_g2d_blt((uint32_t*)src_buf, 0, 0, src_w, src_h, sx, sy, w, h,
			dst_buf, 0, 0, dst_w, dst_h, dx, dy, w, h);
}

/* pitch alignment rule for 1:1 copies: the width is split into an
   aligned part (whole G2D_PITCH_ALIGN byte groups) handed to the back
   end and a sub-alignment tail done on the cpu; copies narrower than
   G2D_PITCH_ALIGN never reach the back end at all (aw == 0). */
static int32_t g2d_blt_split(const g2d_attached_t* dst,
		uint32_t* src_buf, ewokos_addr_t src_phy, uint8_t src_contig,
		int32_t src_w, int32_t src_h, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		int32_t dx, int32_t dy, uint8_t use_alpha, uint8_t alpha) {
	int32_t aw;
	int32_t ret = -1;

	if(use_alpha != 0 && alpha == 0)
		return -1;

	aw = sw - sw % G2D_PITCH_ALIGN_PX;
	if(aw > 0) {
		if(use_alpha != 0) {
			ret = bsp_g2d_blt_alpha(src_buf, src_phy, src_contig, src_w, src_h, sx, sy, aw, sh,
					dst->buffer, dst->phy, dst->contig, (int32_t)dst->width, (int32_t)dst->height,
					dx, dy, aw, sh, alpha);
		}
		else {
			ret = bsp_g2d_blt(src_buf, src_phy, src_contig, src_w, src_h, sx, sy, aw, sh,
					dst->buffer, dst->phy, dst->contig, (int32_t)dst->width, (int32_t)dst->height,
					dx, dy, aw, sh);
		}
		if(ret != 0)
			return ret;
	}
	if(aw < sw) {
		ret = g2d_cpu_blt(dst->buffer, (int32_t)dst->width, (int32_t)dst->height,
				dx + aw, dy, sw - aw, sh,
				src_buf, src_w, src_h, sx + aw, sy, use_alpha, alpha);
	}
	return ret;
}

static int32_t g2dd_handle_fill_rect(proto_t* in) {
	g2d_fill_req_t req;
	g2d_attached_t dst;
	int32_t ret = -1;

	if(in == NULL)
		return -1;
	if(proto_read_to(in, &req, sizeof(req)) != sizeof(req))
		return -1;

	if(g2d_attach(&req.dst, &dst) != 0)
		return -1;

	/* clip the fill rect to the canvas: the back end gets exact
	   in-bounds coordinates only */
	if(!g2d_clip_rect((int32_t)dst.width, (int32_t)dst.height,
			&req.rect.x, &req.rect.y, &req.rect.w, &req.rect.h)) {
		g2d_detach(&dst);
		return 0; /* fully clipped: nothing to fill */
	}

	G2DD_LOG("g2d_fill rect dst: %d x %d, color: 0x%08X, dst:contig: %d:(0x%08X)\n", dst.width, dst.height, req.color, dst.contig, dst.phy);

	if(((req.color >> 24) & 0xff) == 0xff) {
		ret = bsp_g2d_fill(dst.buffer, dst.phy, dst.contig, (int32_t)dst.width, (int32_t)dst.height,
				req.rect.x, req.rect.y, req.rect.w, req.rect.h, req.color);
	}
	else {
		ret = g2d_fill_alpha(dst.buffer, (int32_t)dst.width, (int32_t)dst.height,
				req.rect.x, req.rect.y, req.rect.w, req.rect.h, req.color);
	}
	g2d_detach(&dst);
	return ret;
}

static int32_t g2d_blit_render(const g2d_attached_t* dst,
		uint32_t* src_buf, ewokos_addr_t src_phy, uint8_t src_contig, int32_t src_w, int32_t src_h,
		int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		const g2d_blit_req_t* req, uint8_t use_alpha) {
	int32_t dx;
	int32_t dy;
	int32_t dw;
	int32_t dh;

	if(dst == NULL || dst->buffer == NULL || src_buf == NULL || req == NULL)
		return -1;
	if(sw <= 0 || sh <= 0 || req->dw <= 0 || req->dh <= 0)
		return -1;

	/* clip the dst rect to the dst canvas, scaling the src rect with the
	   same proportion: the back end gets exact in-bounds coordinates
	   only */
	dx = req->dx;
	dy = req->dy;
	dw = req->dw;
	dh = req->dh;
	if(!g2d_clip_dst((int32_t)dst->width, (int32_t)dst->height,
			&sx, &sy, &sw, &sh, &dx, &dy, &dw, &dh))
		return 0; /* fully clipped: nothing to draw */

	G2DD_LOG("g2d_blit_render src:%d x %d, dst: %d x %d, alpha: %d, src:contig: %d:(0x%08X), dst:contig: %d:(0x%08X)\n", 
            src_w, src_h, dst->width, dst->height, use_alpha, src_contig, src_phy, dst->contig, dst->phy);

	/* 1:1 copies go through the pitch-aligned split (aligned part to the
	   NEON back end, sub-alignment tail on the cpu); scaled blits are
	   scalar inside the back end and need no split */
	if(sw == dw && sh == dh) {
		return g2d_blt_split(dst, src_buf, src_phy, src_contig, src_w, src_h,
				sx, sy, sw, sh, dx, dy, use_alpha, req->alpha);
	}

	if(use_alpha != 0) {
		return bsp_g2d_blt_alpha(src_buf, src_phy, src_contig, src_w, src_h, sx, sy, sw, sh,
				dst->buffer, dst->phy, dst->contig, (int32_t)dst->width, (int32_t)dst->height,
				dx, dy, dw, dh, req->alpha);
	}

	return bsp_g2d_blt(src_buf, src_phy, src_contig, src_w, src_h, sx, sy, sw, sh,
			dst->buffer, dst->phy, dst->contig, (int32_t)dst->width, (int32_t)dst->height,
			dx, dy, dw, dh);
}

/* blit src crop into the dst canvas; with rotation the crop goes into
   temp buffers first, then the rotated result is scaled into the dst
   rect */
static int32_t g2dd_handle_blit(proto_t* in, uint8_t use_alpha) {
	g2d_blit_req_t req;
	g2d_attached_t dst;
	g2d_attached_t src;
	g2d_attached_t cropped;
	g2d_attached_t rotated;
	int32_t degree;
	int32_t rw;
	int32_t rh;
	int32_t ret;

	if(in == NULL)
		return -1;
	if(proto_read_to(in, &req, sizeof(req)) != sizeof(req))
		return -1;
	if(req.sw <= 0 || req.sh <= 0)
		return -1;

	if(g2d_attach(&req.dst, &dst) != 0)
		return -1;
	if(g2d_attach(&req.src, &src) != 0) {
		g2d_detach(&dst);
		return -1;
	}

	ret = -1;
	/* clip the crop rect to the source canvas (cutting), scaling the dst
	   rect with the same proportion; g2d_blit_render clips the dst side
	   the same way */
	if(req.sx < 0) {
		int32_t cut = (int32_t)((int64_t)(-req.sx) * req.dw / req.sw);
		req.dx += cut; req.dw -= cut;
		req.sw += req.sx; req.sx = 0;
	}
	if(req.sy < 0) {
		int32_t cut = (int32_t)((int64_t)(-req.sy) * req.dh / req.sh);
		req.dy += cut; req.dh -= cut;
		req.sh += req.sy; req.sy = 0;
	}
	if(req.sx + req.sw > (int32_t)src.width) {
		int32_t over = req.sx + req.sw - (int32_t)src.width;
		int32_t cut = (int32_t)((int64_t)over * req.dw / req.sw);
		req.dw -= cut; req.sw -= over;
	}
	if(req.sy + req.sh > (int32_t)src.height) {
		int32_t over = req.sy + req.sh - (int32_t)src.height;
		int32_t cut = (int32_t)((int64_t)over * req.dh / req.sh);
		req.dh -= cut; req.sh -= over;
	}
	if(req.sw <= 0 || req.sh <= 0 || req.dw <= 0 || req.dh <= 0)
		goto done;

	/* rotate is clockwise degrees, normalized to [0, 360) */
	degree = g2d_norm_degree(req.rotate);

	/* no rotation: blt scales and clips the crop rect directly */
	if(degree == 0) {
		ret = g2d_blit_render(&dst, src.buffer, src.phy, src.contig,
				(int32_t)src.width, (int32_t)src.height,
				req.sx, req.sy, req.sw, req.sh, &req, use_alpha);
		goto done;
	}

	/* rotated path: crop into a temp surface, then rotate into another;
	   the temp surfaces are shm-backed so they carry phy/contig to the
	   back end like any client canvas */
	if(g2d_alloc_surface(req.sw, req.sh, &cropped) != 0)
		goto done;
	ret = g2d_blt_split(&cropped, src.buffer, src.phy, src.contig,
			(int32_t)src.width, (int32_t)src.height,
			req.sx, req.sy, req.sw, req.sh, 0, 0, 0, 0xff);
	if(ret != 0) {
		g2d_detach(&cropped);
		goto done;
	}

	bsp_g2d_rotated_size(req.sw, req.sh, degree, &rw, &rh);
	if(rw <= 0 || rh <= 0) {
		g2d_detach(&cropped);
		ret = -1;
		goto done;
	}
	if(g2d_alloc_surface(rw, rh, &rotated) != 0) {
		g2d_detach(&cropped);
		goto done;
	}
	ret = bsp_g2d_rotate(cropped.buffer, cropped.phy, cropped.contig, req.sw, req.sh,
			rotated.buffer, rotated.phy, rotated.contig, rw, rh, degree);
	g2d_detach(&cropped);
	if(ret != 0) {
		g2d_detach(&rotated);
		goto done;
	}

	ret = g2d_blit_render(&dst, rotated.buffer, rotated.phy, rotated.contig, rw, rh,
			0, 0, rw, rh, &req, use_alpha);
	g2d_detach(&rotated);

done:
	g2d_detach(&src);
	g2d_detach(&dst);
	return ret;
}

/* rotate the src canvas into the dst canvas; the dst canvas size must
   match the rotated size computed by the backend */
static int32_t g2dd_handle_rotate(proto_t* in) {
	g2d_rotate_req_t req;
	g2d_attached_t src;
	g2d_attached_t dst;
	int32_t degree;
	int32_t rw;
	int32_t rh;
	int32_t ret = -1;

	if(in == NULL)
		return -1;
	if(proto_read_to(in, &req, sizeof(req)) != sizeof(req))
		return -1;

	degree = g2d_norm_degree(req.rotate);
	if(degree == 0)
		return -1;

	if(g2d_attach(&req.src, &src) != 0)
		return -1;
	if(g2d_attach(&req.dst, &dst) != 0) {
		g2d_detach(&src);
		return -1;
	}

	ret = -1;
	bsp_g2d_rotated_size((int32_t)src.width, (int32_t)src.height,
			degree, &rw, &rh);
	if(rw == (int32_t)dst.width && rh == (int32_t)dst.height) {
		ret = bsp_g2d_rotate(src.buffer, src.phy, src.contig, (int32_t)src.width, (int32_t)src.height,
				dst.buffer, dst.phy, dst.contig, rw, rh, degree);
	}

	g2d_detach(&src);
	g2d_detach(&dst);
	return ret;
}

static int32_t g2dd_handle_scale_to(proto_t* in) {
	g2d_scale_to_req_t req;
	g2d_attached_t src;
	g2d_attached_t dst;
	int32_t ret= -1;

	if(in == NULL)
		return -1;
	if(proto_read_to(in, &req, sizeof(req)) != sizeof(req))
		return -1;

	if(g2d_attach(&req.src, &src) != 0)
		return -1;
	if(g2d_attach(&req.dst, &dst) != 0) {
		g2d_detach(&src);
		return -1;
	}

	G2DD_LOG("g2dd_handle_scale_to %d x %d, src:contig: %d:(0x%08X), dst:contig: %d:(0x%08X)\n", src.width, src.height, src.contig, src.phy, dst.contig, dst.phy);

	/* same-size scale_to is a plain 1:1 copy: route it through the pitch
	   split instead of the scaler's internal full-buffer row copy */
	if(src.width == dst.width && src.height == dst.height) {
		ret = g2d_blt_split(&dst, src.buffer, src.phy, src.contig,
				(int32_t)src.width, (int32_t)src.height,
				0, 0, (int32_t)src.width, (int32_t)src.height, 0, 0, 0, 0xff);
	}
	else {
		ret = bsp_g2d_scale_to(src.buffer, src.phy, src.contig, (int32_t)src.width, (int32_t)src.height,
				dst.buffer, dst.phy, dst.contig, (int32_t)dst.width, (int32_t)dst.height);
	}

	g2d_detach(&src);
	g2d_detach(&dst);
	return ret;
}

/* blit a src crop 1:1 into a raw physical destination (scan-out
   buffer): no dst canvas to attach, the back end writes the physical
   range directly. the src must be a contig shm canvas (the only kind
   the hardware 2d can read); the dst range is validated against the
   declared geometry and size here, and against the ram windows inside
   the back end. */
static int32_t g2dd_handle_blit_to_phy(proto_t* in) {
	g2d_blit_to_phy_req_t req;
	g2d_attached_t src;
	int32_t sx, sy, sw, sh;
	int32_t dx, dy, dw, dh;
	int32_t ret = -1;

	if(in == NULL)
		return -1;
	if(proto_read_to(in, &req, sizeof(req)) != sizeof(req))
		return -1;
	if(req.sw <= 0 || req.sh <= 0 || req.dw <= 0 || req.dh <= 0 ||
			req.dst_w <= 0 || req.dst_h <= 0 || req.dst_size == 0)
		return -1;
	/* 1:1 copy only; the dst stride must be whole pixels */
	if(req.sw != req.dw || req.sh != req.dh)
		return -1;
	if(req.pitch < (uint32_t)req.dst_w * 4u || (req.pitch & 3u) != 0)
		return -1;

	if(g2d_attach(&req.src, &src) != 0)
		return -1;
	if(src.contig == 0 || src.phy == 0)
		goto done;

	/* clip the crop to the src canvas, scaling the dst rect with the
	   same proportion (exact here: 1:1) */
	sx = req.sx; sy = req.sy; sw = req.sw; sh = req.sh;
	dx = req.dx; dy = req.dy; dw = req.dw; dh = req.dh;
	if(!g2d_clip_dst((int32_t)src.width, (int32_t)src.height,
			&dx, &dy, &dw, &dh, &sx, &sy, &sw, &sh))
		goto done;
	if(sw != dw || sh != dh)
		goto done;
	/* clip the dst rect to the visible geometry */
	if(!g2d_clip_dst(req.dst_w, req.dst_h,
			&sx, &sy, &sw, &sh, &dx, &dy, &dw, &dh))
		goto done;
	if(sw != dw || sh != dh)
		goto done;
	/* the touched rows must fit the declared physical segment */
	if((uint64_t)(dy + dh - 1) * req.pitch +
			(uint64_t)(dx + dw) * 4u > req.dst_size)
		goto done;

	G2DD_LOG("g2dd_handle_blit_to_phy %d x %d, src:contig: %d:(0x%08X), dst: 0x%08X sz:%d\n", src.width, src.height, src.contig, src.phy, req.dst_phy, req.dst_size);
	ret = bsp_g2d_blt_phy(src.buffer, src.phy, src.contig,
			(int32_t)src.width, (int32_t)src.height, sx, sy, sw, sh,
			req.dst_phy, req.dst_size, req.dst_w, req.dst_h, req.pitch,
			dx, dy, dw, dh);
done:
	g2d_detach(&src);
	return ret;
}

static char* g2d_strdup(const char* s) {
	size_t len;
	char* ret;

	if(s == NULL)
		return NULL;
	len = strlen(s);
	ret = (char*)malloc(len + 1);
	if(ret == NULL)
		return NULL;
	memcpy(ret, s, len + 1);
	return ret;
}

static char* g2d_cmd(vdevice_t* dev, int from_pid, int argc, char** argv, void* p) {
	(void)dev;
	(void)from_pid;
	(void)p;

	if(argc <= 0 || argv == NULL || argv[0] == NULL)
		return NULL;

	if(strcmp(argv[0], "info") == 0)
		return g2d_strdup("stateless argb8888 shm canvases via soft");

	/* `devcmd /dev/g2d cache 0|1` toggles the attach cache at runtime,
	   `devcmd /dev/g2d cache` reports the hit/miss/evict counters */
	if(strcmp(argv[0], "cache") == 0) {
		char buf[192];
		uint32_t slots = 0;
		for(uint32_t i = 0; i < G2D_ATTACH_CACHE_MAX; i++) {
			if(_g2d_attach_cache[i].used)
				slots++;
		}
		if(argc > 1 && argv[1] != NULL) {
			int32_t on = (atoi(argv[1]) != 0) ? 1 : 0;
			/* never leave cached mappings behind a bypass: their shmdt
			   would come from a path the cache cannot see */
			if(on == 0 && _g2d_cache_enabled != 0)
				g2d_cache_drain();
			_g2d_cache_enabled = on;
		}
		snprintf(buf, sizeof(buf),
				"cache %s hits %llu miss %llu sweep %llu evict %llu stale %llu slots %u/%u pin %uKB/%uKB",
				_g2d_cache_enabled ? "on" : "off",
				(unsigned long long)_g2d_cache_hits,
				(unsigned long long)_g2d_cache_misses,
				(unsigned long long)_g2d_cache_swept,
				(unsigned long long)_g2d_cache_evicts,
				(unsigned long long)_g2d_cache_stale,
				(unsigned int)slots,
				(unsigned int)G2D_ATTACH_CACHE_MAX,
				(unsigned int)(_g2d_cache_bytes / 1024),
				(unsigned int)(g2d_cache_budget() / 1024));
		return g2d_strdup(buf);
	}
	return NULL;
}

static int g2d_dev_cntl(vdevice_t* dev, int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
	(void)dev;
	(void)from_pid;
	(void)ret;
	(void)p;

	int res = -1;
	uint32_t clock_hz = 0;
	int is_clock = (cmd == G2D_DEV_CNTL_GET_CLOCK);
	g2d_task_lock();
	_g2dd_req_cnt++;
	switch (cmd) {
	case G2D_DEV_CNTL_FILL_RECT:
		res = g2dd_handle_fill_rect(in);
		break;
	case G2D_DEV_CNTL_BLIT:
		res = g2dd_handle_blit(in, 0);
		break;
	case G2D_DEV_CNTL_BLIT_ALPHA:
		res = g2dd_handle_blit(in, 1);
		break;
	case G2D_DEV_CNTL_ROTATE:
		res = g2dd_handle_rotate(in);
		break;
	case G2D_DEV_CNTL_SCALE_TO:
		res = g2dd_handle_scale_to(in);
		break;
	case G2D_DEV_CNTL_GET_CLOCK:
		/* report the engine clock pinned at startup; 0 Hz on
		   backends without one is "cannot report" */
		clock_hz = bsp_g2d_clock_hz();
		res = (clock_hz > 0) ? 0 : -1;
		break;
	case G2D_DEV_CNTL_BLIT_TO_PHY:
		res = g2dd_handle_blit_to_phy(in);
		break;
	default:
		res = -1;
	}
	g2d_task_unlock();

	if(res != 0 && !is_clock) {
		G2DD_LOG("g2d_dev_cntl: ret is not 0!\n");
	}
	PF->clear(ret)->addi(ret, res);
	if(is_clock && res == 0)
		PF->addi(ret, clock_hz);
	return res;
}

int main(int argc, char** argv) {
	const char* mnt_point;
	vdevice_t dev;

	mnt_point = (argc > 1) ? argv[1] : "/dev/g2d";

	if(bsp_g2d_init() != 0)
		return -1;

	memset(&dev, 0, sizeof(dev));
	strcpy(dev.desc, "g2d");
	dev.dev_cntl = g2d_dev_cntl;
	dev.cmd = g2d_cmd;
	dev.extra_data = NULL;

	if(device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666, true) != 0)
		return -1;
	return 0;
}
