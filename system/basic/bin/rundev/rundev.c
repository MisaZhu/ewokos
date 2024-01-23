#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ewoksys/mstr.h>
#include <ewoksys/proc.h>
#include <ewoksys/ipc.h>
#include <unistd.h>

static int run(const char* cmd) {
	int pid = fork();
	if(pid == 0) {
		proc_detach();

		if(execve(cmd, "", "") != 0) {
			exit(-1);
		}
	}
	else 
		ipc_wait_ready(pid);
	return 0;
}

int main(int argc, char* argv[]) {
	str_t* cmd = str_new("");
	for(int i=1; i<argc; i++) {
		str_add(cmd, argv[i]);
		str_addc(cmd, ' ');
	}
	int ret = run(cmd->cstr);
	str_free(cmd);
	return ret;
}

