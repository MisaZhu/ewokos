#include <mm/kmalloc.h>
#include <mm/dma.h>
#include <kernel/hw_info.h>
#include <kernel/proc.h>
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
    for(uint32_t i=0; i<_dma_block_count; i++) {
        dma_t* d = _dma_blocks[i].head;
        while(d != NULL) {
            dma_t* next = d->next;
            if(d->pid == pid)
                d->pid = 0;

            if(d->prev != NULL && d->prev->pid == 0) { //merge
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
    if(_dma_block_count >= DMA_BLOCK_MAX)
        return -1;

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