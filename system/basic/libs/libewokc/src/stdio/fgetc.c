#include <stdio.h>
#include <sys/errno.h>

int fgetc(FILE *fp) {
	char c = 0;
	if(fread( &c, 1, 1, fp) != 1)
		c = 0;
	return c;
}
