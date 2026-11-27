#ifndef DMA_H
#define DMA_H

#include <stdint.h>
#include <stdbool.h>
#include <ewoksys/ewokdef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* cache=false (default): the buffer is mapped with the platform DMA
 * attribute - Non-Cacheable on non-coherent boards, so CPU writes land
 * straight in DRAM with no maintenance. cache=true grants a cacheable
 * (WRBACK) mapping for CPU-heavy use; the caller must then bracket device
 * access with sys_dma_sync() (clean before device reads, invalidate after
 * device writes). On coherent platforms (QEMU virt) both are cacheable. */
ewokos_addr_t dma_alloc(int32_t dma_block_id, uint32_t size, bool cache);
void     dma_free(int32_t dma_block_id, ewokos_addr_t vaddr);
ewokos_addr_t dma_phy_addr(int32_t dma_block_id, ewokos_addr_t vaddr);
int32_t  dma_set(ewokos_addr_t phy_base, uint32_t size, bool shared);

#ifdef __cplusplus
}
#endif

#endif
