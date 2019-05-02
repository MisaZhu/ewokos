#include <unistd.h>
#include <cmain.h>
#include <stdio.h>
#include <vfs/fs.h>
#include <kstring.h>
#include <stdlib.h>

int main() {
	char fname_r[FULL_NAME_MAX];
	char fname_w[FULL_NAME_MAX];

	init_cmain_arg();
	const char* arg = read_cmain_arg();

	arg = read_cmain_arg();
	if(arg == NULL)
		return -1;
	fs_full_name(arg, fname_r, FULL_NAME_MAX);
	int fd_r = open(fname_r, O_RDONLY);
	if(fd_r < 0)
		return -1;

	int fd_w = 0;
	arg = read_cmain_arg();
	if(arg != NULL) {
		fs_full_name(arg, fname_w, FULL_NAME_MAX);
		fd_w = open(fname_w, O_RDWR|O_CREAT);
		if(fd_w < 0) {
			close(fd_r);
			return -1;
		}
	}

	while(true) {
		char buf[128+1];
		int sz = read(fd_r, buf, 128);
		if(sz <= 0)
			break;
		buf[sz] = 0;
		write(fd_w, buf, sz);
	}
	close(fd_r);
	close(fd_w);
	return 0;
}
