#include <unistd.h>
#include <ewoksys/vfs.h>
#include <stdio.h>
#include <sys/stat.h>


int main(int argc, char* argv[]) {
	fsinfo_t info;
	if(argc < 2) {
		printf("Usage: mkdir <dir>");
		return -1;
	}

	char fullname[FS_FULL_NAME_MAX+1] = {0};
	vfs_fullname(argv[1], fullname, FS_FULL_NAME_MAX);
	if(mkdir(fullname, 0751) != 0) {
		printf("mkdir '%s' failed!\n", fullname);
		return -1;
	}
	return 0;
}
