#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <kserv/fs.h>
#include <kserv/kserv.h>
#include <syscall.h>
#include <fbinfo.h>

void fbtest() {
	FBInfoT fbInfo;
	int fd = fsOpen("/dev/fb0", 0);
	if(fd < 0)
		return;

	fsCtrl(fd, 0, NULL, 0, &fbInfo, sizeof(FBInfoT));

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

	fsWrite(fd, (void*)fb, sz);
	fsClose(fd);
	free(fb);
}

void _start() {
	fbtest();
	exit(0);
}

