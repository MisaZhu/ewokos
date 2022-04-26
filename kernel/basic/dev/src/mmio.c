#include <stddef.h>
#include <kprintf.h>
#include <dev/mmio.h>

uint32_t _mmio_base = 0;

inline void enable_vmmio_base(void) {
	_mmio_base = MMIO_BASE;
}
