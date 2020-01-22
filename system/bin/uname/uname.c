#include <stdio.h>
#include <unistd.h>
#include <kstring.h>

int main(int argc, char* argv[]) {
	if(argc == 1)
		printf("Ewok\n");
	else if(strcmp(argv[1], "-a") == 0)
		printf("Ewok micro-kernel OS, ARM A-Core, V 0.01\n");
	return 0;
}
