#include <arch/raspi2/gpio_arch.h>
#include <arch/raspi2/actled_arch.h>
#include <sys/vdevice.h>
#include <string.h>

static int actled_write(int fd, int from_pid, fsinfo_t* info,
		const void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)info;
	(void)from_pid;
	(void)offset;
	(void)p;

	if(size == 0 || ((const char*)buf)[0] == 0) {
		actled_arch(false);
	}
	else {
		actled_arch(true);
	}
	return size;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/actled";
	gpio_arch_init();

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "actled");
	dev.write = actled_write;
	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
