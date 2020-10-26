#include <unistd.h>
#include <sys/vfs.h>
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
	if(vfs_create(fname, &info, FS_TYPE_DIR, false) != 0) {
		fprintf(stderr, "mkdir '%s' failed!\n", fname);
		return -1;
	}
	return 0;
}
