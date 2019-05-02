#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	printf("read keyboard('enter' to quit):\n");
	int fd = open("/dev/keyb0", 0);
	if(fd < 0) {
		return -1;
	}

	while(true) {
		char buf[1];
		int sz = read(fd, buf, 1);
		if(sz <= 0)
			break;
		printf("%c", buf[0]);
		if(buf[0] == '\r')
			break;
	}

	close(fd);
	return 0;
}

