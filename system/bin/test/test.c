#include <types.h>
#include <stdio.h>
#include <kstring.h>
#include <unistd.h>
#include <stdlib.h>
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
	int id1 = shmalloc(100);
	int id2 = shmalloc(10000);

	int pid = fork();
	if(pid == 0) { //child
		char* p1 = (char*)shmMap(id1);
		char* p2 = (char*)shmMap(id2);
		strcpy(p1, "hello");	
		strcpy(p2, "world");	
		shmUnmap(id1);
		shmUnmap(id2);
		exit(0);
	}

	wait(pid);
	char* p1 = (char*)shmMap(id1);
	char* p2 = (char*)shmMap(id2);
	printf("%s\n", p1);
	printf("%s\n", p2);
//	shmfree(id1);
//	shmfree(id2);

	fbtest();
	exit(0);
}

