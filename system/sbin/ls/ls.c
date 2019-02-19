#include <unistd.h>
#include <stdio.h>
#include <kserv/fs.h>

void _start()
{
	char pwd[NAME_MAX];
	int fd = fsOpen(getcwd(pwd, NAME_MAX));
	if(fd >= 0) {
		FSInfoT info;
		while(1) {
			if(fsChild(fd, &info) < 0)
				break;
			if(info.type == FS_TYPE_FILE)
				printf("-f- %d\t%d\t%s\n", info.owner, info.size, info.name);
			else if(info.type == FS_TYPE_DIR)
				printf("-d- %d\t%d\t[%s]\n", info.owner, info.size, info.name);
			else if(info.type == FS_TYPE_DEV_FILE)
				printf("-v- %d\t%d\t%s : %s(%d)\n", info.owner, info.size, info.name, info.device, info.index);
		}
		fsClose(fd);
	}

	exit(0);
}
