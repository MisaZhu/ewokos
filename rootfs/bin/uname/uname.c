#include <stdio.h>
#include <unistd.h>
#include <kstring.h>

int main() {
	init_cmain_arg();
	const char* arg = read_cmain_arg();
	arg = read_cmain_arg();
	if(arg == NULL)
		printf("Ewok\n");
	else if(strcmp(arg, "-a") == 0)
		printf("Ewok micro-kernel OS, ARM A-Core, V 0.01\n");
	return 0;
}
