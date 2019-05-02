#include <unistd.h>
#include <stdio.h>
#include <cmain.h>
#include <vfs/fs.h>

int main(int argc, char* argv[]) {
	if(argc > 1) {
		char dir[FULL_NAME_MAX];
		char name[SHORT_NAME_MAX];
		fs_parse_name(argv[1], dir, FULL_NAME_MAX, name, SHORT_NAME_MAX);	
		int fd = fs_open(dir, O_RDWR);
		if(fd >= 0) {
			fs_add(fd, name, FS_TYPE_DIR);
			fs_close(fd);
		}
	}
	return 0;
}
