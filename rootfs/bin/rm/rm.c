#include <unistd.h>
#include <stdio.h>
#include <vfs/fs.h>
#include <stdlib.h>
#include <kstring.h>

int main(int argc, char* argv[]) {
	if(argc < 2) {
		printf("usage: rm <fname>.\n");
		return -1;
	}

	tstr_t* name = fs_full_name(argv[1]);
	int res = fs_remove(CS(name));
	if(res != 0) {
		printf("'%s' remove failed!\n", CS(name));
	}
	tstr_free(name);
	return 0;
}
