#include <dev/uart.h>
#include <mm/mmu.h>
#include "ms_serial.h"


uint32_t uart_base = MMIO_BASE;

#define BASE (uart_base + 0x221000)
#define UART_MULTI_REG8(_x_)  ((uint8_t volatile *)(BASE))[((_x_) * 4) - ((_x_) & 1)]


int32_t uart_dev_init(uint32_t baud) {
	//return mini_uart_init(baud);
	return 0;
}

int32_t uart_write(const void* data, uint32_t size) {
	char c;

	for(int i = 0; i <  (int)size; i++){
		c = ((char*)data)[i];
    	    while (!(UART_MULTI_REG8(UART_LSR) & UART_LSR_THRE));
    	    UART_MULTI_REG8(UART_TX) = c;
	}
    while (!(UART_MULTI_REG8(UART_LSR) & UART_LSR_THRE));
	return 0;
}

