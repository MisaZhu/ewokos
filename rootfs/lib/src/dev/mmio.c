#include <dev/mmio.h>
#include <syscall.h>

int32_t mmioGet(uint32_t offset) {
	return syscall1(SYSCALL_MMIO_GET, (int32_t)offset);
}

int32_t mmioPut(uint32_t offset, int32_t val) {
	return syscall2(SYSCALL_MMIO_PUT, (int32_t)offset, val);
}

