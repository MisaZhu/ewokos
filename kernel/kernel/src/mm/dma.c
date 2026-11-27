#include <mm/kmalloc.h>
#include <mm/kalloc.h>
#include <mm/dma.h>
#include <mm/mmu.h>
#include <kernel/hw_info.h>
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

/* The sys_dma range is the single DMA pool: a dynamic contiguous allocator
 * over general RAM via mm_alloc_contig, not a reserved carve-out.
 * head tracks live/pinned allocations only. */
static struct {
    dma_t* head;

    ewokos_addr_t phy_base;
    ewokos_addr_t v_base;
    /* extent of the pool's physical range; ewokos_addr_t because the
     * sys_dma range may span >4GB. */
    ewokos_addr_t size;
} _dma;

static dma_t* dma_new(ewokos_addr_t base, uint32_t size) {
    dma_t* ret = (dma_t*)kcalloc(1, sizeof(dma_t));
    ret->size = size;
    ret->base = base;
    return ret;
}

/*
 * Flip the kernel direct-map alias of a contiguous run between WRBACK
 * (general RAM) and a non-cacheable attribute (PTE_ATTR_SYS_DMA for DMA
 * allocations, PTE_ATTR_NOCACHE for shm). A device-owned page must
 * not keep a cached alias, so every page of the run is rewritten and a
 * global TLB broadcast follows. Before any WRBACK->non-cacheable transition
 * the run is cleaned to DRAM (dcache_flush_range) so stale dirty lines from
 * a previous owner cannot later evict over device-written data. Caller holds
 * the kernel lock. Shared by dma_alloc/dma_release and the shm
 * backing (mm/shm.c).
 */
void dma_pages_set_attr(ewokos_addr_t paddr, uint32_t npages,
        uint32_t attr) {
    for(uint32_t j = 0; j < npages; j++) {
        ewokos_addr_t pa = paddr + (ewokos_addr_t)j * PAGE_SIZE;
        if(attr != PTE_ATTR_WRBACK)
            dcache_flush_range((const void*)P2V(pa), PAGE_SIZE);
        arch_set_direct_map_attr(pa, attr);
    }
    flush_tlb();
}

/* Return an allocation's physical run to general RAM: flip the direct map
 * back to WRBACK, hand the pages to kalloc, and unlink the node. Only called
 * once both ownership (pid==0) and device references (pins==0) drop. */
static void dma_release_node(dma_t* d) {
    dma_pages_set_attr(d->base, d->size / PAGE_SIZE, PTE_ATTR_WRBACK);
    kfree_contig((void*)P2V(d->base), d->size / PAGE_SIZE);
    if(d->prev != NULL)
        d->prev->next = d->next;
    else
        _dma.head = d->next;
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
    for(dma_t* d = _dma.head; d != NULL; d = d->next) {
        if(d->base <= paddr && paddr < d->base + d->size)
            return 0;
    }
    return 1;
}

int32_t dma_pin(int32_t pid, ewokos_addr_t phy_addr, uint32_t size) {
    if(pid == 0 || size == 0)
        return -1;
    for(dma_t* d = _dma.head; d != NULL; d = d->next) {
        if(d->pid == pid && d->base == phy_addr && size <= d->size) {
            if(d->pins == UINT32_MAX)
                return -1;
            d->pins++;
            return 0;
        }
    }
    return -1;
}

int32_t dma_pin_query(ewokos_addr_t phy_addr, int32_t *owner, uint32_t *pins) {
    if(!owner || !pins)
        return -1;
    for(dma_t* d = _dma.head; d != NULL; d = d->next) {
        if(d->base == phy_addr && d->pins != 0) {
            *owner = d->pid;
            *pins = d->pins;
            return 0;
        }
    }
    return -1;
}

int32_t dma_unpin(ewokos_addr_t phy_addr) {
    for(dma_t* d = _dma.head; d != NULL; d = d->next) {
        if(d->base == phy_addr && d->pins != 0) {
            d->pins--;
            if(d->pins == 0 && d->pid == 0)
                dma_release_node(d);
            return 0;
        }
    }
    return -1;
}

void dma_init(void) {
    /* The sys_dma range: phy_base/size describe the DMA-reachable range
     * over general RAM; v_base anchors the identity DMA window. */
    _dma.head = NULL;
    _dma.phy_base = _sys_info.sys_dma.phy_base;
    _dma.v_base = _sys_info.sys_dma.v_base;
    _dma.size = _sys_info.sys_dma.size;
}

void dma_free(int32_t pid, ewokos_addr_t phy_addr) {
    dma_t* d = _dma.head;
    while(d != NULL) {
        dma_t* next = d->next;
        if(d->pid == pid && d->base == phy_addr) {
            d->pid = 0;
            /* device still references the run: retain storage until the
             * final unpin; otherwise hand the pages back to general RAM */
            if(d->pins == 0)
                dma_release_node(d);
            return;
        }
        d = next;
    }
}

/* Total physical bytes currently held by the DMA layer: every node on the
 * list owns its contiguous run until BOTH ownership and device pins drop
 * (dma_release_node is the only place pages return to kalloc), so live and
 * freed-but-pinned allocations alike count as occupied. Caller holds the
 * kernel lock, same as every other _dma.head walk. */
ewokos_addr_t dma_alloced_size(void) {
    ewokos_addr_t ret = 0;
    for(dma_t* d = _dma.head; d != NULL; d = d->next)
        ret += d->size;
    return ret;
}

uint32_t dma_size(int32_t pid, ewokos_addr_t phy_addr) {
    dma_t* d = _dma.head;
    while(d != NULL) {
        if(d->pid == pid && d->base == phy_addr) {
            return d->size;
        }
        d = d->next;
    }
    return 0;
}

void dma_release(int32_t pid) {
    dma_t* d = _dma.head;
    while(d != NULL) {
        dma_t* next = d->next;
        if(d->pid == pid)
            d->pid = 0;

        /* storage returns to general RAM once both owner and device
         * references are gone; pinned runs stay until unpin */
        if(d->pid == 0 && d->pins == 0)
            dma_release_node(d);
        d = next;
    }
}

ewokos_addr_t dma_alloc(int32_t pid, uint32_t size) {
    size = ALIGN_UP(size, PAGE_SIZE);

    /* Draw a physically-contiguous run from general RAM inside the DMA
     * range, compacting via page_move when no free run exists. */
    ewokos_addr_t paddr = mm_alloc_contig(size, _dma.phy_base, _dma.size);
    if(paddr == 0)
        return 0;
    /* No cached alias may survive while DMA owns the run: flip the
     * direct map to the NOCACHE DMA attribute. */
    dma_pages_set_attr(paddr, size / PAGE_SIZE, PTE_ATTR_SYS_DMA);
    dma_t* n = dma_new(paddr, size);
    n->pid = pid;
    n->next = _dma.head;
    if(_dma.head != NULL)
        _dma.head->prev = n;
    _dma.head = n;
    return paddr;
}

ewokos_addr_t dma_phy_addr(ewokos_addr_t vaddr) {
    return vaddr - _dma.v_base + _dma.phy_base;
}

ewokos_addr_t dma_v_addr(ewokos_addr_t phy_addr) {
    return phy_addr - _dma.phy_base + _dma.v_base;
}