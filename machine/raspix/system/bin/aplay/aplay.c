#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc, char** argv) {
	if(argc < 2) {
		slog("  Usage: aplay <file>\n");
		return -1;
	}

	struct stat st;
	if(stat(argv[1], &st) != 0) {
		slog("'%s' stat info failed!\n", argv[1]);
		return -1;
	}

	int fd_from = open(argv[1], O_RDONLY);
	if(fd_from < 0) {
		slog("'%s' open failed!\n", argv[1]);
		return -1;
	}

	const char* devname = "/dev/sound0";
	
	int fd_to = open(devname, O_WRONLY);
	if(fd_to < 0) {
		slog("'%s' open failed!\n", devname);
		close(fd_from);
		return -1;
	}

	uint32_t total = 0;
	while(1) {
		char buf[1024*4];
		int sz = read(fd_from, buf, sizeof(buf));
		if(sz > 0)
			sz = write(fd_to, buf, sz);
		if(sz <= 0)
			break;
		total += sz;
	}

	slog("sound played '%s' %d bytes\n", argv[1], total);

	close(fd_to);
	close(fd_from);
	return 0;
}
