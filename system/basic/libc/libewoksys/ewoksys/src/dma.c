#include <ewoksys/dma.h>
#include <ewoksys/syscall.h>
#include <sysinfo.h>

#ifdef __cplusplus
extern "C" {
#endif

ewokos_addr_t dma_alloc(uint32_t size) {
    return syscall1(SYS_DMA_ALLOC, size);
}

void dma_free(ewokos_addr_t vaddr) {
    syscall1(SYS_DMA_FREE, vaddr);
}

ewokos_addr_t dma_phy_addr(ewokos_addr_t vaddr) {
    return syscall1(SYS_DMA_PHY_ADDR, vaddr);
}

#ifdef __cplusplus
}
#endif
