#include <mm/shm.h>
#include <mm/kalloc.h>
#include <mm/kmalloc.h>
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
/* ewokos-specific (mirror of sys/ipc.h): back the new segment from the
   reserved contiguous slab (_sys_info.shm_contig) instead of scattered
   kalloc pages, so the physical memory is contiguous and usable by dma
   hardware. only affects creation; fails (never silently falls back) when
   the slab is unconfigured or full */
#define 	IPC_CONTIG  0x01000000

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
    uint8_t contig; //backed by the contiguous slab instead of kalloc pages
    ewokos_addr_t phy_base; //slab physical base, valid when contig
    struct share_mem* next;
    struct share_mem* prev;
} share_mem_t;

static share_mem_t* _shm_head = NULL;
static share_mem_t* _shm_tail = NULL;

/* contiguous slab sub-allocator: first-fit with split and merge, managing
   physical page runs inside _sys_info.shm_contig. entries are never
   compacted (released ones turn into empty slots, paddr == 0), so no
   overlapping copies are needed. always called with the shm lock held. */
#define SHM_POOL_MAX 64
typedef struct {
    ewokos_addr_t paddr; //0 means empty slot
    uint32_t pages;
    uint8_t used;
} shm_pool_item_t;

static shm_pool_item_t _shm_pool[SHM_POOL_MAX];
static uint32_t _shm_pool_count = 0;

static ewokos_addr_t shm_pool_alloc(uint32_t pages) {
    for(uint32_t i = 0; i < _shm_pool_count; i++) {
        shm_pool_item_t* it = &_shm_pool[i];
        if(it->paddr == 0 || it->used || it->pages < pages)
            continue;
        if(it->pages > pages) { //split the remainder into a free entry
            int32_t slot = -1;
            for(uint32_t j = 0; j < _shm_pool_count; j++) {
                if(_shm_pool[j].paddr == 0) {
                    slot = (int32_t)j;
                    break;
                }
            }
            if(slot < 0) {
                if(_shm_pool_count >= SHM_POOL_MAX)
                    return 0;
                slot = (int32_t)_shm_pool_count++;
            }
            _shm_pool[slot].paddr = it->paddr + pages * PAGE_SIZE;
            _shm_pool[slot].pages = it->pages - pages;
            _shm_pool[slot].used = 0;
            it->pages = pages;
        }
        it->used = 1;
        return it->paddr;
    }
    return 0;
}

static void shm_pool_free(ewokos_addr_t paddr) {
    for(uint32_t i = 0; i < _shm_pool_count; i++) {
        shm_pool_item_t* it = &_shm_pool[i];
        if(it->paddr != paddr || !it->used)
            continue;
        it->used = 0;
        //merge with physically adjacent free neighbours
        for(uint32_t j = 0; j < _shm_pool_count; j++) {
            shm_pool_item_t* nb = &_shm_pool[j];
            if(j == i || nb->paddr == 0 || nb->used)
                continue;
            if(it->paddr + it->pages * PAGE_SIZE == nb->paddr) { //nb is right
                it->pages += nb->pages;
                nb->paddr = 0;
            }
            else if(nb->paddr + nb->pages * PAGE_SIZE == it->paddr) { //nb is left
                it->paddr = nb->paddr;
                it->pages += nb->pages;
                nb->paddr = 0;
            }
        }
        return;
    }
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

    //seed the contiguous slab pool when kernel.conf reserved one
    _shm_pool_count = 0;
    if(_sys_info.shm_contig.size > 0) {
        _shm_pool[0].paddr = _sys_info.shm_contig.phy_base;
        _shm_pool[0].pages = _sys_info.shm_contig.size / PAGE_SIZE;
        _shm_pool[0].used = 0;
        _shm_pool_count = 1;
    }
}

static share_mem_t* shm_new(void) {
    share_mem_t* ret = (share_mem_t*)kmalloc(sizeof(share_mem_t));
    if(ret == NULL)
        return NULL;

    memset(ret, 0, sizeof(share_mem_t));
    ret->owner_pid = -1;
    return ret;
}

static void shm_unmap_pages(ewokos_addr_t addr, uint32_t pages) {
    uint32_t i;
    for (i = 0; i < pages; i++) {
        ewokos_addr_t physical_addr = resolve_phy_address(_kernel_info.kernel_vm, addr);

        //get the kernel address for kalloc/kfree
        ewokos_addr_t kernel_addr = P2V(physical_addr);
        kfree_page((void *) kernel_addr);
        unmap_page(_kernel_info.kernel_vm, addr);
        flush_tlb_addr(addr); /* scope invalidation to just this page */
        addr += PAGE_SIZE;
    }
}

static int32_t shm_map_pages(ewokos_addr_t addr, uint32_t pages) {
    ewokos_addr_t old_addr = addr;
    uint32_t i;
    for (i = 0; i < pages; i++) {
        char *page = kalloc_page();
        if(page == NULL) {
            printf("shm_map: kalloc failed!\n", (uint32_t)page);
            shm_unmap_pages(old_addr, i);
            return 0;
        }
        memset(page, 0, PAGE_SIZE);

        map_page(_kernel_info.kernel_vm,
                addr,
                V2P(page),
                AP_RW_D, PTE_ATTR_NOCACHE);
        flush_tlb_addr(addr); /* scope invalidation to just this page */
        addr += PAGE_SIZE;
    }
    return 1;
}

