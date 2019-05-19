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
	x_set_font(fd, "8x10", 0, 0);
	x_set_bg(fd, 0x0);
	x_move_to(fd, 100, 100);

	int32_t i=0;
	while(true) {
		char s[32];
		snprintf(s, 31, "test %d", i++);
		x_clear(fd);
		x_text(fd, 10, 10, s);
		usleep(100000);
	}
	return 0;
}

