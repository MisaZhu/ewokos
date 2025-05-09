#ifndef DMA_H
#define DMA_H

#include <stdint.h>
#include <ewokos_config.h>

void          dma_init(void);
ewokos_addr_t dma_alloc(int32_t pid, uint32_t size);
void          dma_release(int32_t pid);

#endif