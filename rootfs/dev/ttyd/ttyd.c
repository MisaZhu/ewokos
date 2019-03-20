#include <dev/devserv.h>
#include <unistd.h>
#include <syscall.h>

int32_t ttyWrite(uint32_t node, void* buf, uint32_t size, int32_t seek) {
	(void)node;
	(void)seek;

	const char* p = (const char*)buf;
	for(uint32_t i=0; i<size; i++) {
		syscall1(SYSCALL_UART_PUTCH, (int)p[i]);
	}
	return size;
}

int32_t ttyRead(uint32_t node, void* buf, uint32_t size, uint32_t seek) {
	(void)node;
	(void)seek;

	char* p  = (char*)buf;
	for(uint32_t i=0; i<size; i++) {
		char c = syscall0(SYSCALL_UART_GETCH);
		if(c == 0) {
			size = i;
			break;
		}
		p[i] = c;
	}
	return size;
}

void _start() {
	device_t dev = {0};
	dev.write = ttyWrite;
	dev.read = ttyRead;

	dev_run(&dev, "dev.tty", 0, "/dev/tty0", true);
	exit(0);
}
