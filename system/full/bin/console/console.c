#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <ewoksys/proc.h>

int main(int argc, char** argv) {
	const char* outdev = "/dev/console0";
	const char* indev = "/dev/vkeyb";

	if(argc > 1) 
		outdev = argv[1];
	if(argc > 2) 
		indev = argv[2];
	
	int in_fd = open(indev, O_RDONLY);
	if(in_fd < 0) {
		printf("Error: %s open failed\n", indev);
		return -1;
	}

	int out_fd = open(outdev, O_WRONLY);
	if(out_fd < 0) {
		close(in_fd);
		printf("Error: %s open failed\n", outdev);
		return -1;
	}

	dup2(in_fd, 0);
	dup2(out_fd, 1);
	close(in_fd);
	close(out_fd);
	proc_exec("/bin/shell");
	return 0;
}
