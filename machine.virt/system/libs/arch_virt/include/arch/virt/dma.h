#ifndef _DMA_H_
#define _DMA_H_
#include <stdint.h>
#include <stdlib.h>

void dma_user_init(void);
void* dma_user_alloc(size_t size);
void* dma_user_phy(void* vaddr);
void dma_user_free(void* vaddr);

#endif