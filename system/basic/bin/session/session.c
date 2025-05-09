#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ewoksys/wait.h>
#include <ewoksys/vfs.h>
#include <ewoksys/core.h>
#include <setenv.h>

static void welcome(void) {
	const char* s = "\033[2J\033[0;0H" //clear screen and move to 0,0
			"+-----Ewok micro-kernel OS-----------------------+\n"
			"| https://github.com/MisaZhu/EwokOS.git          |\n"
			"+------------------------------------------------+\n";
	printf("%s", s);
}

static const char* _tty = "";
static int _auto_restart = 0;

static int doargs(int argc, char* argv[]) {
	_tty = "";

	int c = 0;
	while (c != -1) {
		c = getopt (argc, argv, "rt:");
		if(c == -1)
			break;

		switch (c) {
		case 'r':
			_auto_restart = 1;
			break;
		case 't':
			_tty = optarg;
			break;
		case '?':
			return -1;
		default:
			c = -1;
			break;
		}
	}
	return optind;
}

int main(int argc, char* argv[]) {
	int argind = doargs(argc, argv);
	if(argind < 0)
		return -1;

	if(_tty[0] != 0) {
		int fd = open(_tty, O_RDWR);
		if(fd < 0) {
			return -1;
		}

		setenv("CONSOLE_ID", _tty);
		dup2(fd, 0);
		dup2(fd, 1);
		dup2(fd, 2);
		dup2(fd, VFS_BACKUP_FD0);
		dup2(fd, VFS_BACKUP_FD1);
		close(fd);
	}

	do {
		int pid;
		for(int i=argind; i<argc; i++) {
			pid = fork();
			if(pid == 0) {
				if(proc_exec(argv[i]) < 0) {
					exit(-1);
				}
			}
			else {
				waitpid(pid);
			}
		}
		
		pid = fork();
		if(pid == 0) {
			welcome();
			fflush(stdout);
			if(proc_exec("/bin/login") < 0) {
				exit(-1);
			}
		}
		else {
			waitpid(pid);
		}
	}while(_auto_restart);
	return 0;
}
