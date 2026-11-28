#include <mm/shm.h>
#include <mm/kalloc.h>
#include <mm/kmalloc.h>
#include <mm/dma.h>
#include <mm/mmu.h>
#include <mm/mmudef.h>
#include <kernel/kernel.h>
#include <kernel/hw_info.h>
#include <kernel/proc.h>
#include <kernel/system.h>
#include <kstring.h>
#include <kprintf.h>
#include <stddef.h>

#define 	IPC_PRIVATE 0
#define 	IPC_CREAT   00001000 /* create if key is nonexistent */
#define 	IPC_EXCL    00002000 /* fail if key exists */
/* Opt-in Non-Cacheable backing for device-backed segments (g2d canvas,
   display framebuffer, dma target). Such a segment is handed to hardware
   that does no cache maintenance by VA, so every alias (direct map, shm
   window, process) must be NOCACHE. Default (no flag) is cacheable WRBACK:
   most segments are CPU-only (piped rings, devcmd IO buffers, xinfo) and
   cached CPU memcpy through them is far faster. Mirror of ewoksys/ipc.h
   SHM_NOCACHE. */
#define 	SHM_NOCACHE 0x10000000
#define 	IPC_RMID    0 /* remove resource (mirror of sys/ipc.h) */
/* DMA pin hold: prevents the physical run from being freed while a
   device (e.g. GPI) may still read/write it.  pin must be balanced by an
   unpin from the same DMA owner before segment teardown. */
#define 	IPC_DMA_PIN   4
#define 	IPC_DMA_UNPIN 5

#ifdef KERNEL_SMP
extern void mcore_lock(int32_t* v);
extern void mcore_unlock(int32_t* v);
#endif

static ewokos_addr_t shmem_tail = 0;
static int32_t id_counter = 1;
static int32_t _shm_spin = 0;

typedef struct share_mem {
    int32_t id;
    int32_t key;
    ewokos_addr_t addr; //memory block base address
    uint32_t pages; //memory pages
    uint32_t used; //used or free
    int32_t flag; 
    int32_t owner_pid; //process id which alloced this shm
    int32_t refs;
    uint32_t pins;   // DMA-pinned while hardware may still reference it
    uint32_t nocache; // device-backed segment: NOCACHE aliases; 0 = cacheable CPU-only
    ewokos_addr_t phy_base; //contiguous run physical base, 0 while free
    struct share_mem* next;
    struct share_mem* prev;
} share_mem_t;

static share_mem_t* _shm_head = NULL;
static share_mem_t* _shm_tail = NULL;

/* Every shm segment is backed by a dynamically claimed physically-contiguous
   run of GENERAL RAM (mm_alloc_contig with page_move compaction, the same
   mechanism dma_alloc uses), so any segment can be handed to dma hardware
   through shm_phy_addr(). There is no reserved carve-out: while no segment
   owns them the pages stay in the page allocator and any process can reuse
   them. Total live shm is bounded by the SHM_MAX_SIZE virtual window.

   Runs are drawn from [shm_phy_base, shm_phy_base+shm_phy_size), fixed at
   shm_init: the arch_dma_range() hardware limit (every segment may be a
   GPU/accelerator buffer, so its pages MUST be device-reachable), raised
   above 4GB when the BSP opts in via arch_relocate_dma_high() and RAM is
   well above it - that keeps the low 4GB free for one contiguous user heap.
   always called with the shm lock held. */
static ewokos_addr_t shm_phy_base = 0;
static ewokos_addr_t shm_phy_size = 0;

/* Claim a contiguous physical run for a segment. The run is zeroed through
   the cacheable kernel direct map (P2V - the shm window VA is unmapped in
   kernel/syscall context). A device-backed (nocache) segment is then flipped
   to NOCACHE: dma_pages_set_attr cleans the zeroing to DRAM before the flip,
   so no cached alias survives while a device may read/write it - the same
   enter/leave protocol dma_alloc/dma_release use. A default cacheable
   (CPU-only) segment keeps the WRBACK direct map: its window/process aliases
   are WRBACK too, so the zeroing stays coherent through the cache with no
   flip/flush. Returns the physical base, or 0 when no contiguous run can be
   obtained. */
