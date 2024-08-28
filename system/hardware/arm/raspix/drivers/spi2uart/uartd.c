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
#include <arch/bcm283x/spi.h>

#include "sc16is750.h"

static charbuf_t *_RxBuf = NULL;
static 	SC16IS750_t spiuart;
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
	for(int i = 0; i < size; i++){
		uint8_t c = ((uint8_t*)(buf+offset))[i];
		SC16IS750_write(&spiuart, SC16IS750_CHANNEL_B, c);	
	}
	return size;
}

static int loop(void* p) {
	(void)p;
	char c;
	ipc_disable();
	int len = SC16IS750_available(&spiuart, SC16IS750_CHANNEL_B);
	for(int i = 0; i < len; i++){
		c = SC16IS750_read(&spiuart, SC16IS750_CHANNEL_B);
		if(c != '\r' || !_no_return) {
			charbuf_push(_RxBuf, c, true);
			proc_wakeup(RW_BLOCK_EVT);
		}
	}
	ipc_enable();
	proc_usleep(10000);
	return 0;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/tty1";
	_mmio_base = mmio_map();

	if(argc > 2 && strcmp(argv[2], "nr") == 0)
		_no_return = true;

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "spi_uart");

	SC16IS750_init(&spiuart, SC16IS750_PROTOCOL_SPI, 18, SC16IS750_DUAL_CHANNEL);
	SC16IS750_begin(&spiuart, SC16IS750_DEFAULT_SPEED, SC16IS750_DEFAULT_SPEED, 14745600UL); //baudrate&frequency setting

	if (SC16IS750_ping(&spiuart)!=1) 
		return 0;

	_RxBuf = charbuf_new(0);

	dev.read = uart_read;
	dev.write = uart_write;
	dev.loop_step = loop;

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666);

	charbuf_free(_RxBuf);
	return 0;
}
