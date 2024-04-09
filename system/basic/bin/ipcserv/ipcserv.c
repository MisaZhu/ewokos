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
	else 
		ipc_wait_ready(pid);
	return 0;
}

int main(int argc, char* argv[]) {

	struct stat buf;
	if(argc< 2)
		return -1;

	int ret = stat(argv[1], &buf);
	if(ret >= 0 && buf.st_mode & X_OK){
		str_t* cmd = str_new("");
		for(int i=1; i<argc; i++) {
			str_add(cmd, argv[i]);
			str_addc(cmd, ' ');
		}

		ret = run(cmd->cstr);
		str_free(cmd);
	}
	return ret;
}

