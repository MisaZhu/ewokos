#include <dev/mmio.h>
#include <syscall.h>

inline int32_t mmio_get(uint32_t offset) {
	return syscall1(SYSCALL_MMIO_GET, (int32_t)offset);
}

inline int32_t mmio_put(uint32_t offset, int32_t val) {
	return syscall2(SYSCALL_MMIO_PUT, (int32_t)offset, val);
}

