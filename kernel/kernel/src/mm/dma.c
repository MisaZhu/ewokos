#include <mm/kmalloc.h>
#include <mm/dma.h>
#include <mm/mmu.h>
#include <kernel/hw_info.h>
#include <kernel/proc.h>
#include <kernel/system.h>
#include <stddef.h>

typedef struct st_dma {
    int32_t  pid;
    uint32_t size;
    ewokos_addr_t base;
    struct st_dma* next;
    struct st_dma* prev;
} dma_t;

typedef struct {
    dma_t* head;

    ewokos_addr_t phy_base;
    ewokos_addr_t v_base;
    uint32_t size;

    uint32_t flags;
    int32_t  owner_pid;
    bool shared;
} dma_block_t;

#define DMA_BLOCK_MAX   16
static uint32_t _dma_block_count = 0;
static dma_block_t _dma_blocks[DMA_BLOCK_MAX];

/*
 * Cross-proc dma mapping tracker.
 *
 * dma_alloc() maps its buffer only into the allocator's vm, so another
 * root daemon that needs the same physical range (g2dd attaching a
 * client's dma canvas) pulls it in through sys_mem_map(). That peer
 * mapping is invisible to the owner's dma_t, so when the owner dies
 * dma_release() would mark the physical range free while the peer's
 * page tables still point at it - the next dma_alloc hands the same
 * memory to a third party and the peer silently corrupts it.
 *
 * Every sys_mem_map of a sys_dma range records (owner_pid, peer_pid,
 * peer_vaddr, paddr, size) here. dma_release() revokes the peer's
 * mapping before freeing the range (safety net); a well-behaved peer
 * drops it voluntarily via SYS_DMA_UNMAP (protocol path); a peer that
 * dies first is cleaned up by dma_peer_map_forget_peer() from its own
 * proc_funeral. Bounded table: when full, sys_mem_map of a dma range
 * is refused rather than tracked-but-unrevocable.
 */
#define DMA_PEER_MAP_MAX 64
typedef struct {
    int32_t owner_pid;        /* proc that owns the underlying dma_alloc */
    int32_t peer_pid;         /* proc that mapped it via sys_mem_map */
    ewokos_addr_t peer_vaddr; /* vaddr in the peer's vm */
    ewokos_addr_t paddr;      /* physical base of the mapped range */
    uint32_t size;            /* bytes mapped (page aligned) */
    uint8_t used;
} dma_peer_map_t;

static dma_peer_map_t _dma_peer_maps[DMA_PEER_MAP_MAX];

static dma_t* dma_new(ewokos_addr_t base, uint32_t size) {
    dma_t* ret = (dma_t*)kcalloc(1, sizeof(dma_t));
    ret->size = size;
    ret->base = base;
    return ret;
}

void dma_init(void) {
    _dma_block_count = 0;
    for(uint32_t i=0; i<DMA_BLOCK_MAX; i++) {
        _dma_blocks[i].head = NULL;
        _dma_blocks[i].owner_pid = -1;
        _dma_blocks[i].flags = 0;
        _dma_blocks[i].phy_base= 0;
        _dma_blocks[i].size = 0;
        _dma_blocks[i].shared = false;
    }
    for(uint32_t i=0; i<DMA_PEER_MAP_MAX; i++) {
        _dma_peer_maps[i].used = 0;
        _dma_peer_maps[i].owner_pid = 0;
        _dma_peer_maps[i].peer_pid = 0;
    }
    dma_set(-1, _sys_info.sys_dma.phy_base, _sys_info.sys_dma.v_base, _sys_info.sys_dma.size, false);
}

void dma_free(int32_t dma_block_id, int32_t pid, ewokos_addr_t phy_addr) {
    if(dma_block_id < 0 || dma_block_id >= (int32_t)_dma_block_count)
        return;

    dma_t* d = _dma_blocks[dma_block_id].head;
    while(d != NULL) {
        dma_t* next = d->next;
        if(d->pid == pid && d->base == phy_addr) {
            d->pid = 0;
            if(d->prev != NULL && d->prev->pid == 0) { //merge
                d->prev->next = next; 
                if(next != NULL)
                next->prev = d->prev;
                d->prev->size += d->size;
                kfree(d);
            }
            return;
        }
        d = next;
    }
}

uint32_t  dma_size(int32_t dma_block_id, int32_t pid, ewokos_addr_t phy_addr) {
    if(dma_block_id < 0 || dma_block_id >= (int32_t)_dma_block_count)
        return 0;

    dma_t* d = _dma_blocks[dma_block_id].head;
    while(d != NULL) {
        if(d->pid == pid && d->base == phy_addr) {
            return d->size;
        }
        d = d->next;
    }
    return 0;
}

