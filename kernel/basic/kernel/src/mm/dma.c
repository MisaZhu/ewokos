#include <mm/kmalloc.h>
#include <mm/dma.h>
#include <kernel/hw_info.h>
#include <stddef.h>

typedef struct st_dma {
    int32_t  pid;
    uint32_t size;
    uint32_t base;
    struct st_dma* next;
    struct st_dma* prev;
} dma_t;

static dma_t* _dma_head = NULL;

static dma_t* dma_new(uint32_t base, uint32_t size) {
    dma_t* ret = (dma_t*)kcalloc(1, sizeof(dma_t));
    ret->size = size;
    ret->base = base;
    return ret;
}

void dma_init(void) {
    _dma_head = dma_new(_sys_info.dma.phy_base, _sys_info.dma.size);
}

void dma_release(int32_t pid) {
    dma_t* d = _dma_head;
    while(d != NULL) {
        if(d->pid == pid)
            d->pid = 0;
        d = d->next;
    }
}

uint32_t dma_alloc(int32_t pid, uint32_t size) {
    size = ALIGN_UP(size, PAGE_SIZE);
    dma_t* d = _dma_head;
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