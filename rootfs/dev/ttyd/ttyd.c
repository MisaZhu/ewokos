#include <dev/devserv.h>
#include <unistd.h>
#include <syscall.h>
#include <stdio.h>

static int tty_getch(char* c) {
	return syscall3(SYSCALL_DEV_CHAR_READ, dev_typeid(DEV_UART, 0), (int32_t)c, 1);
}

int32_t tty_write(int32_t pid, int32_t fd, void* buf, uint32_t size, int32_t seek) {
	(void)pid;
	(void)fd;
	(void)seek;

	syscall3(SYSCALL_DEV_CHAR_WRITE, dev_typeid(DEV_UART, 0), (int32_t)buf, (int32_t)size);
	return size;
}

int32_t tty_read(int32_t pid, int32_t fd, void* buf, uint32_t size, int32_t seek) {
	(void)pid;
	(void)fd;
	(void)size;
	(void)seek;

	char* p = (char*)buf;
	int32_t res = tty_getch(&p[0]);
	errno = ENONE;
	if(res == 0) {
		res = -1;
		errno = EAGAIN;
	}
	else if(res < 0) 
		return -1;
	if(((char*)buf)[0] == 0x4)//ctrl+d
		res = 0;
	return res;
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	device_t dev = {0};
	dev.write = tty_write;
	dev.read = tty_read;

	dev_run(&dev, argc, argv);
	return 0;
}
