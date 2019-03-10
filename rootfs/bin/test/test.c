#include <types.h>
#include <stdio.h>
#include <kstring.h>
#include <unistd.h>
#include <stdlib.h>
#include <graph/graph.h>
#include <vfs/fs.h>
#include <shm.h>

void fbtest() {
	GraphT* g = graphOpen("/dev/fb0");
	if(g == NULL)
		return;

	int i = 0;
	char s[32];

	while(i<100) {
		clear(g, 0xF*i);
		snprintf(s, 31, "Hello, MicroKernel OS! (%d)", i++);
		drawText(g, 10, 100, s, &fontBig, 0xFFFFFF);
		graphFlush(g);
	}

	graphClose(g);
}

void _start() {
	fbtest();
	exit(0);
}