void dma_release(int32_t pid) {
    /*
     * Revoke cross-proc peer mappings of this owner's dma ranges BEFORE
     * the sub-allocations are marked free. Without this the range becomes
     * reusable while a peer (g2dd) still has it mapped, and the next
     * dma_alloc hands the same physical memory to a third party. The peer
     * is unmapped in its own vm (no TTBR switch needed: unmap_pages walks
     * the passed page-dir); a global flush_tlb() drops any cached walk on
     * every core. A peer that already died is skipped - its vm is torn
     * down by its own proc_funeral.
     */
    dma_peer_map_revoke_owner(pid);

    for(uint32_t i=0; i<_dma_block_count; i++) {
        dma_t* d = _dma_blocks[i].head;
        while(d != NULL) {
            dma_t* next = d->next;
            if(d->pid == pid)
                d->pid = 0;

            /* only merge FREE nodes; merging a live allocation into a free
             * prev would mark it reusable and hand the same physical range
             * to the next dma_alloc caller (scan-out/XHCI corruption). */
            if(d->pid == 0 && d->prev != NULL && d->prev->pid == 0) { //merge
                d->prev->next = next; 
                if(next != NULL)
                next->prev = d->prev;
                d->prev->size += d->size;
                kfree(d);
            }
            d = next;
        }
    }
}

/*
 * Live owner pid of the dma sub-allocation fully containing
 * [paddr, paddr+size), or -1 when the range is not inside any live
 * allocation. Used by sys_mem_map to attribute a cross-proc mapping to
 * the owner whose dma_release() must later revoke it.
 */
static int32_t dma_find_owner(ewokos_addr_t paddr, uint32_t size) {
    for(uint32_t i=0; i<_dma_block_count; i++) {
        dma_t* d = _dma_blocks[i].head;
        while(d != NULL) {
            if(d->pid != 0 &&
                    d->base <= paddr &&
                    (d->base + d->size) >= (paddr + size))
                return d->pid;
            d = d->next;
        }
    }
    return -1;
}

static int32_t dma_peer_map_track(int32_t owner_pid, int32_t peer_pid,
        ewokos_addr_t peer_vaddr, ewokos_addr_t paddr, uint32_t size) {
    if(owner_pid <= 0 || peer_pid <= 0)
        return -1;
    /* dedup: the same peer re-mapping the same range refreshes the entry
       instead of consuming a second slot (g2dd's attach cache re-hits) */
    for(uint32_t i=0; i<DMA_PEER_MAP_MAX; i++) {
        dma_peer_map_t* m = &_dma_peer_maps[i];
        if(m->used && m->peer_pid == peer_pid &&
                m->peer_vaddr == peer_vaddr && m->paddr == paddr) {
            m->owner_pid = owner_pid;
            m->size = size;
            return (int32_t)i;
        }
    }
    for(uint32_t i=0; i<DMA_PEER_MAP_MAX; i++) {
        dma_peer_map_t* m = &_dma_peer_maps[i];
        if(!m->used) {
            m->owner_pid = owner_pid;
            m->peer_pid = peer_pid;
            m->peer_vaddr = peer_vaddr;
            m->paddr = paddr;
            m->size = size;
            m->used = 1;
            return (int32_t)i;
        }
    }
    return -2; /* table full: caller must refuse the map, an untracked
                  mapping could never be revoked on owner death */
}

int32_t dma_peer_map_by_paddr(int32_t peer_pid, ewokos_addr_t peer_vaddr,
        ewokos_addr_t paddr, uint32_t size) {
    int32_t owner_pid = dma_find_owner(paddr, size);
    if(owner_pid <= 0)
        return -1; /* range not inside a live allocation: nothing to track,
                      mapping still allowed (matches a free/reserved range) */
    return dma_peer_map_track(owner_pid, peer_pid, peer_vaddr, paddr, size);
}

void dma_peer_map_revoke_owner(int32_t owner_pid) {
    if(owner_pid <= 0)
        return;
    bool flushed = false;
    for(uint32_t i=0; i<DMA_PEER_MAP_MAX; i++) {
        dma_peer_map_t* m = &_dma_peer_maps[i];
        if(!m->used || m->owner_pid != owner_pid)
            continue;
        proc_t* peer = proc_get(m->peer_pid);
        if(peer != NULL && peer->space != NULL &&
                peer->space->vm != NULL &&
                peer->info.state != UNUSED) {
            unmap_pages(peer->space->vm, m->peer_vaddr, m->size / PAGE_SIZE);
            flushed = true;
        }
        m->used = 0;
    }
    if(flushed)
        flush_tlb();
}

void dma_peer_map_forget_peer(int32_t peer_pid) {
    if(peer_pid <= 0)
        return;
    /*
     * No unmap here: this runs from the peer's own proc_funeral, where
     * free_page_tables() tears down the peer's whole vm right after. Just
     * drop the tracking slots so a later owner-side revoke does not walk
     * a stale entry and so the table stays reusable.
     */
    for(uint32_t i=0; i<DMA_PEER_MAP_MAX; i++) {
        dma_peer_map_t* m = &_dma_peer_maps[i];
        if(m->used && m->peer_pid == peer_pid)
            m->used = 0;
    }
}

