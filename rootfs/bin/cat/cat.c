#include <unistd.h>
#include <cmain.h>
#include <stdio.h>
#include <vfs/fs.h>
#include <kstring.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
	if(argc < 2)
		return -1;
	tstr_t* fname = fs_full_name(argv[1]);
	int fd = open(CS(fname), O_RDONLY);
	if(fd < 0) {
		printf("'%s' open failed!\n", CS(fname));
		tstr_free(fname);
		return -1;
	}
	tstr_free(fname);

	int fd_w = _stdout;
	while(true) {
		char buf[128];
		int sz;
		while(true) { //non-block
			sz = read(fd, buf, 128);
			if(sz < 0 && errno == EAGAIN)
				continue;
			break;
		}
		if(sz <= 0)
			break;

		int res = -1;
		while(true) { //non-block
			res = write(fd_w, buf, sz);
			if(res < 0 && errno == EAGAIN)
				continue;
			break;
		}
		if(res <= 0)
			break;
	}
	close(fd);
	return 0;
}
