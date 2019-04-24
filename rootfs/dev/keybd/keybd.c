#include <dev/devserv.h>
#include <unistd.h>
#include <syscall.h>
#include <devices.h>

int32_t keyb_read(uint32_t node, void* buf, uint32_t size, int32_t seek) {
	(void)node;
	(void)size;
	(void)seek;

	int32_t res;
	while(true) {
		res = syscall3(SYSCALL_DEV_READ, dev_typeid(DEV_KEYBOARD, 0), (int32_t)buf, 1);
		if(res != 0)
			break;
	}
	return res;
}

int main() {
	device_t dev = {0};
	dev.read = keyb_read;

	dev_run(&dev, "dev.keyb", 0, "/dev/keyb0", true);
	return 0;
}
