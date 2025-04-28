#include <unistd.h>
#include <ewoksys/vfs.h>
#include <stdio.h>
#include <sys/stat.h>

int main(int argc, char* argv[]) {
	const char* fname;
	fsinfo_t info;
	if(argc < 2) {
		printf("Usage: mkfifo <fname>");
		return -1;
	}

	fname = vfs_fullname(argv[1]);
	if(mkfifo(fname, 0664) != 0) {
		printf("mkfifo '%s' failed!\n", argv[1]);
		return -1;
	}
	return 0;
}
