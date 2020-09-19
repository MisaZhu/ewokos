#include <sys/mmio.h>
#include <sys/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t _mmio_base = 0;

uint32_t mmio_map(void) {
	_mmio_base = syscall0(SYS_MMIO_MAP);
	return _mmio_base;
}

#ifdef __cplusplus
}
#endif

