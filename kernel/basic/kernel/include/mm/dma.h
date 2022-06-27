#ifndef DMA_H
#define DMA_H

#include <stdint.h>

void     dma_init(void);
uint32_t dma_alloc(int32_t pid, uint32_t size);
void     dma_release(int32_t pid);

#endif