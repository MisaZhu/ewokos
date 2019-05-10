#include <unistd.h>
#include <stdio.h>
#include <cmain.h>
#include <vfs/fs.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
	if(argc > 1) {
		tstr_t* dir = tstr_new("", MFS);
		tstr_t* name = tstr_new("", MFS);
		fs_parse_name(argv[1], dir, name);
		int fd = fs_open(CS(dir), O_RDWR);
		if(fd >= 0) {
			fs_add(fd, CS(name), FS_TYPE_DIR);
			fs_close(fd);
		}
		tstr_free(dir);
		tstr_free(name);
	}
	return 0;
}
