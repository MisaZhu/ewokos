#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

static int read_line(int fd, char* line, int sz) {
	int i = 0;
	while(i<sz) {
		char c;
		if(read(fd, &c, 1) <= 0) {
			line[i] = 0;
			return -1;
		}
		if(c == '\n') {
			line[i] = 0;
			break;
		}
		line[i++] = c;
	}
	return i;
}

static int welcome() {
	int fd = open("/etc/init/welcome", 0);
	if(fd < 0)
		return -1;

	char line[128];
	int i = 0;
	while(true) {
		i = read_line(fd, line, 128-1);
		if(i < 0)
			break;
		printf("%s\n", line);
	}
	close(fd);
	return 0;
}

int main() {
	welcome();
	usleep(100000);
	while(1) {
		int pid = fork();
		if(pid == 0) {
			exec("/sbin/login");
		}
		else {
			wait(pid);
		}
	}
	return 0;
}
