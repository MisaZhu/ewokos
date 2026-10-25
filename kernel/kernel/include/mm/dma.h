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

/* Device references to a live allocation (exact base, nonzero extent).
 * free/release revoke ownership immediately, but pinned physical storage is
 * excluded from allocation/coalescing until the final unpin. Kernel callers
 * must hold the same kernel lock as dma_alloc/free and must unpin only after
 * the device mapping/access has been reliably revoked. These are kernel-only
 * APIs; they do not restore CPU mappings or implement a device stop policy. */
int32_t       dma_pin(int32_t dma_block_id, int32_t pid,
                      ewokos_addr_t phy_addr, uint32_t size);
int32_t       dma_unpin(int32_t dma_block_id, ewokos_addr_t phy_addr);
/* Read-only snapshot of an exact allocation while pinned; same lock.
 * owner=0 proves free/release has revoked CPU allocation ownership. */
int32_t       dma_pin_query(int32_t dma_block_id, ewokos_addr_t phy_addr,
                            int32_t *owner, uint32_t *pins);

/*
 * Cross-proc dma mapping tracking (see dma.c). sys_mem_map of a sys_dma
 * range records the peer mapping so the owner's dma_release can revoke it;
 * a peer drops it voluntarily via SYS_DMA_UNMAP or loses it at its own
 * proc_funeral.
 */
int32_t       dma_peer_map_by_paddr(int32_t peer_pid, ewokos_addr_t peer_vaddr, ewokos_addr_t paddr, uint32_t size);
void          dma_peer_map_revoke_owner(int32_t owner_pid);
void          dma_peer_map_forget_peer(int32_t peer_pid);
int32_t       dma_peer_unmap(int32_t peer_pid, ewokos_addr_t peer_vaddr);

#endif
