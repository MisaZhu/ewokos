#include <mm/kmalloc.h>
#include <mm/kalloc.h>
#include <mm/dma.h>
#include <mm/mmu.h>
#include <kernel/hw_info.h>
#include <kernel/proc.h>
#include <kernel/system.h>
#include <stddef.h>

typedef struct st_dma {
    int32_t  pid;
    uint32_t pins;
    uint32_t size;
    ewokos_addr_t base;
    struct st_dma* next;
    struct st_dma* prev;
} dma_t;

typedef struct {
    dma_t* head;

    ewokos_addr_t phy_base;
    ewokos_addr_t v_base;
    /* extent of the block's physical range; ewokos_addr_t because the dynamic
     * sys_dma range may span >4GB (static carve-outs stay small). */
    ewokos_addr_t size;

    uint32_t flags;
    int32_t  owner_pid;
    bool shared;
    /* dynamic blocks (the sys_dma range) draw physically-contiguous runs
     * from general RAM via kalloc_contig_compact instead of sub-allocating
     * a reserved carve-out. head tracks live/pinned allocations only. */
    bool dynamic;
} dma_block_t;

#define DMA_BLOCK_MAX   16
static uint32_t _dma_block_count = 0;
static dma_block_t _dma_blocks[DMA_BLOCK_MAX];

static dma_t* dma_new(ewokos_addr_t base, uint32_t size) {
    dma_t* ret = (dma_t*)kcalloc(1, sizeof(dma_t));
    ret->size = size;
    ret->base = base;
    return ret;
}

/*
 * Flip the kernel direct-map alias of a contiguous run between WRBACK
 * (general RAM) and PTE_ATTR_SYS_DMA (NOCACHE). A DMA-owned page must not
 * keep a cached alias, so every page of the run is rewritten and a global
 * TLB broadcast follows. Before WRBACK->NOCACHE the run is cleaned to DRAM
 * (dcache_flush_range) so stale dirty lines from a previous owner cannot
 * later evict over DMA-written data. Caller holds the kernel lock.
 */
static void dma_pages_set_attr(ewokos_addr_t paddr, uint32_t npages,
        uint32_t attr) {
    for(uint32_t j = 0; j < npages; j++) {
        ewokos_addr_t pa = paddr + (ewokos_addr_t)j * PAGE_SIZE;
        if(attr == PTE_ATTR_SYS_DMA)
            dcache_flush_range((const void*)P2V(pa), PAGE_SIZE);
        arch_set_direct_map_attr(pa, attr);
    }
    flush_tlb();
}

/* Return a dynamic allocation's physical run to general RAM: flip the direct
 * map back to WRBACK, hand the pages to kalloc, and unlink the node. Only
 * called once both ownership (pid==0) and device references (pins==0) drop. */
static void dma_dynamic_release_node(uint32_t block_id, dma_t* d) {
    dma_pages_set_attr(d->base, d->size / PAGE_SIZE, PTE_ATTR_WRBACK);
    kfree_contig((void*)P2V(d->base), d->size / PAGE_SIZE);
    if(d->prev != NULL)
        d->prev->next = d->next;
    else
        _dma_blocks[block_id].head = d->next;
    if(d->next != NULL)
        d->next->prev = d->prev;
    kfree(d);
}

/*
 * Whether compaction (page_move) may relocate a physical page. Any page
 * tracked by a DMA allocation node - live or device-pinned - is excluded:
 * its physical run is owned by the DMA layer and a PTE retarget alone cannot
 * update the device's view. Pages outside every DMA node are general RAM
 * (or free) and may be moved subject to the refcount check in the caller.
 */
int32_t dma_page_movable(ewokos_addr_t paddr) {
    for(uint32_t i = 0; i < _dma_block_count; i++) {
        for(dma_t* d = _dma_blocks[i].head; d != NULL; d = d->next) {
            if(d->base <= paddr && paddr < d->base + d->size)
                return 0;
        }
    }
    return 1;
}

static bool dma_reusable(const dma_t* d) {
    return d != NULL && d->pid == 0 && d->pins == 0;
}

/* Caller must not keep a node pointer across this operation. A free request
 * may remove its CPU/owner lifetime while a device still holds a reference. */
static void dma_coalesce(dma_t* d) {
    if(!dma_reusable(d))
        return;
    if(dma_reusable(d->prev)) {
        dma_t* prev = d->prev;
        prev->size += d->size;
        prev->next = d->next;
        if(d->next != NULL)
            d->next->prev = prev;
        kfree(d);
        d = prev;
    }
    while(dma_reusable(d->next)) {
        dma_t* next = d->next;
        d->size += next->size;
        d->next = next->next;
        if(d->next != NULL)
            d->next->prev = d;
        kfree(next);
    }
}

int32_t dma_pin(int32_t dma_block_id, int32_t pid,
        ewokos_addr_t phy_addr, uint32_t size) {
    if(dma_block_id < 0 || dma_block_id >= (int32_t)_dma_block_count ||
            pid == 0 || size == 0)
        return -1;
    for(dma_t* d = _dma_blocks[dma_block_id].head; d != NULL; d = d->next) {
        if(d->pid == pid && d->base == phy_addr && size <= d->size) {
            if(d->pins == UINT32_MAX)
                return -1;
            d->pins++;
            return 0;
        }
    }
    return -1;
}

