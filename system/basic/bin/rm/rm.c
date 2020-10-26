#include <unistd.h>
#include <stdio.h>
#include <sys/vfs.h>

int main(int argc, char* argv[]) {
	const char* fname;
	if(argc < 2) {
		printf("Usage: rm <fname>");
		return -1;
	}
	fname = vfs_fullname(argv[1]);
	return unlink(fname);
}
