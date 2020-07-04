#include <sys/mmio.h>
#include <sys/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif


uint32_t mmio_map(void) {
	return syscall0(SYS_MMIO_MAP);
}

#ifdef __cplusplus
}
#endif

