#include <x/xclient.h>
#include <unistd.h>

int32_t x_open(const char* dev_name) {
	return open(dev_name, O_RDWR);
}

void x_close(int32_t fd) {
	close(fd);
}
