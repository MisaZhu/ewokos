#include <dev/devserv.h>
#include <unistd.h>
#include <syscall.h>
#include <device.h>

int32_t mouse_read(uint32_t node, void* buf, uint32_t size, int32_t seek) {
	(void)node;
	(void)size;
	(void)seek;

	int32_t res;
	while(true) {
		res = syscall3(SYSCALL_DEV_CHAR_READ, dev_typeid(DEV_MOUSE, 0), (int32_t)buf, size);
		if(res != 0)
			break;
		sleep(0);
	}
	return res;
}

int main() {
	device_t dev = {0};
	dev.read = mouse_read;

	dev_run(&dev, "dev.mouse", 0, "/dev/mouse0", true);
	return 0;
}
