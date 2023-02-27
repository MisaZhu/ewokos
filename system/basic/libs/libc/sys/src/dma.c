#include <sys/dma.h>
#include <sys/syscall.h>
#include <sysinfo.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t dma_map(uint32_t size) {
	return syscall3(SYS_MEM_MAP, 0, DMA_MAGIC, size);
}

#ifdef __cplusplus
}
#endif