static ewokos_addr_t shm_phys_alloc(uint32_t pages, uint32_t nocache) {
    if(shm_phy_size == 0)
        return 0;
    ewokos_addr_t paddr = mm_alloc_contig(pages * PAGE_SIZE,
            shm_phy_base, shm_phy_size);
    if(paddr == 0)
        return 0;
    memset((void*)P2V(paddr), 0, (ewokos_addr_t)pages * PAGE_SIZE);
    if(nocache)
        dma_pages_set_attr(paddr, pages, PTE_ATTR_NOCACHE);
    return paddr;
}

/* Return a run to general RAM. A device-backed segment restores the WRBACK
   direct-map alias (dma_pages_set_attr ends with a global flush_tlb that also
   publishes the window unmap); a cacheable segment is already WRBACK, so only
   the TLB flush is needed. The caller must have unmapped every window/process
   PTE of the run first (a stale NOCACHE alias of a recycled WRBACK page is a
   mismatched-attribute alias). */
static void shm_phys_free(ewokos_addr_t paddr, uint32_t pages, uint32_t nocache) {
    if(paddr == 0)
        return;
    if(nocache)
        dma_pages_set_attr(paddr, pages, PTE_ATTR_WRBACK);
    else
        flush_tlb();
    kfree_contig((void*)P2V(paddr), pages);
}

/* Whether page_move compaction may relocate a physical page: pages owned by
   a live segment are tracked by physical base here (devices hold the
   physical address), which a PTE retarget alone cannot update. Free blocks
   have already returned their runs, so only used segments are checked. */
int32_t shm_page_movable(ewokos_addr_t paddr) {
    for(share_mem_t* it = _shm_head; it != NULL; it = it->next) {
        if(it->used && it->phy_base != 0 &&
                paddr >= it->phy_base &&
                paddr < it->phy_base + (ewokos_addr_t)it->pages * PAGE_SIZE)
            return 0;
    }
    return 1;
}

static inline void shm_lock(void) {
#ifdef KERNEL_SMP
    mcore_lock(&_shm_spin);
#endif
}

static inline void shm_unlock(void) {
#ifdef KERNEL_SMP
    mcore_unlock(&_shm_spin);
#endif
}

void shm_init() {
    //share memory base address at virtual address 1GB
    shmem_tail = ALIGN_UP(SHM_BASE, PAGE_SIZE);
    _shm_head = NULL;
    _shm_tail = NULL;
    id_counter = 1;

    /* Fix the physical allocation range (runs after sys_info_config, so
       arch_dma_range sees the final allocable layout). The hardware limit
       is a hard constraint - GPU/accelerators must reach every shm page.
       arch_relocate_dma_high() only picks placement INSIDE that limit:
       with 64-bit-clean masters and RAM well above 4GB, draw runs from
       above 4GB so the low 4GB stays one contiguous user heap. The >6GB
       threshold guarantees the high span is at least 2GB - far more than
       the SHM_MAX_SIZE window can ever hold. */
    ewokos_addr_t base, top;
    arch_dma_range(&base, &top);
    base = ALIGN_UP(base, PAGE_SIZE);
    top = ALIGN_DOWN(top, PAGE_SIZE);
    if(arch_relocate_dma_high() && top > (6ull*GB)) {
        ewokos_addr_t hi = ALIGN_UP((ewokos_addr_t)(4ull*GB), PAGE_SIZE);
        if(hi > base)
            base = hi;
    }
    shm_phy_base = base;
    shm_phy_size = (top > base) ? (top - base) : 0;
}

static share_mem_t* shm_new(void) {
    share_mem_t* ret = (share_mem_t*)kmalloc(sizeof(share_mem_t));
    if(ret == NULL)
        return NULL;

    memset(ret, 0, sizeof(share_mem_t));
    ret->owner_pid = -1;
    return ret;
}

/* map a run of already claimed contiguous physical pages into the shm
   window. The run was zeroed through the kernel direct map (P2V); a
   device-backed (nocache) segment had its direct-map alias flipped to
   NOCACHE by shm_phys_alloc and is mapped NOCACHE here too (shared with
   non-coherent hardware - g2d/V3D, dma engines - every alias must agree, see
   shm_proc_map). A default cacheable CPU-only segment keeps WRBACK on every
   alias. */
