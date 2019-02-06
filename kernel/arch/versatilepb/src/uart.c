#include <mm/mmu.h>
#include <hardware.h>
#include <dev/uart.h>

/* memory mapping for the serial port */
#define UART0 ((volatile uint32_t*)(MMIO_BASE+0x001f1000))
/* serial port register offsets */
#define UART_DATA        0x00 
#define UART_FLAGS       0x06
#define UART_INT_ENABLE  0x0e
#define UART_INT_TARGET  0x0f
#define UART_INT_CLEAR   0x11

/* serial port bitmasks */
#define UART_RECEIVE  0x10
#define UART_TRANSMIT 0x20

void uartDevInit(void) {
	UART0[UART_INT_ENABLE] = UART_RECEIVE;
}


void uartTransmit(char c) {
	/* wait until transmit buffer is full */
	while (UART0[UART_FLAGS] & UART_TRANSMIT);

	/* write the character */
	UART0[UART_DATA] = c;
}

bool uartReadyToRecv(void) {
	return UART0[UART_INT_TARGET] & UART_RECEIVE;
}

int uartReceive(void) {
	return UART0[UART_DATA];
}

