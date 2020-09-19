#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[]) {
	if(argc < 2)
		return -1;
	const char* p = argv[1];
	if(p[0] != '\\') {
		write(1, p, strlen(p));
	}
	else {
		uint8_t i = atoi(p+1);
		write(1, &i, 1);
	}
	return 0;
}

