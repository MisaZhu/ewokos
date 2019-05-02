#include <unistd.h>
#include <stdio.h>
#include <cmain.h>
#include <vfs/fs.h>

int main() {
	init_cmain_arg();
	const char* arg = read_cmain_arg();
	arg = read_cmain_arg();

	if(arg != NULL) {
		char dir[NAME_MAX];
		char name[NAME_MAX];
		fs_full_path(arg, dir, NAME_MAX-1, name, NAME_MAX-1);	
		int fd = fs_open(dir, O_RDWR);
		if(fd >= 0) {
			fs_add(fd, name, FS_TYPE_DIR);
			fs_close(fd);
		}
	}
	return 0;
}
