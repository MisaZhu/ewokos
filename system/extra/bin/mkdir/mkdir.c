#include <unistd.h>
#include <sys/vfs.h>
#include <stdio.h>
#include <vprintf.h>

int main(int argc, char* argv[]) {
	if(argc < 2) {
		printf("Usage: mkdir <dir>");
		return -1;
	}

	const char* fname = vfs_fullname(argv[1]);
	fsinfo_t info;
	return vfs_create(fname, &info, FS_TYPE_DIR);
}
