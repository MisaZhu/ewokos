#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ewoksys/vfs.h>

int main(int argc, char* argv[]) {
	const char* fname;
	if(argc < 3) {
		printf("Usage: chmod <mode> <fname>\n");
		return -1;
	}

	fname = vfs_fullname(argv[2]);
	if(chmod(fname, strtoul(argv[1], NULL, 8)) != 0) {
		printf("Can't chmod [%s]!\n", fname);
		return -1;
	}
	return 0;
}
