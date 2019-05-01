#include <dev/devserv.h>
#include <unistd.h>
#include <syscall.h>
#include <stdio.h>

static int tty_getch(char* c) {
	return syscall3(SYSCALL_DEV_CHAR_READ, dev_typeid(DEV_UART, 0), (int32_t)c, 1);
}

int32_t tty_write(uint32_t node, void* buf, uint32_t size, int32_t seek) {
	(void)node;
	(void)seek;

	syscall3(SYSCALL_DEV_CHAR_WRITE, dev_typeid(DEV_UART, 0), (int32_t)buf, (int32_t)size);
	return size;
}

int32_t tty_read(uint32_t node, void* buf, uint32_t size, int32_t seek) {
	(void)node;
	(void)size;
	(void)seek;

	char* p = (char*)buf;
	int32_t res = tty_getch(&p[0]);
	if(res == 1 && p[0] == 4) //ctrl+d, means closed.
		return -1;
	return res;
}

int main() {
	device_t dev = {0};
	dev.write = tty_write;
	dev.read = tty_read;

	dev_run(&dev, "dev.tty", 0, "/dev/tty0", true);
	return 0;
}
