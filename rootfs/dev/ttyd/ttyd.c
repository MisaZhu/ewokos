#include <dev/devserv.h>
#include <unistd.h>
#include <syscall.h>
#include <stdio.h>

static void tty_putch(int c) {
	char buf[1];
	buf[0] = (char)c;
	syscall3(SYSCALL_DEV_CHAR_WRITE, dev_typeid(DEV_UART, 0), (int32_t)buf, 1);
}

static int tty_getch() {
	char buf[1];
	while(true) {
		if(syscall3(SYSCALL_DEV_CHAR_READ, dev_typeid(DEV_UART, 0), (int32_t)buf, 1) > 0)
			break;
	}
	return (int)buf[0];
}

int32_t tty_write(uint32_t node, void* buf, uint32_t size, int32_t seek) {
	(void)node;
	(void)seek;

	const char* p = (const char*)buf;
	for(uint32_t i=0; i<size; i++) {
		tty_putch((int)p[i]);
	}
	return size;
}

int32_t tty_read(uint32_t node, void* buf, uint32_t size, int32_t seek) {
	(void)node;
	(void)size;
	(void)seek;

	char* p  = (char*)buf;
	*p = (char)tty_getch();
	if(*p == 4) //ctrl+d, means closed.
		return 0;
	return 1;
	/*for(uint32_t i=0; i<size; i++) {
		char c = (char)getch();
		if(c == 0) {
			size = i;
			break;
		}
		p[i] = c;
	}
	return size;
	*/
}

int main() {
	device_t dev = {0};
	dev.write = tty_write;
	dev.read = tty_read;

	dev_run(&dev, "dev.tty", 0, "/dev/tty0", true);
	return 0;
}
