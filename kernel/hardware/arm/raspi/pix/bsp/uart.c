#include <dev/uart.h>
#include <bcm283x/pl011_uart.h>
#include <bcm283x/mini_uart.h>
#include <kernel/hw_info.h>
#include <kstring.h>
#include "uart_arch.h"

int32_t uart_dev_init(void) {
	if(_uart_type == UART_PL011)
		return pl011_uart_init();
	return mini_uart_init();
}

int32_t uart_write(const void* data, uint32_t size) {
	if(_uart_type == UART_PL011)
		return pl011_uart_write(data, size);
	return mini_uart_write(data, size);
}