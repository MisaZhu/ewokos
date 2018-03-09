#include <pmessage.h>
#include <fork.h>
#include <malloc.h>
#include <stdio.h>
#include <kserv/fs.h>
#include <kserv/kserv.h>
#include <kserv/fs.h>
#include <string.h>

void _start()
{
	int fd = fsOpen("/initrd/test.c");
	if(fd >= 0) {
		FSInfoT info;
		fsInfo(fd, &info);
		printf("size: %d\n", info.size);

		if(info.size > 0) {
			char* buf = (char*)malloc(info.size);
			int res = fsRead(fd, buf, info.size);
			if(res >= 0)
				buf[res] = 0;
			//printf("%s\n", buf);
			free(buf);
		}
		fsClose(fd);
	}
	exit(0);
}
