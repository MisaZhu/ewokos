#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <kstring.h>

int main(int argc, char* argv[]) {
	if(argc < 2)
		return -1;
	
	int fd = open(argv[1], O_RDWR|O_CREAT);
	if(fd < 0) {
		printf("error: open %s failed!\n", argv[1]);
		return -1;
	}

	const char* s = "hello world\n";
	write(fd, s, strlen(s));
	close(fd);
	return 0;
}

