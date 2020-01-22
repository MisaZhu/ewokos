#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <sys/vdevice.h>
#include <sys/mmio.h>

#define AUX_OFFSET 0x00215000
#define UART_OFFSET 0x00215040

#define AUX_BASE (_mmio_base | AUX_OFFSET)
#define UART_BASE (_mmio_base | UART_OFFSET)

#define AUX_ENABLES (AUX_BASE+0x04)
#define UART_AUX_ENABLE 0x01

#define UART_IO_REG   (UART_BASE+0x00)
#define UART_IER_REG  (UART_BASE+0x04)
#define UART_IIR_REG  (UART_BASE+0x08)
#define UART_LCR_REG  (UART_BASE+0x0C)
#define UART_MCR_REG  (UART_BASE+0x10)
#define UART_LSR_REG  (UART_BASE+0x14)
#define UART_MSR_REG  (UART_BASE+0x18)
#define UART_SCRATCH  (UART_BASE+0x1C)
#define UART_CNTL_REG (UART_BASE+0x20)
#define UART_STAT_REG (UART_BASE+0x24)
#define UART_BAUD_REG (UART_BASE+0x28)

#define UART_BAUD_115200 270
#define UART_BAUD_9600 3254
#define UART_BAUD_DEFAULT UART_BAUD_115200

#define UART_TXD_GPIO 14
#define UART_RXD_GPIO 15

#define UART_TXFIFO_EMPTY 0x20
#define UART_RXFIFO_AVAIL 0x01

static uint32_t _mmio_base = 0;

void uart_trans(uint32_t data) {
	while(!(get32(UART_LSR_REG) & UART_TXFIFO_EMPTY));
	if(data == '\r')
		data = '\n';
	put32(UART_IO_REG, data);
}

int32_t uart_ready_to_recv(void) {
	if((get32(UART_LSR_REG)&UART_RXFIFO_AVAIL) == 0)
		return -1;
	return 0;
}

int32_t uart_recv(void) {
	return get32(UART_IO_REG) & 0xFF;
}

int32_t uart_write(const void* data, uint32_t size) {
  int32_t i;
  for(i=0; i<(int32_t)size; i++) {
    char c = ((char*)data)[i];
    uart_trans(c);
  }
  return i;
}

static int tty_read(int fd, int from_pid, fsinfo_t* info,
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)offset;
	(void)info;
	(void)size;
	(void)p;
	if(uart_ready_to_recv() != 0)
		return ERR_RETRY;

	char c = uart_recv();
	if(c == 0) 
		return ERR_RETRY;

	((char*)buf)[0] = c;
	return 1;	
}

static int tty_write(int fd, int from_pid, fsinfo_t* info, 
		const void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)info;
	(void)from_pid;
	(void)offset;
	(void)p;
	return uart_write(buf, size);
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/tty0";
	_mmio_base = mmio_map();

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "tty");
	dev.read = tty_read;
	dev.write = tty_write;

	device_run(&dev, mnt_point, FS_TYPE_DEV, NULL, 1);
	return 0;
}
