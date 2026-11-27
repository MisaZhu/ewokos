#ifndef DMA_H
#define DMA_H

#include <stdint.h>
#include <stdbool.h>
#include <ewokos_config.h>

void          dma_init(void);
/* DMA buffers are always mapped with the platform DMA attribute
 * PTE_ATTR_SYS_DMA (NOCACHE): CPU writes land straight in DRAM and no
 * cache maintenance is required around device access. */
ewokos_addr_t dma_alloc(int32_t pid, uint32_t size);
void          dma_free(int32_t pid, ewokos_addr_t phy_addr);
ewokos_addr_t dma_phy_addr(ewokos_addr_t vaddr);
ewokos_addr_t dma_v_addr(ewokos_addr_t phy_addr);
uint32_t      dma_size(int32_t pid, ewokos_addr_t phy_addr);
void          dma_release(int32_t pid);
/* Total bytes held by DMA allocations (live + device-pinned); kernel lock. */
ewokos_addr_t dma_alloced_size(void);

/* Whether page_move compaction may relocate a physical page: 0 when it lies
 * inside any DMA allocation node (live or device-pinned), 1 otherwise. Used
 * by the contiguous allocator's compaction path and by page_move itself. */
int32_t       dma_page_movable(ewokos_addr_t paddr);

/* Flip the kernel direct-map alias of a contiguous physical run between
 * WRBACK (general RAM) and a non-cacheable attribute, cleaning dirty lines
 * to DRAM before any WRBACK->non-cacheable transition and broadcasting a
 * global TLB flush afterwards. Shared by the DMA layer and the shm
 * backing; caller holds the kernel lock with the pages quiescent. */
void          dma_pages_set_attr(ewokos_addr_t paddr, uint32_t npages,
                                 uint32_t attr);

/* Device references to a live allocation (exact base, nonzero extent).
 * free/release revoke ownership immediately, but pinned physical storage is
 * excluded from allocation/coalescing until the final unpin. Kernel callers
 * must hold the same kernel lock as dma_alloc/free and must unpin only after
 * the device mapping/access has been reliably revoked. These are kernel-only
 * APIs; they do not restore CPU mappings or implement a device stop policy. */
int32_t       dma_pin(int32_t pid, ewokos_addr_t phy_addr, uint32_t size);
int32_t       dma_unpin(ewokos_addr_t phy_addr);
/* Read-only snapshot of an exact allocation while pinned; same lock.
 * owner=0 proves free/release has revoked CPU allocation ownership. */
int32_t       dma_pin_query(ewokos_addr_t phy_addr, int32_t *owner,
                            uint32_t *pins);

#endif
