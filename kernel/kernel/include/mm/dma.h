#ifndef DMA_H
#define DMA_H

#include <stdint.h>
#include <stdbool.h>
#include <ewokos_config.h>

void          dma_init(void);
ewokos_addr_t dma_alloc(int32_t dma_block_id, int32_t pid, uint32_t size);
void          dma_free(int32_t dma_block_id, int32_t pid, ewokos_addr_t phy_addr);
ewokos_addr_t dma_phy_addr(int32_t dma_block_id, ewokos_addr_t vaddr);
ewokos_addr_t dma_v_addr(int32_t dma_block_id, ewokos_addr_t phy_addr);
uint32_t      dma_size(int32_t dma_block_id, int32_t pid, ewokos_addr_t phy_addr);
int32_t       dma_set(int32_t pid, ewokos_addr_t phy_base, ewokos_addr_t v_base, uint32_t size, bool shared);
void          dma_release(int32_t pid);

#endif