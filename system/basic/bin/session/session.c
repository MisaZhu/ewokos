#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ewoksys/wait.h>

static void welcome(void) {
	const char* s = "\n"
			"+-----Ewok micro-kernel OS-----------------------+\n"
			"| https://github.com/MisaZhu/EwokOS.git          |\n"
			"+------------------------------------------------+\n";
	printf("%s", s);
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	while(1) {
		welcome();
		int pid = fork();
		if(pid == 0) {
			if(proc_exec("/bin/login") < 0) {
				exit(-1);
			}
		}
		else {
			waitpid(pid);
		}
	}
	return 0;
}
