#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <sys/vdevice.h>
#include <sys/charbuf.h>
#include <sys/mmu.h>
#include <sys/proc.h>
#include <sys/ipc.h>
#include <arch/bcm283x/mini_uart.h>

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
  int res = charbuf_pop(&_buffer, &c);

  if(res != 0 || c == 0)
    return ERR_RETRY;

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

	int ret = bcm283x_mini_uart_write(buf, size);
	return ret;
}

static int uart_loop_raw(void) {
	if(bcm283x_mini_uart_ready_to_recv() != 0) {
		return 0;
	}

	char c = bcm283x_mini_uart_recv();
	if(c == 0) 
		return 0;

	charbuf_push(&_buffer, c, true);
	proc_wakeup(0);
	return 0;
}

static int uart_loop(void*p) {
	(void)p;
	int res = uart_loop_raw();
	usleep(50000);
	return res;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/tty1";
	_mmio_base = mmio_map(false);

	charbuf_init(&_buffer);

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "mini_uart");
	dev.read = uart_read;
	dev.write = uart_write;
	dev.loop_step = uart_loop;

	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
