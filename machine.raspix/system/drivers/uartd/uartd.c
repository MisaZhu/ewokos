#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <sysinfo.h>
#include <ewoksys/syscall.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/charbuf.h>
#include <ewoksys/mmio.h>
#include <ewoksys/proc.h>
#include <ewoksys/ipc.h>
#include <arch/bcm283x/mini_uart.h>
#include <arch/bcm283x/pl011_uart.h>

static charbuf_t *_RxBuf;
static bool _mini_uart;
static bool _no_return;

static int uart_read(int fd, int from_pid, fsinfo_t* node, 
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)offset;
	(void)node;
	(void)size;
	(void)p;

	int i;
	for(i = 0; i < size; i++){
	int res = charbuf_pop(_RxBuf, buf + i);
	if(res != 0)
		break;
	}
	return (i==0)?VFS_ERR_RETRY:i;
}

static int uart_write(int fd, int from_pid, fsinfo_t* node,
		const void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)node;
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
			proc_usleep(100);
		};
	}
	return size;
	*/
	if(_mini_uart)
		return bcm283x_mini_uart_write(buf, size);
	else
		return bcm283x_pl011_uart_write(buf, size);
}

static int loop(void* p) {
	(void)p;
	char c;
	if(_mini_uart) {
		c = bcm283x_mini_uart_recv();
		if(c != '\r' || !_no_return) {
			ipc_disable();
			charbuf_push(_RxBuf, c, true);
			ipc_enable();
			proc_wakeup(RW_BLOCK_EVT);
		}
	}
	else {
		c = bcm283x_pl011_uart_recv(100);
		if(c != '\r' || !_no_return) {
			ipc_disable();
			charbuf_push(_RxBuf, c, true);
			ipc_enable();
			proc_wakeup(RW_BLOCK_EVT);
		}
	}
	proc_usleep(10000);
	return 0;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/tty0";
	_mmio_base = mmio_map();
	_mini_uart = true;
	_no_return = false;

	if(argc > 2 && strcmp(argv[2], "nr") == 0)
		_no_return = true;

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));

	sys_info_t sysinfo;
	syscall1(SYS_GET_SYS_INFO, (ewokos_addr_t)&sysinfo);
	if(strcmp(sysinfo.machine, "raspberry-pi1") == 0 ||
			strcmp(sysinfo.machine, "raspberry-pi2b") == 0)  {
		strcpy(dev.name, "pl011_uart");
		_mini_uart = false;
	}
	else
		strcpy(dev.name, "mini_uart");

	_RxBuf = charbuf_new(0);

	dev.read = uart_read;
	dev.write = uart_write;
	dev.loop_step = loop;

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666);

	charbuf_free(_RxBuf);
	return 0;
}
