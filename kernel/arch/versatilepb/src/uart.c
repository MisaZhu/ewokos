#include <mm/mmu.h>
#include <hardware.h>
#include <dev/uart.h>
#include <dev/device.h>
#include <system.h>
#include <proc.h>

/* memory mapping for the serial port */
#define UART0 ((volatile uint32_t*)(MMIO_BASE+0x001f1000))
/* serial port register offsets */
#define UART_DATA        0x00 
#define UART_FLAGS       0x18
#define UART_INT_ENABLE  0x0e
#define UART_INT_TARGET  0x0f
#define UART_INT_CLEAR   0x11

/* serial port bitmasks */
#define UART_RECEIVE  0x10
#define UART_TRANSMIT 0x20

void uart_init(void) {
	UART0[UART_INT_ENABLE] = UART_RECEIVE;
}

void uart_trans(char c) {
	/* wait until transmit buffer is full */
	while (UART0[UART_FLAGS] & UART_TRANSMIT);
	/* write the character */
	UART0[UART_DATA] = c;
}

void uart_putch(int c) {
	if(c == '\r')
		c = '\n';
	uart_trans(c);
}

bool uart_ready_to_recv(void) {
	return UART0[UART_INT_TARGET] & UART_RECEIVE;
}

int32_t uart_recv(void) {
	return UART0[UART_DATA];
}

#define UART_BUF_SIZE 16

static char _buffer_data[UART_BUF_SIZE];
static dev_buffer_t _uart_buffer = { _buffer_data, UART_BUF_SIZE, 0, 0 };
static int32_t _uart_lock = 0;

int32_t dev_uart_read(int16_t id, void* buf, uint32_t size) {
	(void)id;

	CRIT_IN(_uart_lock)
	char* p = (char*)buf;	
	int32_t i = 0;
	while(i < (int32_t)size) {
		char c;
		if(dev_buffer_pop(&_uart_buffer, &c) != 0) 
			break;
		p[i] = c;
		i++;
	}
	CRIT_OUT(_uart_lock)

	//if(i == 0)
	//	proc_sleep((int32_t)&_uart_buffer);
	return i;	
}

int32_t uart_getch() {
	char c;
	if(dev_uart_read(0, &c, 1) == 0)
		return 0;
	return (int32_t)c;
}

int32_t dev_uart_write(int16_t id, void* buf, uint32_t size) {
	(void)id;
	CRIT_IN(_uart_lock)
	char* p = (char*)buf;
	uint32_t i;
	for(i=0; i<size; i++) {
		uart_putch(p[i]);
	}
	CRIT_OUT(_uart_lock)
	return -1;
}

void uart_handle(void) {
	while (uart_ready_to_recv()) {
		int32_t data = uart_recv();

		CRIT_IN(_uart_lock);
		dev_buffer_push(&_uart_buffer, (char)data, false);
		//proc_wake((int32_t)&_uart_buffer);
		CRIT_OUT(_uart_lock);
	}
}
