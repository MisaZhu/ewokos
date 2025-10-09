#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/errno.h>
#include <string.h>
#include <fcntl.h>

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

static int _streaming = 0;

static int doargs(int argc, char* argv[]) {
	int c = 0;
	while (c != -1) {
		c = getopt (argc, argv, "s");
		if(c == -1)
			break;

		switch (c) {
		case 's':
			_streaming = 1;
			break;
		default:
			c = -1;
			break;
		}
	}
	return optind;
}

int main(int argc, char** argv) {
	const char* fname = "";
	int argind = doargs(argc, argv);
	if(argind < argc)
		fname = argv[argind];
	else {
		printf("  Usage: cat <file>\n");
		return -1;
	}

	int fd = open(fname, 0);
	if(fd < 0) {
		printf("Can't open [%s]!\n", argv[1]);
		return -1;
	}

	while(1) {
		char buf[1024];
		int sz = read(fd, buf, 1024);
		if(sz > 0) {
			out(buf, sz);
		}
		else {
			if(!_streaming && (sz == 0 || errno != EAGAIN))
				break;
		}
	}

	return 0;
}
