#include <dev/devserv.h>
#include <unistd.h>
#include <syscall.h>
#include <stdio.h>

int32_t tty_write(uint32_t node, void* buf, uint32_t size, int32_t seek) {
	(void)node;
	(void)seek;

	const char* p = (const char*)buf;
	for(uint32_t i=0; i<size; i++) {
		putch((int)p[i]);
	}
	return size;
}

int32_t tty_read(uint32_t node, void* buf, uint32_t size, int32_t seek) {
	(void)node;
	(void)seek;

	char* p  = (char*)buf;
	for(uint32_t i=0; i<size; i++) {
		char c = (char)getch();
		if(c == 0) {
			size = i;
			break;
		}
		p[i] = c;
	}
	return size;
}

int main() {
	device_t dev = {0};
	dev.write = tty_write;
	dev.read = tty_read;

	dev_run(&dev, "dev.tty", 0, "/dev/tty0", true);
	return 0;
}
