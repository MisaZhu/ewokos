#include <ewoksys/dma.h>
#include <ewoksys/syscall.h>
#include <sysinfo.h>

#ifdef __cplusplus
extern "C" {
#endif

ewokos_addr_t dma_alloc(int32_t dma_block_id, uint32_t size) {
	return syscall2(SYS_DMA_ALLOC, dma_block_id, size);
}

void dma_free(int32_t dma_block_id, ewokos_addr_t vaddr) {
	syscall2(SYS_DMA_FREE, dma_block_id, vaddr);
}

uint32_t dma_phy_addr(int32_t dma_block_id, ewokos_addr_t vaddr) {
	return syscall2(SYS_DMA_PHY_ADDR, dma_block_id, vaddr);
}

int32_t dma_set(ewokos_addr_t phy_base, uint32_t size, bool shared) {
	return syscall3(SYS_DMA_SET, phy_base, size, shared);
}

#ifdef __cplusplus
}
#endif
