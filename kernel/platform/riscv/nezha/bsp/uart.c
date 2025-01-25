#include <dev/uart.h>
#include <kernel/hw_info.h>
#include <kstring.h>
#include <sbi.h>

#include <dev/uart.h>
#include <kernel/hw_info.h>
#include <mm/mmu.h>


/* memory mapping for the serial port */
#define UART0_OFFSET 0x2500000
//#define UART0_OFFSET 0x00000
#define UART0 (_sys_info.mmio.v_base + UART0_OFFSET)

/* serial port register offsets */
#define UART_LSR_OFFSET 0x14
#define UART_THR_OFFSET 0x0 

#define UART_LSR_THRE       0x20    /* Transmit-hold-register empty */

int32_t uart_dev_init(uint32_t baud) {

}


static inline void uart_putc(char c) {
    while ((get8(UART0 + UART_LSR_OFFSET) & UART_LSR_THRE) == 0);
    	put8(UART0 + UART_THR_OFFSET, c);
}

int32_t uart_write(const void* data, uint32_t size) {
	int32_t i;
	for(i=0; i<(int32_t)size; i++) {
		char c = ((char*)data)[i];
		uart_putc(c);
	}
	return i;
}



