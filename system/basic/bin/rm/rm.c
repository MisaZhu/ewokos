#include <unistd.h>
#include <stdio.h>
#include <ewoksys/vfs.h>

int main(int argc, char* argv[]) {
	const char* fname;
	if(argc < 2) {
		printf("Usage: rm <fname>");
		return -1;
	}
	fname = vfs_fullname(argv[1]);
	if(unlink(fname) != 0) {
		printf("Can't remove [%s]!\n", fname);
		return -1;
	}
	return 0;
}
