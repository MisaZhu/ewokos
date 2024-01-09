#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
	const char* fname;
	if(argc < 3) {
		printf("Usage: chmod <fname> <mode>\n");
		return -1;
	}

	fname = vfs_fullname(argv[1]);
	if(chmod(fname, atoi_base(argv[2], 8)) != 0) {
		printf("Can't chmod [%s]!\n", fname);
		return -1;
	}
	return 0;
}
