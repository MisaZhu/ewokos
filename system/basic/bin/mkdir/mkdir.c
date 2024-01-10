#include <unistd.h>
#include <ewoksys/vfs.h>
#include <stdio.h>
#include <vprintf.h>

int main(int argc, char* argv[]) {
	const char* fname;
	fsinfo_t info;
	if(argc < 2) {
		printf("Usage: mkdir <dir>");
		return -1;
	}

	fname = vfs_fullname(argv[1]);
	if(vfs_create(fname, &info, FS_TYPE_DIR, 0751, false, true) != 0) {
		printf("mkdir '%s' failed!\n", argv[1]);
		return -1;
	}
	return 0;
}
