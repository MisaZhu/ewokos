#include <dev/uart.h>
#include <mm/mmu.h>

#define REG32(x) (*(volatile uint32_t*)(x))

#define UART_BASE		(MMIO_BASE + 0x01D0C000)
#define UART_LSR		(UART_BASE + 0x14)
#define UART_TX			(UART_BASE + 0x0)

#define UART_LSR_THRE	0x20


int32_t uart_dev_init(uint32_t baud) {
	(void)baud;
	return 0;
}

int32_t uart_write(const void* data, uint32_t size) {
	char c;

	for(int i = 0; i <  (int)size; i++){
		c = ((char*)data)[i];
    	while (!(REG32(UART_LSR) & UART_LSR_THRE));
    	REG32(UART_TX) = c;
	}
	return 0;
}

