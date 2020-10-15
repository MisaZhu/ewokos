#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[]) {
	const char* p = argv[1];
	if(argc < 2)
		return -1;
	if(p[0] != '\\') {
		write(1, p, strlen(p));
	}
	else {
		p++;
		uint8_t i;
		if(p[0] == '0')
			i = atoi_base(p, 16);
		else
			i = atoi(p);
		write(1, &i, 1);
	}
	return 0;
}

