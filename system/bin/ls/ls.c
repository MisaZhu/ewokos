#include <unistd.h>
#include <stdio.h>
#include <kserv/fs.h>
#include <stdlib.h>

void _start()
{
	char pwd[NAME_MAX];
	int fd = fsOpen(getcwd(pwd, NAME_MAX), 0);
	if(fd >= 0) {
		int32_t num;
		FSInfoT* infos = fsKids(fd, &num);
		if(infos != NULL) {
			for(int i=0; i<num; i++) {
				FSInfoT* info = &infos[i];
				if(info->type == FS_TYPE_FILE)
					printf("-f- %d\t%d\t%s\n", info->owner, info->size, info->name);
				else if(info->type == FS_TYPE_DIR)
					printf("-d- %d\t%d\t[%s]\n", info->owner, info->size, info->name);
				else if(info->type == FS_TYPE_DEV_FILE)
					printf("-v- %d\t%d\t%s : %s(%d)\n", info->owner, info->size, info->name, info->device, info->index);
			}
			free(infos);
		}
		fsClose(fd);
	}

	exit(0);
}
