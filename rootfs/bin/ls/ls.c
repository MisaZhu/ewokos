#include <unistd.h>
#include <stdio.h>
#include <vfs/fs.h>
#include <stdlib.h>
#include <kstring.h>

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
