#include <stdio.h>
#include <sys/vfs.h>
#include <unistd.h>

int ftell(FILE* fp) {
	return vfs_tell(fp->fd);
}

