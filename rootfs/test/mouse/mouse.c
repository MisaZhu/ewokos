#include <device.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main() {
	printf("read mouse, click to quit.\n");
	int fd = open("/dev/mouse0", 0);
	if(fd < 0)
		return -1;

	while(true) {
		int8_t ev[4];
		int sz = read(fd, ev, 4);
		if(sz < 0)
			break;
		if(sz == 0)
			continue;
	
		printf("mouse: down:0x%x, rx:%d, ry:%d, rz:%d\n", ev[0], ev[1], ev[2], ev[3]);
		if(ev[0] == 1)
			break;
	}
	close(fd);
	return 0;
}
