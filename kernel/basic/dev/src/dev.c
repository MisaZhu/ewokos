#include <stddef.h>
#include <kprintf.h>
#include <dev/dev.h>
#include <dev/uart.h>

uint32_t _mmio_base = 0;

void dev_init(void) {
	uart_dev_init();
}

inline void enable_vmmio_base(void) {
	_mmio_base = MMIO_BASE;
}
