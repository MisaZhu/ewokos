#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <kstring.h>
#include <vprintf.h>
#include <x/xclient.h>

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	int32_t fd = x_open("/dev/xman0");
	if(fd < 0)
		return -1;

	proto_t* in;
	
	in = proto_new(NULL, 0);
	proto_add_str(in, "8x10");
	x_cmd(fd, X_CMD_SET_FONT, in, NULL);
	proto_free(in);
	
	in = proto_new(NULL, 0);
	proto_add_int(in, 0xff);
	x_cmd(fd, X_CMD_SET_BG, in, NULL);
	proto_free(in);

	in = proto_new(NULL, 0);
	proto_add_int(in, 100);
	proto_add_int(in, 100);
	x_cmd(fd, X_CMD_MOVE_TO, in, NULL);
	proto_free(in);

	int32_t i=0;
	while(true) {
		char s[32];
		snprintf(s, 31, "test %d", i++);
		x_cmd(fd, X_CMD_CLEAR, NULL, NULL);

		in = proto_new(NULL, 0);
		proto_add_int(in, 10);
		proto_add_int(in, 10);
		proto_add_str(in, s);
		x_cmd(fd, X_CMD_STR, in, NULL);
		proto_free(in);
		usleep(100000);
	}
	return 0;
}