int32_t dma_pin_query(int32_t dma_block_id, ewokos_addr_t phy_addr,
        int32_t *owner, uint32_t *pins) {
    if(dma_block_id < 0 || dma_block_id >= (int32_t)_dma_block_count ||
            !owner || !pins)
        return -1;
    for(dma_t* d = _dma_blocks[dma_block_id].head; d != NULL; d = d->next) {
        if(d->base == phy_addr && d->pins != 0) {
            *owner = d->pid;
            *pins = d->pins;
            return 0;
        }
    }
    return -1;
}

int32_t dma_unpin(int32_t dma_block_id, ewokos_addr_t phy_addr) {
    if(dma_block_id < 0 || dma_block_id >= (int32_t)_dma_block_count)
        return -1;
    for(dma_t* d = _dma_blocks[dma_block_id].head; d != NULL; d = d->next) {
        if(d->base == phy_addr && d->pins != 0) {
            d->pins--;
            if(_dma_blocks[dma_block_id].dynamic) {
                if(d->pins == 0 && d->pid == 0)
                    dma_dynamic_release_node((uint32_t)dma_block_id, d);
            }
            else {
                dma_coalesce(d);
            }
            return 0;
        }
    }
    return -1;
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
        _dma_blocks[i].dynamic = false;
    }
    /* Block 0 is the sys_dma range: a dynamic contiguous allocator over
     * general RAM, not a reserved carve-out. phy_base/size describe the
     * DMA-reachable range; v_base anchors the identity DMA window. */
    _dma_blocks[0].phy_base = _sys_info.sys_dma.phy_base;
    _dma_blocks[0].v_base = _sys_info.sys_dma.v_base;
    _dma_blocks[0].size = _sys_info.sys_dma.size;
    _dma_blocks[0].owner_pid = -1;
    _dma_blocks[0].head = NULL;
    _dma_blocks[0].dynamic = true;
    _dma_block_count = 1;
}

void dma_free(int32_t dma_block_id, int32_t pid, ewokos_addr_t phy_addr) {
    if(dma_block_id < 0 || dma_block_id >= (int32_t)_dma_block_count)
        return;

    dma_t* d = _dma_blocks[dma_block_id].head;
    while(d != NULL) {
        dma_t* next = d->next;
        if(d->pid == pid && d->base == phy_addr) {
            d->pid = 0;
            if(_dma_blocks[dma_block_id].dynamic) {
                /* device still references the run: retain storage until the
                 * final unpin; otherwise hand the pages back to general RAM */
                if(d->pins == 0)
                    dma_dynamic_release_node((uint32_t)dma_block_id, d);
            }
            else {
                dma_coalesce(d);
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
    for(uint32_t i=0; i<_dma_block_count; i++) {
        dma_t* d = _dma_blocks[i].head;
        while(d != NULL) {
            dma_t* next = d->next;
            if(d->pid == pid)
                d->pid = 0;

            if(_dma_blocks[i].dynamic) {
                /* dynamic storage returns to general RAM once both owner and
                 * device references are gone; pinned runs stay until unpin */
                if(d->pid == 0 && d->pins == 0)
                    dma_dynamic_release_node(i, d);
            }
            /* only merge FREE nodes; merging a live allocation into a free
             * prev would mark it reusable and hand the same physical range
             * to the next dma_alloc caller (scan-out/XHCI corruption). */
            else {
                if(dma_reusable(d) && dma_reusable(d->prev)) { //merge
                    d->prev->next = next; 
                    if(next != NULL)
                    next->prev = d->prev;
                    d->prev->size += d->size;
                    kfree(d);
                }
            }
            d = next;
        }
    }
}

ewokos_addr_t dma_alloc(int32_t dma_block_id, int32_t pid, uint32_t size) {
    if(dma_block_id < 0 || dma_block_id >= (int32_t)_dma_block_count)
        return 0;

    size = ALIGN_UP(size, PAGE_SIZE);

    if(_dma_blocks[dma_block_id].dynamic) {
        /* Draw a physically-contiguous run from general RAM inside the DMA
         * range, compacting via page_move when no free run exists. */
        uint32_t npages = size / PAGE_SIZE;
        ewokos_addr_t limit = _dma_blocks[dma_block_id].phy_base +
            _dma_blocks[dma_block_id].size;
        void* base = kalloc_contig_compact(npages, limit);
        if(base == NULL)
            return 0;
        ewokos_addr_t paddr = V2P((ewokos_addr_t)base);
        /* No cached alias may survive while DMA owns the run: flip the
         * direct map to the NOCACHE DMA attribute. */
        dma_pages_set_attr(paddr, npages, PTE_ATTR_SYS_DMA);
        dma_t* n = dma_new(paddr, size);
        n->pid = pid;
        n->next = _dma_blocks[dma_block_id].head;
        if(_dma_blocks[dma_block_id].head != NULL)
            _dma_blocks[dma_block_id].head->prev = n;
        _dma_blocks[dma_block_id].head = n;
        return paddr;
    }

    dma_t* d = _dma_blocks[dma_block_id].head;
    while(d != NULL) {
        if(!dma_reusable(d) || d->size < size) {
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
        /* A dead/recycled block owner does not revoke device access. Do
         * not destroy the only records retaining its physical storage. */
        for(dma_t* d = _dma_blocks[i].head; d != NULL; d = d->next)
            if(d->pins != 0)
                return -1;
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
        _dma_blocks[i].dynamic = false;
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
    _dma_blocks[_dma_block_count].dynamic = false;
    int32_t ret = _dma_block_count;
    _dma_block_count++;
    return ret;
}
