#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

void fclose(FILE* fp) {
	if(fp == NULL)
		return;
	close(fp->fd);
	free(fp);
}

