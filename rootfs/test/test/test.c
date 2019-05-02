#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <kstring.h>

int main() {
	init_cmain_arg();
	const char* arg = read_cmain_arg();
	arg = read_cmain_arg();
	if(arg == NULL) {
		return -1;
	}
	
	int fd = open(arg, O_RDWR|O_CREAT);
	if(fd < 0) {
		printf("error: open %s failed!\n", arg);
		return -1;
	}

	const char* s = "hello world\n";
	write(fd, s, strlen(s));
	close(fd);
	return 0;
}

