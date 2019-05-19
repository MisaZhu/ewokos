#include <x/xcmd.h>
#include <unistd.h>

int32_t x_cmd(int32_t fd, int32_t cmd, proto_t* in, proto_t* out) {
	return fs_ctrl(fd, cmd, in, out);
}


