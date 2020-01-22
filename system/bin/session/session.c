#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

static void welcome(void) {
	const char* s;
	if(strcmp(getenv("CONSOLE"), "console") == 0)  {
		s = "\n"
			"+-----Ewok micro-kernel OS-----------------------+\n"
			"| https://github.com/MisaZhu/micro_kernel_os.git |\n"
			"+------------------------------------------------+\n"
			"(CTRL + TAB to switch between X and console)\n\n";
	}
	else {
		s = "\n"
			"+-----Ewok micro-kernel OS-----------------------+\n"
			"| https://github.com/MisaZhu/micro_kernel_os.git |\n"
			"+------------------------------------------------+\n\n";
	}
	printf("%s", s);
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	while(1) {
		welcome();
		int pid = fork();
		if(pid == 0) {
			exec("/bin/shell");
		}
		else {
			waitpid(pid);
		}
	}
	return 0;
}
