#include <unistd.h>
#include <stdio.h>
#include <vfs/fs.h>
#include <stdlib.h>

int main() {
	char pwd[NAME_MAX];
	int fd = fs_open(getcwd(pwd, NAME_MAX), 0);
	if(fd >= 0) {
		uint32_t num;
		fs_info_t* infos = fs_kids(fd, &num);
		if(infos != NULL) {
			for(uint32_t i=0; i<num; i++) {
				fs_info_t* info = &infos[i];
				if(info->type == FS_TYPE_FILE)
					printf("-f- %d\t%d\t%s\n", info->owner, info->size, info->name);
				else if(info->type == FS_TYPE_DIR)
					printf("-d- %d\t%d\t[%s]\n", info->owner, info->size, info->name);
			}
			free(infos);
		}
		fs_close(fd);
	}

	return 0;
}
