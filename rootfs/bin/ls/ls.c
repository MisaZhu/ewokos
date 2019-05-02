#include <unistd.h>
#include <stdio.h>
#include <vfs/fs.h>
#include <stdlib.h>
#include <kstring.h>

int main() {
	char name[FULL_NAME_MAX];
	init_cmain_arg();
	const char* arg = read_cmain_arg();
	arg = read_cmain_arg();
	if(arg == NULL) {
		fs_full_name("", name, FULL_NAME_MAX);
	}
	else {
		fs_full_name(arg, name, FULL_NAME_MAX);
	}

	int fd = fs_open(name, 0);
	if(fd >= 0) {
		uint32_t num;
		fs_info_t* infos = fs_kids(fd, &num);
		if(infos != NULL) {
			for(uint32_t i=0; i<num; i++) {
				fs_info_t* info = &infos[i];
				if(info->type == FS_TYPE_FILE)
					printf("  %16s -f-  %4d  %d\n", info->name, info->owner, info->size);
				else if(info->type == FS_TYPE_DIR)
					printf(" +%16s -d-  %4d  %d\n", info->name, info->owner, info->size);
			}
			free(infos);
		}
		fs_close(fd);
	}

	return 0;
}
