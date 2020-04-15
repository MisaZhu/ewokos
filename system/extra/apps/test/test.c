#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/ipc.h>

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	int i=0;
	while(1) {
		sleep(1);
		i++;
	}
  return 0;
}

