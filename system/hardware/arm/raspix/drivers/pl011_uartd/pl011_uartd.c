#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <sys/vdevice.h>
#include <sys/charbuf.h>
#include <sys/mmio.h>
#include <sys/proc.h>
#include <sys/klog.h>
#include <sys/timer.h>
#include <sys/interrupt.h>
#include <arch/bcm283x/pl011_uart.h>

static charbuf_t _buffer;

static int uart_read(int fd, int from_pid, fsinfo_t* info, 
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)offset;
	(void)info;
	(void)size;
	(void)p;

	/*char c;
	int res = charbuf_pop(&_buffer, &c);
	if(res != 0 || c == 0)
		return ERR_RETRY;
	((char*)buf)[0] = c;
	return 1;
	*/

	int i;
	for(i = 0; i < size; i++){
	int res = charbuf_pop(&_buffer, buf + i);
	if(res != 0)
		break;
	}
	return (i==0)?ERR_RETRY_NON_BLOCK:i;
}

static int uart_write(int fd, int from_pid, fsinfo_t* info,
		const void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)info;
	(void)from_pid;
	(void)offset;
	(void)p;
	return bcm283x_pl011_uart_write(buf, size);
}

/*
static void interrupt_handle(uint32_t interrupt, uint32_t data) {
	(void)interrupt;
	(void)data;

	char c;
	while(bcm283x_pl011_uart_ready_to_recv() == 0) {
		c = bcm283x_pl011_uart_recv();
		charbuf_push(&_buffer, c, true);
	}
	sys_interrupt_end();
}
*/

static void interrupt_handle(void) {
	char c;
	while(bcm283x_pl011_uart_ready_to_recv() == 0) {
		c = bcm283x_pl011_uart_recv();
		charbuf_push(&_buffer, c, true);
	}
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/tty0";
	_mmio_base = mmio_map();

	charbuf_init(&_buffer);

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "pl011_uart");
	dev.read = uart_read;
	dev.write = uart_write;

	//sys_interrupt_setup(SYS_INT_TIMER0, interrupt_handle, 0);
	uint32_t tid = timer_set(10000, interrupt_handle);
	if(tid == 0) {
		klog("Error: can't set timer\n");
		return -1;
	}

	device_run(&dev, mnt_point, FS_TYPE_CHAR);

	if(tid > 0)
		timer_remove(tid);
	return 0;
}

