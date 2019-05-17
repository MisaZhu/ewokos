#include <dev/devserv.h>
#include <unistd.h>
#include <syscall.h>
#include <device.h>

int32_t kevent_read(int32_t pid, int32_t fd, void* buf, uint32_t size, int32_t seek) {
	(void)fd;
	(void)size;
	(void)seek;

	int32_t uid = syscall1(SYSCALL_GET_UID, pid);
	if(uid != 0)
		return -1;
	return syscall3(SYSCALL_DEV_CHAR_READ, dev_typeid(DEV_KEVENT, 0), (int32_t)buf, size);
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	device_t dev = {0};
	dev.read = kevent_read;

	dev_run(&dev, argc, argv);
	return 0;
}
