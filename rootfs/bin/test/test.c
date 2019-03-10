#include <types.h>
#include <stdio.h>
#include <kstring.h>
#include <unistd.h>
#include <stdlib.h>
#include <graph/graph.h>
#include <vfs/fs.h>
#include <shm.h>
#include <fbinfo.h>

void fbtest() {
	FBInfoT fbInfo;
	int fd = fsOpen("/dev/fb0", 0);
	if(fd < 0) {
		printf("open failed!\n");
		return;
	}
	if(fsCtrl(fd, 0, NULL, 0, &fbInfo, sizeof(FBInfoT)) != 0) {
		printf("get fbinfo failed!\n");
		fsClose(fd);
		return;
	}

	uint32_t sz;
	int shmID = fsDMA(fd, &sz);
	if(shmID < 0 || sz == 0) {
		printf("get dma failed!\n");
		fsClose(fd);
		return;
	}
	
	void* p = shmMap(shmID);
	if(p == NULL) {
		printf("map dma failed!\n");
		fsClose(fd);
		return;
	}

	GraphT* g = graphNew(p, fbInfo.width, fbInfo.height);

	int i = 0;
	char s[32];

	while(true) {
		clear(g, 0xF*i);
		snprintf(s, 31, "Hello, MicroKernel OS! (%d)", i++);
		drawText(g, 10, 100, s, &fontBig, 0xFFFFFF);
		fsFlush(fd);
	}

	shmUnmap(shmID);
	graphFree(g);
	fsClose(fd);
}

void _start() {
	fbtest();
	exit(0);
}

