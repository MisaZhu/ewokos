#include <stdio.h>

int setvbuf(FILE *stream, char *buf, int mode, size_t size) {
	(void)mode;
	(void)size;
	setbuf(stream, buf);
	return 0;
}
