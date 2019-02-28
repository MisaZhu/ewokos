#include <unistd.h>
#include <stdio.h>
#include <kserv/fs.h>
#include <stdlib.h>

void _start()
{
	char pwd[NAME_MAX];
	int fd = fsOpen(getcwd(pwd, NAME_MAX), 0);
	if(fd >= 0) {
		uint32_t num;
		FSInfoT* infos = fsKids(fd, &num);
		if(infos != NULL) {
			for(uint32_t i=0; i<num; i++) {
				FSInfoT* info = &infos[i];
				if(info->type == FS_TYPE_FILE)
					printf("-f- %d\t%d\t%s\n", info->owner, info->size, info->name);
				else if(info->type == FS_TYPE_DIR)
					printf("-d- %d\t%d\t[%s]\n", info->owner, info->size, info->name);
			}
			free(infos);
		}
		fsClose(fd);
	}

	exit(0);
}
