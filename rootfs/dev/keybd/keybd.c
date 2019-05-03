#include <dev/devserv.h>
#include <unistd.h>
#include <syscall.h>
#include <device.h>

int32_t keyb_read(uint32_t node, void* buf, uint32_t size, int32_t seek) {
	(void)node;
	(void)size;
	(void)seek;

	return syscall3(SYSCALL_DEV_CHAR_READ, dev_typeid(DEV_KEYBOARD, 0), (int32_t)buf, 1);
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	device_t dev = {0};
	dev.read = keyb_read;

	dev_run(&dev, argc, argv);
	return 0;
}
