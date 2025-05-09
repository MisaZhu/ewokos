#include <arch/bcm283x/gpio_actled.h>
#include <arch/bcm283x/gpio.h>
#include <ewoksys/vdevice.h>
#include <string.h>

static void actled(bool on) {
	bcm283x_gpio_actled(on);
}

static int actled_write(int fd, int from_pid, fsinfo_t* node,
		const void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)node;
	(void)from_pid;
	(void)offset;
	(void)p;

	if(size == 0 || ((const char*)buf)[0] == 0) {
		actled(false);
	}
	else {
		actled(true);
	}
	return size;
}

static int actled_dev_cntl(int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
	(void)from_pid;
	(void)cmd;
	(void)ret;
	(void)p;

	if(proto_read_int(in) == 0) {
		actled(false);
	}
	else {
		actled(true);
	}
	return 0;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/actled";
	bcm283x_gpio_init();

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "actled");
	dev.write = actled_write;
	dev.dev_cntl = actled_dev_cntl;
	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666);
	return 0;
}
