#include <unistd.h>
#include <stdio.h>
#include <vfs/fs.h>
#include <stdlib.h>
#include <kstring.h>

int main(int argc, char* argv[]) {
	char name[FULL_NAME_MAX];
	if(argc < 2) {
		printf("usage: rm <fname>.\n");
		return -1;
	}
	else {
		fs_full_name(argv[1], name, FULL_NAME_MAX);
	}

	int res = fs_remove(name);
	if(res != 0) {
		printf("'%s' remove failed!\n", name);
	}
	return 0;
}
