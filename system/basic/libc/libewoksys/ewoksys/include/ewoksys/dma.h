#ifndef DMA_H
#define DMA_H

#include <stdint.h>
#include <stdbool.h>
#include <ewoksys/ewokdef.h>

#ifdef __cplusplus
extern "C" {
#endif

ewokos_addr_t dma_alloc(int32_t dma_block_id, uint32_t size);
void     dma_free(int32_t dma_block_id, ewokos_addr_t vaddr);
uint32_t dma_phy_addr(int32_t dma_block_id, ewokos_addr_t vaddr);
int32_t  dma_set(ewokos_addr_t phy_base, uint32_t size, bool shared);

#ifdef __cplusplus
}
#endif

#endif
