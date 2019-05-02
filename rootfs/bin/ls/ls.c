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
	printf("cwd: %s\n", name);
	if(fd >= 0) {
		int32_t i = 0;
		while(true) {
			fs_info_t info;
			if(fs_kid(fd, i, &info) != 0)
				break;
			if(info.type == FS_TYPE_FILE)
				printf("  %16s -f-  %4d  %d\n", info.name, info.owner, info.size);
			else if(info.type == FS_TYPE_DIR)
				printf(" +%16s -d-  %4d  %d\n", info.name, info.owner, info.size);
			i++;
		}
		fs_close(fd);
	}

	return 0;
}
