#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

void setbuf(FILE *fp, char *buf) {
	(void)fp;
	(void)buf;
}