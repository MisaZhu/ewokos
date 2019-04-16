#include <unistd.h>
#include <stdio.h>
#include <vfs/fs.h>
#include <stdlib.h>

#define ALIGN_LEN 16
static const char* align_cmd(const char* cmd) {
	static char ret[ALIGN_LEN+1];
	int32_t i = 0;

	while(i< ALIGN_LEN) {
		ret[i] = cmd[i];
		if(cmd[i] == 0)
			break;
		i++;
	}
	
	while(i< ALIGN_LEN) {
		ret[i++] = ' ';
	}
	ret[i] = 0;
	return ret;
}


int main() {
	char pwd[NAME_MAX];
	int fd = fs_open(getcwd(pwd, NAME_MAX), 0);
	if(fd >= 0) {
		uint32_t num;
		fs_info_t* infos = fs_kids(fd, &num);
		if(infos != NULL) {
			for(uint32_t i=0; i<num; i++) {
				fs_info_t* info = &infos[i];
				const char* fname = align_cmd(info->name);
				if(info->type == FS_TYPE_FILE)
					printf("%s -f- %d  %d\n", fname, info->owner, info->size);
				else if(info->type == FS_TYPE_DIR)
					printf("%s -d- %d  %d\n", fname, info->owner, info->size);
			}
			free(infos);
		}
		fs_close(fd);
	}

	return 0;
}
