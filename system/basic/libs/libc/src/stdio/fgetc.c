#include <stdio.h>
#include <errno.h>

int fgetc(FILE *fp)
{
	char c = 0;
	fread( &c, 1, 1, fp);;
	return c;
}
