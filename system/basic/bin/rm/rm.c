#include <unistd.h>
#include <stdio.h>
#include <ewoksys/vfs.h>

int main(int argc, char* argv[]) {
	if(argc < 2) {
		printf("Usage: rm <fname>");
		return -1;
	}

	char fullname[FS_FULL_NAME_MAX+1] = {0};
	vfs_fullname(argv[1], fullname, FS_FULL_NAME_MAX);
	if(unlink(fullname) != 0) {
		printf("Can't remove [%s]!\n", fullname);
		return -1;
	}
	return 0;
}
