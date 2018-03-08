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
		char buf[128];
		while(1) {
			int res = fsRead(fd, buf, 128);
			if(res <= 0)
				break;

			buf[res] = 0;
			printf("%s", buf);
		}
		fsClose(fd);
	}
	exit(0);
}
