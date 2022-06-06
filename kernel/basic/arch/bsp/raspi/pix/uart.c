#include <dev/uart.h>
#include <bcm283x/pl011_uart.h>
#include <bcm283x/mini_uart.h>
#include <kernel/hw_info.h>
#include <kstring.h>

int32_t uart_dev_init(void) {
	if(strcmp(_sys_info.machine, "raspi2B") == 0)
		return pl011_uart_init();
	return mini_uart_init();
}

int32_t uart_write(const void* data, uint32_t size) {
	if(strcmp(_sys_info.machine, "raspi2B") == 0)
		return pl011_uart_write(data, size);
	return mini_uart_write(data, size);
}