#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <kserv/fs.h>
#include <kserv/kserv.h>
#include <fb.h>
#include <shm.h>

void fbtest() {
	FBInfoT fbInfo;

	fbGetInfo(&fbInfo);
	fbOpen();

	uint32_t sz = 4*fbInfo.height*fbInfo.width;
	uint32_t *fb = (uint32_t*)malloc(sz);
	uint32_t color = 0x0;
	uint32_t loopy,loopx,index,check;
	
	/** do the thing... */
	check = fbInfo.pitch/sizeof(uint32_t);
	index = 0;
	for (loopy=0;loopy<fbInfo.height;loopy++) {
		for (loopx=0;loopx<fbInfo.width||loopx<check;loopx++) {
			fb[index++] = color;
			color ++;
		}
	}

	fbWrite((void*)fb, sz);
	fbClose();
	free(fb);
}

void _start() {
	void *p = shmalloc(10000);
	//shmUnmap(p);

	fbtest();
	exit(0);
}

