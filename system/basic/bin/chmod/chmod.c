#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ewoksys/vfs.h>

int main(int argc, char* argv[]) {
	if(argc < 3) {
		printf("Usage: chmod <mode> <fname>\n");
		return -1;
	}

	char fullname[FS_FULL_NAME_MAX+1] = {0};
	vfs_fullname(argv[2], fullname, FS_FULL_NAME_MAX);
	if(chmod(fullname, strtoul(argv[1], NULL, 8)) != 0) {
		printf("Can't chmod [%s]!\n", fullname);
		return -1;
	}
	return 0;
}
