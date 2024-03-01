#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <ewoksys/proc.h>

int main(int argc, char** argv) {
	const char* dev = "/dev/console0";

	if(argc > 1) 
		dev = argv[1];
	
	int fd = open(dev, O_RDONLY);
	if(fd < 0) {
		printf("Error: %s open failed\n", dev);
		return -1;
	}

	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);
	close(fd);

	proc_exec("/bin/session");
	return 0;
}
