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
#include <sys/interrupt.h>

/* memory mapping for the serial port */
#define UART0 ((volatile uint32_t*)(_mmio_base+0x001f1000))
/* serial port register offsets */
#define UART_DATA        0x00
#define UART_FLAGS       0x18
#define UART_INT_ENABLE  0x0e
#define UART_INT_TARGET  0x0f
#define UART_INT_CLEAR   0x11

/* serial port bitmasks */
#define UART_RECEIVE  0x10
#define UART_TRANSMIT 0x20

static inline void uart_basic_trans(char c) {
  /* wait until transmit buffer is full */
  while (UART0[UART_FLAGS] & UART_TRANSMIT);

  /* write the character */
  if(c == '\r')
    c = '\n';
  put8(UART0+UART_DATA, c);
}

int32_t uart_write(const void* data, uint32_t size) {
  int32_t i;
  for(i=0; i<(int32_t)size; i++) {
    char c = ((char*)data)[i];
    uart_basic_trans(c);
  }
  return i;
}

static charbuf_t _buffer;

static int tty_read(int fd, int from_pid, fsinfo_t* info,
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)offset;
	(void)info;
	(void)size;
	(void)p;

	char c;
	int res = charbuf_pop(&_buffer, &c);

	if(res != 0 || c == 0)
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

static void interrupt_handle(uint32_t interrupt) {
	(void)interrupt;
	uint32_t data = get32(UART0 + UART_DATA);
	charbuf_push(&_buffer, data, true);
	proc_wakeup(0);
	sys_interrupt_end();
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/tty0";
	_mmio_base = mmio_map(false);

	charbuf_init(&_buffer);

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "tty");
	dev.read = tty_read;
	dev.write = tty_write;

	sys_interrupt_setup(SYS_INT_UART0, interrupt_handle);
	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
