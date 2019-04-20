#include <unistd.h>
#include <stdio.h>
#include <vfs/fs.h>
#include <stdlib.h>
#include <kstring.h>

int main() {
	char name[NAME_MAX];
	char pwd[NAME_MAX];
	getcwd(pwd, NAME_MAX);

	init_cmain_arg();
	const char* arg = read_cmain_arg();
	arg = read_cmain_arg();
	if(arg == NULL) {
		strcpy(name, pwd);
	}
	else if(arg[0] == '/') {
		strcpy(name, arg);
	}
	else {
		if(strcmp(pwd, "/") == 0)
			snprintf(name, NAME_MAX-1, "/%s", arg);
		else
			snprintf(name, NAME_MAX-1, "%s/%s", pwd, arg);
	}

	int fd = fs_open(name, 0);
	if(fd >= 0) {
		uint32_t num;
		fs_info_t* infos = fs_kids(fd, &num);
		if(infos != NULL) {
			for(uint32_t i=0; i<num; i++) {
				fs_info_t* info = &infos[i];
				if(info->type == FS_TYPE_FILE)
					printf("%16s -f-  %4d  %d\n", info->name, info->owner, info->size);
				else if(info->type == FS_TYPE_DIR)
					printf("*%15s -d-  %4d  %d\n", info->name, info->owner, info->size);
			}
			free(infos);
		}
		fs_close(fd);
	}

	return 0;
}
