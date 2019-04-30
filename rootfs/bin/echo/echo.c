#include <unistd.h>
#include <stdio.h>

int main() {
	init_cmain_arg();
	const char* arg = read_cmain_arg();
	arg = read_cmain_arg();
	if(arg == NULL)
		return 0;
	printf("%s\n", arg);
	return 0;
}
