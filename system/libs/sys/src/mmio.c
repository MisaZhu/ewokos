#include <sys/mmio.h>
#include <sys/syscall.h>

uint32_t mmio_map(void) {
	return syscall0(SYS_MMIO_MAP);
}
