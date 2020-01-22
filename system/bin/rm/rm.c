#include <unistd.h>
#include <stdio.h>
#include <sys/vfs.h>

int main(int argc, char* argv[]) {
	if(argc < 2) {
		printf("Usage: rm <fname>");
		return -1;
	}
	const char* fname = vfs_fullname(argv[1]);
	return unlink(fname);
}
