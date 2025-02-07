#ifndef DMA_H
#define DMA_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t dma_map(uint32_t size);
uint32_t dma_phy_addr(uint32_t vaddr);

#ifdef __cplusplus
}
#endif

#endif
