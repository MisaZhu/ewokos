#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/stat.h>

int fchmod(int fd, int mode);

void out(void* data, int32_t size) {
	char* buf = (char*)data;
	int32_t wr = 0;
	while(1) {
		if(size <= 0)
			break;

		int sz = write(1, buf, size);
		if(sz <= 0 && errno != EAGAIN)
			break;

		if(sz > 0) {
			size -= sz;
			wr += sz;
			buf += sz;
		}
	}
}

int main(int argc, char** argv) {
	if(argc < 3) {
		printf("  Usage: cp <file_from> <file_to>\n");
		return -1;
	}

	struct stat st;
	if(stat(argv[1], &st) != 0) {
		printf("'%s' stat info failed!\n", argv[1]);
		return -1;
	}

	int fd_from = open(argv[1], O_RDONLY);
	if(fd_from < 0) {
		printf("'%s' open failed!\n", argv[1]);
		return -1;
	}
	
	int fd_to = open(argv[2], O_WRONLY | O_CREAT);
	if(fd_to < 0) {
		printf("'%s' open failed!\n", argv[2]);
		close(fd_from);
		return -1;
	}

	while(1) {
		char buf[1024];
		int sz = read(fd_from, buf, 1024);
		if(sz > 0)
			sz = write(fd_to, buf, sz);
		if(sz <= 0)
			break;
	}

	fchmod(fd_to, st.st_mode);
	close(fd_to);
	close(fd_from);
	return 0;
}
