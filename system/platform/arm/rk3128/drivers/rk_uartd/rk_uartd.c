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

#define UART_LSR_THRE	0x20
#define UART_LSR_DR 	0x01      
#define REG32(x) (*(volatile uint32_t*)(x))

static int uart_read(int fd, int from_pid, fsinfo_t* node, 
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)node;
	(void)size;
	(void)offset;
	(void)p;

	if((REG32(_mmio_base + 0x60014) & UART_LSR_DR) == 0)
		return VFS_ERR_RETRY;

	((char*)buf)[0] = (char)REG32(_mmio_base + 0x60000);
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
		while (((REG32(_mmio_base + 0x60014)) & UART_LSR_THRE) == 0);	
			REG32(_mmio_base + 0x60000) = c;
	}
	return size;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/tty1";
	_mmio_base = mmio_map_offset(0x10000000, 8*1024*1024);
	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "rk_uart");
	dev.read = uart_read;
	dev.write = uart_write;

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666);
	return 0;
}
