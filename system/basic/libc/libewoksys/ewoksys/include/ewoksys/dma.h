#ifndef DMA_H
#define DMA_H

#include <stdint.h>
#include <stdbool.h>
#include <ewoksys/ewokdef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* DMA buffers are always Non-Cacheable: CPU writes land straight in DRAM
 * and the device sees them without any cache maintenance. Allocations come
 * from the single dynamic sys_dma pool. */
ewokos_addr_t dma_alloc(uint32_t size);
void     dma_free(ewokos_addr_t vaddr);
ewokos_addr_t dma_phy_addr(ewokos_addr_t vaddr);


#ifdef __cplusplus
}
#endif

#endif
