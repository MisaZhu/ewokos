#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ewoksys/core.h>

int main(int argc, char* argv[]) {
	if(argc < 2) {
		printf("Usage: setux <0~9>\n");
		return -1;
	}

	core_set_active_ux(atoi(argv[1]));
	return 0;
}
