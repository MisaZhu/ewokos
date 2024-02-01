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

#include "ms_serial.h"


#define BASE (_mmio_base + 0x221000)
#define UART_MULTI_REG8(_x_)  ((uint8_t volatile *)(BASE))[((_x_) * 4) - ((_x_) & 1)]


static int uart_read(int fd, int from_pid, fsinfo_t* node, 
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)offset;
	(void)node;
	(void)size;
	(void)p;

	char c;

    if(!(UART_MULTI_REG8(UART_LSR) & UART_LSR_DR))
		return VFS_ERR_RETRY;

    ((uint8_t*)buf)[0]=(char) ( UART_MULTI_REG8(UART_TX) & 0xff);

    return 1;
}

static int uart_write(int fd, int from_pid, fsinfo_t* node,
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
    	    while (!(UART_MULTI_REG8(UART_LSR) & UART_LSR_THRE));
    	    UART_MULTI_REG8(UART_TX) = c;
	}
	return size;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/tty1";
	_mmio_base = mmio_map();
	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "ms_uart");
	dev.read = uart_read;
	dev.write = uart_write;

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666);
	return 0;
}
