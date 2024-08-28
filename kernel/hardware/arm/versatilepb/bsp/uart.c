#include <dev/uart.h>
#include <kernel/hw_info.h>
#include <mm/mmu.h>

/* memory mapping for the serial port */
#define UART0 ((volatile uint32_t*)(_sys_info.mmio.v_base+0x001f1000))
/* serial port register offsets */
#define UART_DATA        0x00 
#define UART_FLAGS       0x18
#define UART_INT_ENABLE  0x0e
#define UART_INT_TARGET  0x0f
#define UART_INT_CLEAR   0x11

/* serial port bitmasks */
#define UART_RECEIVE  0x10
#define UART_TRANSMIT 0x20

int32_t uart_dev_init(uint32_t baud) {
	put32(UART0+UART_INT_ENABLE, UART_RECEIVE);
	return 0;
}

static inline void uart_basic_trans(char c) {
	/* wait until transmit buffer is full */
	while (UART0[UART_FLAGS] & UART_TRANSMIT);

	/* write the character */
	if(c == '\r')
		c = '\n';
	put8(UART0+UART_DATA, c);
}

int32_t uart_write(const void* data, uint32_t size) {
	int32_t i;
	for(i=0; i<(int32_t)size; i++) {
		char c = ((char*)data)[i];
		uart_basic_trans(c);
	}
	return i;
}

