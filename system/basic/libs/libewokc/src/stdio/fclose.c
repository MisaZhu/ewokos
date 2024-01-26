#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

int fclose(FILE* fp) {
	if(fp == NULL)
		return -1;
	close(fp->fd);
	free(fp);
	return 0;
}

