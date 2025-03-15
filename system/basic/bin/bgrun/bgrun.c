#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ewoksys/mstr.h>
#include <ewoksys/proc.h>
#include <ewoksys/ipc.h>
#include <unistd.h>
#include <sys/stat.h>

static int run(const char* cmd) {
	int pid = fork();
	if(pid == 0) {
		proc_detach();

		if(proc_exec(cmd) != 0) {
			exit(-1);
		}
	}
	return 0;
}

int main(int argc, char* argv[]) {

	struct stat buf;
	if(argc< 2)
		return -1;

	//printf("run: %-42s ", argv[1]);
	printf("run: %s ", argv[1]);
	int ret = stat(argv[1], &buf);
	if(ret >= 0 && buf.st_mode & X_OK){
		str_t* cmd = str_new("");
		for(int i=1; i<argc; i++) {
			str_add(cmd, argv[i]);
			str_addc(cmd, ' ');
		}

		ret = run(cmd->cstr);
		str_free(cmd);
		printf("[\033[32m%s\033[0m]\n", "OK"); //green for ok
	}
	else 
		printf("[\033[31m%s\033[0m]\n", "ERR!"); //red for failed
	return ret;
}

