#include <ewoksys/dma.h>
#include <ewoksys/syscall.h>
#include <sysinfo.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t dma_map(uint32_t size) {
	return syscall1(SYS_DMA_ALLOC, size);
}

uint32_t dma_phy_addr(uint32_t vaddr) {
	return syscall1(SYS_DMA_PHY_ADDR, vaddr);
}

#ifdef __cplusplus
}
#endif

