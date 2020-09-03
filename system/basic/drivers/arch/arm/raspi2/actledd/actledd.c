#include <arch/bcm283x/gpio_arch.h>
#include <sys/vdevice.h>
#include <string.h>

static void actled(bool on) {
  uint32_t ra;
  ra = get32(GPIO_FSEL4);
  ra &= ~(7<<21);
  ra |= 1<<21;
  put32(GPIO_FSEL4, ra);

  if(!on)
    put32(GPIO_CLR1, 1<<(47-32));
  else
    put32(GPIO_SET1, 1<<(47-32));
}

static int actled_write(int fd, int from_pid, fsinfo_t* info,
		const void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)info;
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
	gpio_arch_init();

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "actled");
	dev.write = actled_write;
	dev.dev_cntl = actled_dev_cntl;
	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
