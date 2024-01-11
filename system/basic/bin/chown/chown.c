#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ewoksys/vfs.h>

int main(int argc, char* argv[]) {
	const char* fname;
	if(argc < 3) {
		printf("Usage: chown <user> <fname>\n");
		return -1;
	}

	int uid = atoi(argv[1]);
	int gid = uid;
	fname = vfs_fullname(argv[2]);
	if(chown(fname, uid, gid) != 0) {
		printf("Can't chown [%s]!\n", fname);
		return -1;
	}
	return 0;
}
