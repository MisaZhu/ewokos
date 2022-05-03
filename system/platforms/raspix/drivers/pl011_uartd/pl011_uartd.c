#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <sys/vdevice.h>
#include <sys/charbuf.h>
#include <sys/mmio.h>
#include <sys/proc.h>
#include <sys/ipc.h>
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

	char c;
	if(bcm283x_pl011_uart_ready_to_recv() != 0) {
		return ERR_RETRY_NON_BLOCK;
	}

	c = bcm283x_pl011_uart_recv();
	((char*)buf)[0] = c;
	return 1;
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

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/tty0";
	_mmio_base = mmio_map(false);

	charbuf_init(&_buffer);

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "pl011_uart");
	dev.read = uart_read;
	dev.write = uart_write;
	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}

