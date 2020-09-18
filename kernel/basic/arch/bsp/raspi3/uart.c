#include <dev/uart.h>
#include "bcm283x/mini_uart.h"

int32_t uart_dev_init(void) {
	return mini_uart_init();
}

int32_t uart_write(const void* data, uint32_t size) {
	return mini_uart_write(data, size);
}
