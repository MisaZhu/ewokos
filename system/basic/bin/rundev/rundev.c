#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/mstr.h>
#include <sys/proc.h>
#include <unistd.h>

static int run(const char* cmd) {
	int pid = fork();
	if(pid == 0) {
		proc_detach();

		if(exec(cmd) != 0) {
			exit(-1);
		}
	}
	else 
		proc_wait_ready(pid);
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

