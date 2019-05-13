#include <dev/devserv.h>
#include <unistd.h>
#include <syscall.h>
#include <device.h>

int32_t mouse_read(int32_t pid, int32_t fd, void* buf, uint32_t size, int32_t seek) {
	(void)pid;
	(void)fd;
	(void)size;
	(void)seek;
	return syscall3(SYSCALL_DEV_CHAR_READ, dev_typeid(DEV_MOUSE, 0), (int32_t)buf, size);
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	device_t dev = {0};
	dev.read = mouse_read;

	dev_run(&dev, argc, argv);
	return 0;
}
