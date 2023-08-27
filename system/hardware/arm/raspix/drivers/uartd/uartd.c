#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <sysinfo.h>
#include <sys/syscall.h>
#include <sys/vdevice.h>
#include <sys/charbuf.h>
#include <sys/mmio.h>
#include <sys/proc.h>
#include <sys/ipc.h>
#include <sys/interrupt.h>
#include <arch/bcm283x/mini_uart.h>
#include <arch/bcm283x/pl011_uart.h>
#include <sys/interrupt.h>

static charbuf_t _TxBuf;
static charbuf_t _RxBuf;
static bool _mini_uart;

static int uart_read(int fd, int from_pid, fsinfo_t* info, 
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)offset;
	(void)info;
	(void)size;
	(void)p;

	int i;
	for(i = 0; i < size; i++){
	int res = charbuf_pop(&_RxBuf, buf + i);
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

	/*int i;
	for(i = 0; i < size; i++){
		char ch = ((char*)buf)[i];
		if(ch == '\r')
			ch = '\n';

		while(true){
			if(charbuf_push(&_TxBuf, ch, false) == 0){
				break;
			} 
			usleep(100);
		};
	}
	return size;
	*/
	if(_mini_uart)
		return bcm283x_mini_uart_write(buf, size);
	else
		return bcm283x_pl011_uart_write(buf, size);
}

static void interrupt_handle(uint32_t interrupt, uint32_t data) {
	(void)interrupt;
	(void)data;
	char c;

	if(_mini_uart) {
		while(bcm283x_mini_uart_ready_to_recv() == 0){
			c = bcm283x_mini_uart_recv();
			charbuf_push(&_RxBuf, c, true);
		}
	}
	else {
		while(bcm283x_pl011_uart_ready_to_recv() == 0){
			c = bcm283x_pl011_uart_recv();
			charbuf_push(&_RxBuf, c, true);
		}
	}

	/*if(bcm283x_mini_uart_ready_to_send() == 0){
		if( charbuf_pop(&_TxBuf, &c) == 0){
			bcm283x_mini_uart_send(c);
		}
	}
	*/
	sys_interrupt_end();
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/tty0";
	_mmio_base = mmio_map();
	_mini_uart = true;
	charbuf_init(&_TxBuf);
	charbuf_init(&_RxBuf);

	sys_info_t sysinfo;
	syscall1(SYS_GET_SYS_INFO, (int32_t)&sysinfo);
	if(strcmp(sysinfo.machine, "raspi1") == 0 ||
			strcmp(sysinfo.machine, "raspi2B") == 0)  {
		_mini_uart = false;
	}

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "mini_uart");
	dev.read = uart_read;
	dev.write = uart_write;

	sys_interrupt_setup(SYS_INT_TIMER0, interrupt_handle, 0);
	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
