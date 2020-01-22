#include <sys/global.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
	int i;
	for(i=1; i<argc; i++) {
		printf("run '%s'.\nn", argv[i]);
		int pid = fork();
		if(pid == 0) {
			detach();
			exec(argv[i]);
		}
	}

	printf("switch to X.\n");
	set_global("current_console", "x");
	return 0;
}