static int32_t shm_map_pages(ewokos_addr_t addr, ewokos_addr_t paddr, uint32_t pages, uint32_t nocache) {
    uint32_t attr = nocache ? PTE_ATTR_NOCACHE : PTE_ATTR_WRBACK;
    uint32_t i;
    for (i = 0; i < pages; i++) {
        map_page(_kernel_info.kernel_vm,
                addr,
                paddr,
                AP_RW_D, attr);
        flush_tlb_addr(addr);
        addr += PAGE_SIZE;
        paddr += PAGE_SIZE;
    }
    return 1;
}

static int32_t shm_alloc(int32_t key, uint32_t size, int32_t flag) {
    uint32_t nocache = (flag & SHM_NOCACHE) != 0;
    size = ALIGN_UP(size, 32);
    ewokos_addr_t addr = shmem_tail;
    uint32_t pages = (size / PAGE_SIZE);
    if((size % PAGE_SIZE) != 0)
        pages++;

    /* grab the contiguous physical run up front; strict failure when no
       contiguous run can be obtained (never fall back to scattered pages,
       hardware would silently get a non-contiguous buffer) */
    ewokos_addr_t paddr = shm_phys_alloc(pages, nocache);
    if(paddr == 0)
        return -1;

    share_mem_t* i = _shm_head;
    while(i != NULL) { //search for available memory block
        if(!i->used && i->pages >= pages)
            break;
        i = i->next;
    }

    share_mem_t* tmp = NULL;
    if(i != NULL) { //avaible item found.
        addr =  i->addr;
        if(i->pages > pages) { //try split one item to two;
            tmp = shm_new();
            if(tmp != NULL) {
                tmp->pages = i->pages -  pages;
                tmp->addr = i->addr + (pages * PAGE_SIZE);
                i->pages = pages;
                tmp->next = i->next;
                tmp->prev = i;
                if(i->next != NULL)
                    i->next->prev = tmp;
                i->next = tmp;
                if(i == _shm_tail)
                    _shm_tail = tmp;
            }
        }
    }
    else { // not found, need to expand pages for new block.
        if((addr + size) >= (SHM_BASE + SHM_MAX_SIZE)) {
            shm_phys_free(paddr, pages, nocache);
            return -1;
        }

        i = shm_new();
        if(i == NULL) {
            shm_phys_free(paddr, pages, nocache);
            return -1;
        }
        i->addr = addr;
        if(_shm_head == NULL) {
            _shm_head = _shm_tail = i;
        }
        else  {
            _shm_tail->next = i;
            i->prev = _shm_tail;
            _shm_tail = i;
        }
    }		

    /* map pages for a fresh OR reused block: free_item returned the physical
       run to the page allocator and dropped the window PTEs, so a reused
       block has no backing left and must always be remapped onto paddr -
       mapping is mandatory or the segment has no backing at all. */
    if(shm_map_pages(addr, paddr, pages, nocache) == 0) {
        shm_phys_free(paddr, pages, nocache);
        return -1;
    }
    i->pages = pages;

    if(addr == shmem_tail)
        shmem_tail += pages * PAGE_SIZE;

    i->used = 1;
    i->key = key;
    i->id = id_counter++;
    i->phy_base = paddr;
    proc_t* current_proc = get_current_proc();
    i->owner_pid = current_proc ? current_proc->info.pid : -1;
    i->flag = flag & 0666;
    i->nocache = nocache;

    return i->id;
}

static share_mem_t* shm_item_by_key(int32_t key) { //get shm item by key.
    share_mem_t* i = _shm_head;
    while(i != NULL) {
        if(i->used && i->key == key)
            return i;
        i = i->next;
    }
    return NULL;
}

