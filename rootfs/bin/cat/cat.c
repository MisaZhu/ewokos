#include <unistd.h>
#include <cmain.h>
#include <stdio.h>
#include <vfs/fs.h>
#include <kstring.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
	char fname[FULL_NAME_MAX];
	if(argc < 2)
		return -1;
	fs_full_name(argv[1], fname, FULL_NAME_MAX);
	int fd = open(fname, O_RDONLY);
	if(fd < 0) {
		printf("'%s' open failed!\n", fname);
		return -1;
	}

	int fd_w = _stdout;
	while(true) {
		char buf[128];
		int sz = read(fd, buf, 128);
		if(sz <= 0)
			break;
		while(true) { //non-block
			int res = write(fd_w, buf, sz);
			if(res < 0 && errno == EAGAIN)
				continue;
			break;
		}
	}
	close(fd);
	return 0;
}
