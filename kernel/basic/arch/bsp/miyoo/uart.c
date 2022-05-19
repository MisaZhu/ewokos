#include <dev/uart.h>

#ifdef PI2
#include <bcm283x/pl011_uart.h>

int32_t uart_dev_init(void) {
	return pl011_uart_init();
}

int32_t uart_write(const void* data, uint32_t size) {
	return pl011_uart_write(data, size);
}

#else
#include "bcm283x/mini_uart.h"

int32_t uart_dev_init(void) {
	return mini_uart_init();
}

int32_t uart_write(const void* data, uint32_t size) {
	return mini_uart_write(data, size);
}

#endif
