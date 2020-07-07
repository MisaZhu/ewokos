#include <stdio.h>
#include <unistd.h>
#include <string.h>

int puts(const char *s) {
	int ret = strlen(s);
	return write(1, s, ret);
}

