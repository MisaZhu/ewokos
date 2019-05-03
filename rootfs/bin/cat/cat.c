#include <unistd.h>
#include <cmain.h>
#include <stdio.h>
#include <vfs/fs.h>
#include <kstring.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
	char fname_r[FULL_NAME_MAX];
	if(argc < 2)
		return -1;
	fs_full_name(argv[1], fname_r, FULL_NAME_MAX);
	int fd_r = open(fname_r, O_RDONLY);
	if(fd_r < 0)
		return -1;

	int fd_w = _stdout;
	while(true) {
		char buf[128+1];
		int sz = read(fd_r, buf, 128);
		if(sz <= 0)
			break;
		buf[sz] = 0;
		write(fd_w, buf, sz);
	}
	close(fd_r);
	return 0;
}