int32_t shm_get(int32_t key, uint32_t size, int32_t flag) {
    shm_lock();
    if(key != IPC_PRIVATE) {
        share_mem_t* it = shm_item_by_key(key);
        if(it != NULL) {
            if((flag & IPC_EXCL) != 0) {
                shm_unlock();
                return -1;
            }
            shm_unlock();
            return it->id;
        }
        else if((flag & IPC_CREAT) == 0) {
            shm_unlock();
            return -1;
        }
    }
    else {
        flag = 0666 | (flag & SHM_NOCACHE);
    }

    int32_t ret = shm_alloc(key, size, (flag & 0666) | (flag & SHM_NOCACHE));
    shm_unlock();
    return ret;
}

static share_mem_t* shm_item_by_id(int32_t id) { //get shm item by id.
    share_mem_t* i = _shm_head;
    while(i != NULL) {
        if(i->used && i->id == id)
            return i;
        i = i->next;
    }
    return NULL;
}

static share_mem_t* shm_item_by_addr(void* addr) { //get shm item by addr.
    share_mem_t* i = _shm_head;
    while(i != NULL) {
        if(i->used && i->addr == addr) 
            return i;
        i = i->next;
    }
    return NULL;
}

uint32_t shm_alloced_size(void) {
    uint32_t ret = 0;
    shm_lock();
    share_mem_t* i = _shm_head;
    while(i != NULL) {
        if(i->used) {
            ret += i->pages * PAGE_SIZE;
        }
        i = i->next;
    }
    shm_unlock();
    return ret;
}

static share_mem_t* free_item(share_mem_t* it) {
    if(it->phy_base != 0) {
        /* drop the window PTEs BEFORE the pages recycle: they are NOCACHE and
           would otherwise alias a page another owner now maps WRBACK. Then
           hand the physical run back to general RAM (the flip inside
           shm_phys_free ends with a global flush_tlb, publishing the unmap).
           The window block itself stays free and is still merged with
           neighbours below; reuse always remaps the window onto a fresh run. */
        unmap_pages(_kernel_info.kernel_vm, it->addr, it->pages);
        shm_phys_free(it->phy_base, it->pages, it->nocache);
        it->phy_base = 0;
    }
    it->used = 0;
    if(it->next != NULL && !it->next->used) { //merge right free items
        share_mem_t* p = it->next;
        it->next = p->next;
        if(p->next != NULL)
            p->next->prev = it;
        else //tail
            _shm_tail = it;
        it->pages += p->pages;
        kfree(p);
    }

    if(it->prev != NULL && !it->prev->used) { //merge left free items
        share_mem_t* p = it->prev;
        p->next = it->next;
        if(it->next != NULL)
            it->next->prev = p;
        else
            _shm_tail = p;
        p->pages += it->pages;
        kfree(it);
        it = p;
    }
    return it->next;
}

#define SHM_R 0x1
#define SHM_W 0x2
#define SHM_N 0x0

static uint32_t check_access(proc_t* proc, share_mem_t* it) {
    if(proc->info.uid == 0)
        return (SHM_R | SHM_W);
    
    if(it->owner_pid < 0)
        return (SHM_R | SHM_W); // Allow access if no owner
    
    proc_t* owner = proc_get(it->owner_pid);
    owner = proc_get_proc(owner);
    if(owner == NULL)
        return (SHM_R | SHM_W); // Allow access if owner not found
    
    if(owner == proc)
        return (SHM_R | SHM_W);

    if(it->key == IPC_PRIVATE) { //family only
        if(proc_childof(proc, owner) == 0)
                return (SHM_R | SHM_W); //passed
        return SHM_N;
    }

    int32_t a = 0;
    if(owner->info.uid == proc->info.uid)
        a = (it->flag >> 6) & 0x7;
    else if(owner->info.gid == proc->info.gid)
        a = (it->flag >> 3) & 0x7;
    else
        a = it->flag & 0x7;

    uint32_t res = SHM_N;
    if((a & 0x4) != 0)
        res |= SHM_R;
    if((a & 0x2) != 0)
        res |= SHM_W;
    return res;
}
    