/* map a run of already reserved contiguous physical pages into the shm
   window. zeroing must go through the kernel direct map (P2V, set up in
   map_allocable_pages), NOT the window VA: the window sits in the private
   user half of the address space and is unmapped in kernel/syscall context */
static int32_t shm_map_pages_contig(ewokos_addr_t addr, ewokos_addr_t paddr, uint32_t pages) {
    uint32_t i;
    memset((void*)P2V(paddr), 0, pages * PAGE_SIZE);
    for (i = 0; i < pages; i++) {
        map_page(_kernel_info.kernel_vm,
                addr,
                paddr,
                AP_RW_D, PTE_ATTR_NOCACHE);
        flush_tlb_addr(addr);
        addr += PAGE_SIZE;
        paddr += PAGE_SIZE;
    }
    return 1;
}

static int32_t shm_alloc(int32_t key, uint32_t size, int32_t flag, uint8_t contig) {
    size = ALIGN_UP(size, 32);
    ewokos_addr_t addr = shmem_tail;
    uint32_t pages = (size / PAGE_SIZE);
    if((size % PAGE_SIZE) != 0)
        pages++;

    /* grab the contiguous physical run up front; strict failure when the
       slab is unconfigured or exhausted (never fall back to scattered
       pages, hardware would silently get a non-contiguous buffer) */
    ewokos_addr_t pool_paddr = 0;
    if(contig) {
        pool_paddr = shm_pool_alloc(pages);
        if(pool_paddr == 0) {
            return -1;
        }
    }

    
    share_mem_t* i = _shm_head;
    while(i != NULL) { //search for available memory block
        /* free blocks keep their backing type: a slab run cannot back a
           scattered request and vice versa */
        if(!i->used && i->pages >= pages && i->contig == contig)
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
                tmp->contig = i->contig; //free remainder keeps the backing type
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
        if((shmem_tail + size) >= (SHM_BASE + SHM_MAX_SIZE)) {
            if(contig)
                shm_pool_free(pool_paddr);
            return -1;
        }

        i = shm_new();
        if(i == NULL) {
            if(contig)
                shm_pool_free(pool_paddr);
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

    /* map pages for a fresh block; a REUSED free contig block must also be
       remapped: free_item returned its physical run to the slab pool, so the
       window PTEs still point at a run somebody else may own now. Retargeting
       them at pool_paddr is mandatory or two segments alias the same memory
       (scattered blocks are different: their kalloc pages stay with the block) */
    if(i->pages == 0 || contig) {
        int32_t ok;
        if(contig)
            ok = shm_map_pages_contig(addr, pool_paddr, pages);
        else
            ok = shm_map_pages(addr, pages);
        if(!ok) {
            if(contig)
                shm_pool_free(pool_paddr);
            return -1;
        }
        i->pages = pages;
    }	

    if(addr == shmem_tail)
        shmem_tail += pages * PAGE_SIZE;

    i->used = 1;
    i->key = key;
    i->id = id_counter++;
    i->contig = contig;
    i->phy_base = pool_paddr;
    proc_t* current_proc = get_current_proc();
    i->owner_pid = current_proc ? current_proc->info.pid : -1;
    i->flag = flag;

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
        flag = 0666;
    }

    int32_t ret = shm_alloc(key, size, (flag & 0666), (flag & IPC_CONTIG) != 0);
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
    if(it->contig) {
        /* hand the physical run back to the slab pool right away; the window
           block itself stays free and is still merged with neighbours below
           (the merge is contig-type guarded, and reuse always remaps the
           window PTEs onto a fresh pool run, so stale PTEs are harmless) */
        shm_pool_free(it->phy_base);
    }
    //shm_unmap_pages(it->addr, it->pages);
    it->used = 0;
    if(it->next != NULL && !it->next->used && it->next->contig == it->contig) { //merge right free items
        share_mem_t* p = it->next;
        it->next = p->next;
        if(p->next != NULL)
            p->next->prev = it;
        else //tail
            _shm_tail = it;
        it->pages += p->pages;
        kfree(p);
    }

    if(it->prev != NULL && !it->prev->used && it->prev->contig == it->contig) { //merge left free items
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

    ewokos_addr_t addr = it->addr;
    for (i = 0; i < it->pages; i++) {
        ewokos_addr_t physical_addr = resolve_phy_address(_kernel_info.kernel_vm, addr);
        map_page(proc->space->vm,
                addr,
                physical_addr,
                access, PTE_ATTR_WRBACK);
        flush_tlb_addr(addr); /* scope invalidation to just this page */
        addr += PAGE_SIZE;
    }
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
        flush_tlb_addr(addr); /* scope invalidation to just this page */
        addr += PAGE_SIZE;
    }
    if(proc->info.shm_size > (it->pages*PAGE_SIZE))
        proc->info.shm_size -= it->pages * PAGE_SIZE;
    else
        proc->info.shm_size = 0;

    it->refs--;
    if(free_it && it->refs <= 0) {
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

/* resolve the physical address of vaddr inside a contig-backed shm segment
   (for dma hardware). fails (returns 0) for scattered kalloc-backed
   segments: their pages are not physically contiguous, and returning an
   address would let hardware walk into unrelated memory */
ewokos_addr_t shm_contig_phy_addr(int32_t id, ewokos_addr_t vaddr) {
    shm_lock();
    share_mem_t* it = shm_item_by_id(id);
    if(it == NULL || !it->contig || it->phy_base == 0) {
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
