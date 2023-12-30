#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/charbuf.h>
#include <ewoksys/mmio.h>
#include <ewoksys/proc.h>
#include <ewoksys/ipc.h>
#include <ewoksys/interrupt.h>
#include <ewoksys/interrupt.h>

//#define DEBUG

/* memory mapping for the serial port */
#define UART0 (_mmio_base)
/* serial port register offsets */
#define UART_LSR_OFFSET 0x5
#define UART_THR_OFFSET 0x0 

#define UART_LSR_THRE       0x20    /* Transmit-hold-register empty */
#define UART_LSR_DR       	0x1    	/*Data Ready*/

int32_t uart_dev_init(void) {

}


static inline void uart_putc(char c) {
    while ((get8(UART0 + UART_LSR_OFFSET) & UART_LSR_THRE) == 0);
    put8(UART0 + UART_THR_OFFSET, c);
}


static int uart_read(int fd, int from_pid, uint32_t node, 
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)offset;
	(void)node;
	(void)size;
	(void)p;
	if((get8(UART0 + UART_LSR_OFFSET) & UART_LSR_DR) == 1){
		*(char*)buf = get8(UART0 + UART_THR_OFFSET);
		return 1;
	}else
    	return ERR_RETRY_NON_BLOCK;
}

static int uart_write(int fd, int from_pid, uint32_t node,
		const void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)node;
	(void)from_pid;
	(void)offset;
	(void)p;
	char c;

	for(int i = 0; i < size; i++){
		c = ((char*)buf)[i];
		if(c == '\r') c = '\n';
			uart_putc(c);
	}
	return size;
}

static void interrupt_handle(uint32_t interrupt, uint32_t data) {
	(void)interrupt;
	(void)data;
	char c;


	sys_interrupt_end();
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/tty1";
	_mmio_base = mmio_map();
	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "virt_uart");
	dev.read = uart_read;
	dev.write = uart_write;

	//sys_interrupt_setup(SYS_INT_TIMER0, interrupt_handle, 0);
	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}


