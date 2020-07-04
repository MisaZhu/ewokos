#include <stdio.h>
#include <unistd.h>

int fseek(FILE* fp, int offset, int whence) {
	if(fp == NULL)
		return -1;
	return lseek(fp->fd, offset, whence);
}

