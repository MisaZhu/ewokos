#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

void do_sig(int sig_no) {
	printf("signal: %d\n", sig_no);
	//exit(0);
}

int main(int argc, char* argv[]) {
	signal(SIGSTOP, do_sig);
	int i = 0;	
	while(1) {
		sleep(1);
	}
	return 0;
}