int32_t dma_peer_unmap(int32_t peer_pid, ewokos_addr_t peer_vaddr) {
    if(peer_pid <= 0)
        return -1;
    for(uint32_t i=0; i<DMA_PEER_MAP_MAX; i++) {
        dma_peer_map_t* m = &_dma_peer_maps[i];
        if(!m->used || m->peer_pid != peer_pid || m->peer_vaddr != peer_vaddr)
            continue;
        proc_t* peer = proc_get(peer_pid);
        if(peer != NULL && peer->space != NULL && peer->space->vm != NULL) {
            unmap_pages(peer->space->vm, peer_vaddr, m->size / PAGE_SIZE);
            flush_tlb();
        }
        m->used = 0;
        return 0;
    }
    return -1;
}

ewokos_addr_t dma_alloc(int32_t dma_block_id, int32_t pid, uint32_t size) {
    if(dma_block_id < 0 || dma_block_id >= (int32_t)_dma_block_count)
        return 0;

    size = ALIGN_UP(size, PAGE_SIZE);
    dma_t* d = _dma_blocks[dma_block_id].head;
    while(d != NULL) {
        if(d->pid != 0 || d->size < size) {
            d = d->next;
            continue;
        }

        d->pid = pid;
        if(d->size > size) {
            dma_t* n = dma_new(d->base + size, d->size - size);
            d->size = size;
            n->prev = d;
            n->next = d->next;
            if(d->next != NULL)
                d->next->prev = n;
            d->next = n;
        }
        return d->base;
    }
    return 0;
}

ewokos_addr_t dma_phy_addr(int32_t dma_block_id, ewokos_addr_t vaddr) {
    if(dma_block_id < 0 || dma_block_id >= (int32_t)_dma_block_count)
        return 0;

    return vaddr - _dma_blocks[dma_block_id].v_base + _dma_blocks[dma_block_id].phy_base;
}

ewokos_addr_t dma_v_addr(int32_t dma_block_id, ewokos_addr_t phy_addr) {
    if(dma_block_id < 0 || dma_block_id >= (int32_t)_dma_block_count)
        return 0;

    return phy_addr - _dma_blocks[dma_block_id].phy_base + _dma_blocks[dma_block_id].v_base;
}

int32_t  dma_set(int32_t pid, ewokos_addr_t phy_base, ewokos_addr_t v_base, uint32_t size, bool shared) {
    if(pid >= 0) {
        proc_t* cproc = proc_get(pid);
        if(cproc == NULL || cproc->info.uid != 0)
            return -1;
    }

    ewokos_addr_t pbase = ALIGN_UP(phy_base, PAGE_SIZE);
    ewokos_addr_t vbase = ALIGN_UP(v_base, PAGE_SIZE);
    size = ALIGN_DOWN(size, PAGE_SIZE);

    if(pbase != phy_base) {
        if(size < (PAGE_SIZE*2))
            return -1;
        size -= PAGE_SIZE;
    }

    /*
     * Reclaim a slot whose registered owner died. The physical carve-out
     * is static (reserved by kernel.conf), so a driver that crashes and
     * is respawned by init calls SYS_DMA_SET again on every restart.
     * Without reclaim, _dma_block_count grows one slot per respawn until
     * DMA_BLOCK_MAX is exhausted and dma_set starts failing permanently
     * (reboot required).
     *
     * Reclaim iff the previous owner is either the current caller (pid
     * was recycled to the same driver instance) or is fully gone from
     * the task table, or is a ZOMBIE/UNUSED proc whose funeral is
     * pending. Kernel-registered blocks (owner_pid < 0, e.g. the
     * sys_dma pool seeded by dma_init) are never reclaimable.
     *
     * The sub-allocation list is rebuilt as one full-range free node so
     * the new owner starts clean; the previous owner's dma_release has
     * already run (or is a no-op on the emptied list) by the time the
     * slot becomes reclaimable.
     */
    for(uint32_t i=0; i<_dma_block_count; i++) {
        if(_dma_blocks[i].phy_base != phy_base || _dma_blocks[i].size != size)
            continue;
        int32_t prev_owner = _dma_blocks[i].owner_pid;
        if(prev_owner < 0)
            continue; // kernel-owned block (sys_dma pool), never reclaim
        if(prev_owner != pid) {
            proc_t* prev = proc_get(prev_owner);
            if(prev != NULL &&
                    prev->info.state != UNUSED &&
                    prev->info.state != ZOMBIE)
                continue; // live owner, don't steal
        }
        dma_t* d = _dma_blocks[i].head;
        while(d != NULL) {
            dma_t* next = d->next;
            kfree(d);
            d = next;
        }
        _dma_blocks[i].head = dma_new(phy_base, size);
        _dma_blocks[i].owner_pid = pid;
        _dma_blocks[i].v_base = v_base;
        _dma_blocks[i].shared = shared;
        return (int32_t)i;
    }

    if(_dma_block_count >= DMA_BLOCK_MAX)
        return -1;

    _dma_blocks[_dma_block_count].owner_pid = pid; 
    _dma_blocks[_dma_block_count].phy_base = phy_base; 
    _dma_blocks[_dma_block_count].v_base = v_base; 
    _dma_blocks[_dma_block_count].size = size; 
    _dma_blocks[_dma_block_count].head = dma_new(phy_base, size);
    _dma_blocks[_dma_block_count].shared = shared; 
    int32_t ret = _dma_block_count;
    _dma_block_count++;
    return ret;
}