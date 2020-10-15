#include <unistd.h>
#include <stdio.h>
#include <sys/global.h>

int main(int argc, char* argv[]) {
	if(argc < 2) {
		printf("Usage: global key {value}\n");
		return -1;
	}

	if(argc == 2) {
		const char* v = get_global_str(argv[1]);
		printf("global: %s = %s\n", argv[1], v);
	}
	else {
		set_global_str(argv[1], argv[2]);
	}
	return 0;
}