/*map share memory to process*/
void* shm_proc_map(proc_t* proc, int32_t id) {
    shm_lock();
    share_mem_t* it = shm_item_by_id(id);
    if(it == NULL || proc == NULL) {
        shm_unlock();
        return NULL;
    }

    uint32_t access = check_access(proc, it);
    //uint32_t access = SHM_W | SHM_R;
    if(access == SHM_N) {
        shm_unlock();
        return NULL;
    }

    uint32_t i;
    //check if mapped , keep it and return
    for (i = 0; i < SHM_MAX; i++) {
        if(proc->space->shms[i] == id) {
            shm_unlock();
            return (void*)it->addr;
        }
    }

    //do real map
    for (i = 0; i < SHM_MAX; i++) {
        if(proc->space->shms[i] == 0) {
            proc->space->shms[i] = id;
            break;
        }
    }
    if(i >= SHM_MAX) {
        shm_unlock();
        return NULL;
    }

    if((access & SHM_W) != 0)
        access = AP_RW_RW;
    else
        access = AP_RW_R;

    /*
     * A device-backed (SHM_NOCACHE) shm segment is the CPU<->accelerator
     * exchange buffer (g2d canvas, display framebuffer, dma target). The
     * hardware drivers rely on it being Non-Cacheable in every process and do
     * no cache maintenance by VA, so the process view MUST be NOCACHE too -
     * every alias must agree. A default cacheable segment is CPU-only (piped
     * ring, devcmd IO buffer, xinfo) and is mapped WRBACK so CPU memcpy
     * through it stays cached; it must never be handed to a device.
     */
    uint32_t attr = it->nocache ? PTE_ATTR_NOCACHE : PTE_ATTR_WRBACK;
    ewokos_addr_t addr = it->addr;
    for (i = 0; i < it->pages; i++) {
        ewokos_addr_t physical_addr = resolve_phy_address(_kernel_info.kernel_vm, addr);
        if(map_page(proc->space->vm,
                addr,
                physical_addr,
                access, attr) != 0) {
            /* Do not publish a partial attachment on page-table OOM. */
            uint32_t mapped = i;
            for(uint32_t j = 0; j < mapped; j++)
                unmap_page(proc->space->vm, it->addr + j * PAGE_SIZE);
            flush_tlb_range(it->addr, mapped);
            for(uint32_t j = 0; j < SHM_MAX; j++)
                if(proc->space->shms[j] == id) {
                    proc->space->shms[j] = 0;
                    break;
                }
            shm_unlock();
            return NULL;
        }
        addr += PAGE_SIZE;
    }
    flush_tlb_range(it->addr, it->pages);
    proc->info.shm_size += it->pages * PAGE_SIZE;
    it->refs++;
    shm_unlock();
    return (void*)it->addr;
}

/*unmap share memory of process*/
static int32_t shm_proc_unmap_it(proc_t* proc, share_mem_t* it, bool free_it) {
    uint32_t i;
    for (i = 0; i < SHM_MAX; i++) {
        if(proc->space->shms[i] == it->id) {
            proc->space->shms[i] = 0;
            break;
        }
    }
    if(i >= SHM_MAX)
        return -1;

    ewokos_addr_t addr = it->addr;
    for (i = 0; i < it->pages; i++) {
        unmap_page(proc->space->vm, addr);
        addr += PAGE_SIZE;
    }
    /* Must complete on other cores before refs can drop/free_item can recycle
     * the physical pages. The SHM lock serializes this entire transaction. */
    flush_tlb_range(it->addr, it->pages);
    if(proc->info.shm_size > (it->pages*PAGE_SIZE))
        proc->info.shm_size -= it->pages * PAGE_SIZE;
    else
        proc->info.shm_size = 0;

    it->refs--;
    if(free_it && it->refs <= 0 && it->pins == 0) {
        free_item(it);
    }
    return 0;
}

void*   shm_map(proc_t* proc, int32_t key, uint32_t size, int32_t flag, int32_t* id) {
    *id = -1;
    int32_t sid = shm_get(key, size, flag);
    if(sid <= 0)
        return NULL;
    void* ret = shm_proc_map(proc, sid);
    if(ret == NULL)
        return NULL;
    *id = sid;
    return ret;
}

