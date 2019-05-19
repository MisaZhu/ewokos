#include <x/xclient.h>
#include <unistd.h>

int32_t x_open(const char* dev_name) {
	return open(dev_name, O_RDWR);
}

void x_close(int32_t fd) {
	close(fd);
}

int32_t x_cmd(int32_t fd, int32_t cmd, proto_t* in, proto_t* out) {
	return fs_ctrl(fd, cmd, in, out);
}


