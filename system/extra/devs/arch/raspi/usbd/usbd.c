#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/vdevice.h>
#include <sys/syscall.h>
#include <sys/mmio.h>
#include <usbd/usbd.h>

void LogPrint(const char* message, uint32_t messageLength) {
  (void)messageLength;
  kprintf(false, message);
}

void usb_host_init(void) {
  UsbInitialise();
  usleep(100000);
  UsbCheckForChange();
  usleep(100000);
  UsbCheckForChange();
  usleep(100000);
  UsbCheckForChange();
  usleep(100000);
  UsbCheckForChange();
  usleep(100000);
  UsbCheckForChange();
}

int main(int argc, char** argv) {
	uint32_t _mmio_base = mmio_map();
	if(_mmio_base == 0)
		return -1;

	usb_host_init();

	const char* mnt_point = argc > 1 ? argv[1]: "/dev/usb";

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "usbd");

	device_run(&dev, mnt_point, FS_TYPE_CHAR, NULL, 1);
	return 0;
}
