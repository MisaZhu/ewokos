#include <unistd.h>
#include <stdio.h>
#include <kserv/fs.h>

void _start()
{
	char pwd[FNAME_MAX];
	int fd = fsOpen(getcwd(pwd, FNAME_MAX));
	if(fd >= 0) {
		FSInfoT info;
		while(1) {
			if(fsChild(fd, &info) < 0)
				break;
			if(info.type == FS_TYPE_FILE)
				printf("-f-\t%d\t%s\n", info.size, info.name);
			else if(info.type == FS_TYPE_DIR)
				printf("-d-\t%d\t[%s]\n", info.size, info.name);
			else if(info.type == FS_TYPE_DEV_FILE)
				printf("-v-\t%d\t%s\n", info.size, info.name);
		}
		fsClose(fd);
	}

	exit(0);
}
