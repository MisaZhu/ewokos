#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <kstring.h>
#include <graph/graph.h>

void fbtest() {
	GraphT* g = graphOpen("/dev/fb0");
	if(g == NULL) {
		return;
	}

	int32_t i = 0;
	char s[32];
	while(i<1000) {
		clear(g, rgb(100, 100, 200));
		snprintf(s, 31, "Hello, MicroKernel OS! (%d)", i++);
		fill(g, 10, 10, fontBig.w* strlen(s) + 20, fontBig.h + 20, rgb(0, 0, 0));
		drawText(g, 20, 20, s, &fontBig, rgb(255, 255, 255));
		graphFlush(g);
	}

	graphClose(g);
}

void _start() {
	fbtest();
	exit(0);
}

