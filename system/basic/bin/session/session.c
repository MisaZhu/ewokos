#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ewoksys/wait.h>

static void welcome(void) {
	const char* s = "\n"
			"+-----Ewok micro-kernel OS-----------------------+\n"
			"| https://github.com/MisaZhu/EwokOS.git          |\n"
			"+------------------------------------------------+\n";
	printf("%s", s);
}

int main(int argc, char* argv[]) {
	const char* tty = "/dev/tty0";
	if(argc > 1)
		tty = argv[1];

	int fd = open(tty, O_RDWR);
	if(fd < 0) {
		klog("Can't open tty device(%s)!\n", tty);
		return -1;
	}

	setenv("CONSOLE_ID", tty);
	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);
	close(fd);

	while(1) {
		int pid = fork();
		if(pid == 0) {
			if(proc_exec("/bin/tsaver") < 0) {
				exit(-1);
			}
		}
		else {
			waitpid(pid);
		}

		pid = fork();
		if(pid == 0) {
			welcome();
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
