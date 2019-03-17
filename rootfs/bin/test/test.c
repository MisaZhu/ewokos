#include <types.h>
#include <stdio.h>
#include <kstring.h>
#include <unistd.h>

void test() {
	int fd = open("/dev/console0", 0);
	if(fd < 0)
		return;

	int i = 0;
	char s[32];

	while(i<100) {
		snprintf(s, 31, "Hello, MicroKernel OS! (%d)\n", i++);
		write(fd, s, strlen(s));
	}
	close(fd);
}

void _start() {
	test();
	exit(0);
}

