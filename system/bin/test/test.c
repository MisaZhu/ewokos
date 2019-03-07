#include <types.h>
#include <stdio.h>
#include <kstring.h>
#include <unistd.h>
#include <stdlib.h>
#include <fb.h>
#include <graph/graph.h>

void fbtest() {
	FBInfoT fbInfo;
	fbGetInfo(&fbInfo);
	fbOpen();

	GraphT* g = graphNew(fbInfo.width, fbInfo.height);

	clear(g, 0x444444);
	drawText(g, 10, 100, "Hello, MicroKernel OS!", &fontBig, 0xFFFFFF);
	fbWrite((void*)g->buffer, 4 * g->w * g->h);

	graphFree(g);
	fbClose();
}

void _start() {
	fbtest();
	exit(0);
}

