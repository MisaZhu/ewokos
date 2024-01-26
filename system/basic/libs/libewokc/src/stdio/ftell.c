#include <stdio.h>
#include <ewoksys/vfs.h>
#include <unistd.h>

int ftell(FILE* fp) {
	return vfs_tell(fp->fd);
}

