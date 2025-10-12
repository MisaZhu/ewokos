#include <stdlib.h>
#include <stdint.h>
#include <ewoksys/dma.h>

#define DEFAULT_DMA_SIZE (8 * 4096)
#define ALIGN_UP(x, a) (((x) + (a) - 1) & ~((a) - 1))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

static uint8_t* _dma_base = 0;
static uint8_t* _dma_phy = 0;
static size_t _dma_alloced = 0;
static size_t _dma_size;

int dma_user_init(size_t total){
    if(_dma_base)
        return 0;
    _dma_alloced = 0;
    _dma_size = ALIGN_UP(total, 4096);
    _dma_base = dma_alloc(0, _dma_size)&0xFFFFFFFF;
    _dma_phy = dma_phy_addr(0, _dma_base)&0xFFFFFFFF;
}

void* dma_user_phy(void* vaddr){
    return _dma_phy + ((uint8_t*)vaddr - _dma_base);
}

void* dma_user_alloc(size_t size){
    if(!_dma_base)
        dma_user_init(MAX(DEFAULT_DMA_SIZE, size + DEFAULT_DMA_SIZE));

    if(size > _dma_size){
        printf("dma_user_alloc: size %d > _dma_size %d\n", size, _dma_size);
        return -1;    
    }
    void* ret = _dma_base + _dma_alloced;
    _dma_alloced += ALIGN_UP(size, 16);
    _dma_size -= ALIGN_UP(size, 16);
    return ret;
}