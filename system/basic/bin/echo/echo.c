#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[]) {
	if(argc < 2)
		return -1;
	
	setbuf(stdout, NULL);

	if(strcmp(argv[1], "-e") == 0) {
		if(argc < 3)
			return -1;
		const char* p = argv[2];
		printf("\033[%s", p);
	}
	else
		printf("%s", argv[1]);

	return 0;
}
