#include <types.h>
#include <stdio.h>
#include <kstring.h>
#include <unistd.h>
#include <stdlib.h>
#include <vfs/fs.h>
#include <shm.h>

void fbtest() {
	int g = fsOpen("/dev/console0", 0);
	if(g < 0)
		return;

	int i = 0;
	char s[32];

	while(i<100) {
		snprintf(s, 31, "Hello, MicroKernel OS! (%d)\n", i++);
		fsWrite(g, s, strlen(s));
	}

	fsClose(g);
}

void _start() {
	fbtest();
	exit(0);
}

