#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/vdevice.h>
#include <sys/proto.h>
#include <x/xcntl.h>

static void input(char c) {
	char s[2];	
	s[0] = c;
	s[1] = 0;

	proto_t in;
	PF->init(&in, NULL, 0)->adds(&in, s);
	dev_cntl("/dev/x", X_DCNTL_INPUT, &in, NULL);
	PF->clear(&in);
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	int fd = open("/dev/keyb0", O_RDONLY);
	if(fd < 0)
		return 1;
	while(true) {
		char v;
		int rd = read(fd, &v, 1);
		if(rd == 1) {
			input(v);
		}
		usleep(30000);
	}

	close(fd);
	return 0;
}
