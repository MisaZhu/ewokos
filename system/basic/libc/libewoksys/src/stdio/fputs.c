#include <stdio.h>
#include <string.h>

int fputs(const char *s, FILE *stream) {
	size_t len = strlen(s);
	size_t written = fwrite(s, 1, len, stream);
	return written == len ? 0 : EOF;
}