int32_t shm_proc_unmap_by_id(proc_t* proc, uint32_t id, bool free_it) {
    shm_lock();
    share_mem_t* it = shm_item_by_id(id);
    if(it == NULL) {
        shm_unlock();
        return -1;
    }
    int32_t ret = shm_proc_unmap_it(proc, it, free_it);
    shm_unlock();
    return ret;
}

/*
 * Reap shm segments owned by owner_pid that no proc has attached
 * (refs <= 0). proc_unmap_shms walks only the ATTACHED set
 * (proc->space->shms[]), so a segment created via shm_get(IPC_CREAT)
 * but never shmat'd would otherwise linger forever, holding a VA
 * window slot and a live contiguous run.
 *
 * Called from proc_funeral for TASK_TYPE_PROC right after
 * proc_unmap_shms, so segments whose refs dropped to zero during
 * the attached sweep have already been freed by shm_proc_unmap_it and
 * are not double-processed here.
 *
 * Safe with surviving threads of a dying owner: refs == 0 means no
 * thread has the segment in its shms[] either, so nothing to unmap
 * from any vm. Kernel-registered / ownerless segments (owner_pid < 0)
 * are never touched.
 */
void shm_release_orphans(int32_t owner_pid) {
    if(owner_pid < 0)
        return;
    shm_lock();
    share_mem_t* it = _shm_head;
    while(it != NULL) {
        if(it->used && it->owner_pid == owner_pid && it->refs <= 0 &&
                it->pins == 0) {
            /*
             * free_item may merge the freed block with a free prev or
             * next neighbour; its return value is always the next safe
             * traversal pointer after any merge it performed.
             */
            it = free_item(it);
        }
        else {
            it = it->next;
        }
    }
    shm_unlock();
}

/* shmctl() back end.
   IPC_RMID: destroy the segment only when no process has it attached
   (refs == 0). An unattached segment is invisible to every process, so
   destroying it cannot affect anyone; a still-attached one is left
   untouched and the call fails. Lets a creator back out cleanly when a
   follow-up step (e.g. its own shmat) fails right after shmget. */
int32_t shm_ctrl(int32_t id, int32_t cmd) {
    shm_lock();
    share_mem_t* it = shm_item_by_id(id);
    if(it == NULL) {
        shm_unlock();
        return -1;
    }
    int32_t ret = -1;
    if(cmd == IPC_RMID) {
        if(it->refs <= 0 && it->pins == 0) {
            free_item(it);
            ret = 0;
        }
    }
    else if(cmd == IPC_DMA_PIN) {
        if(it->phy_base != 0 && it->pins != 0xffffffffu) {
            it->pins++;
            ret = 0;
        }
    }
    else if(cmd == IPC_DMA_UNPIN) {
        if(it->pins > 0) {
            it->pins--;
            ret = 0;
        }
    }
    shm_unlock();
    return ret;
}

int32_t shm_set_owner(uint32_t id, int32_t pid) {
    shm_lock();
    share_mem_t* it = shm_item_by_id(id);
    if(it == NULL || !it->used) {
        shm_unlock();
        return -1;
    }
    it->owner_pid = pid;	
    shm_unlock();
    return 0;
}

/* resolve the physical address of vaddr inside a shm segment (for dma
   hardware). every segment is backed by one contiguous physical run, so
   the translation is a simple offset from the run base. */
ewokos_addr_t shm_phy_addr(int32_t id, ewokos_addr_t vaddr) {
    shm_lock();
    share_mem_t* it = shm_item_by_id(id);
    if(it == NULL || it->phy_base == 0) {
        shm_unlock();
        return 0;
    }
    if(vaddr < it->addr || vaddr >= (it->addr + it->pages * PAGE_SIZE)) {
        shm_unlock();
        return 0;
    }
    ewokos_addr_t ret = it->phy_base + (vaddr - it->addr);
    shm_unlock();
    return ret;
}

/*unmap share memory of process*/
int32_t shm_proc_unmap(proc_t* proc, void* p) {
    shm_lock();
    share_mem_t* it = shm_item_by_addr(p);
    if(it == NULL || proc == NULL) {
        shm_unlock();
        return -1;
    }
    int32_t ret = shm_proc_unmap_it(proc, it, true);
    shm_unlock();
    return ret;
}