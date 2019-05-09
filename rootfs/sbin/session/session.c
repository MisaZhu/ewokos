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

static int32_t init_stdio(const char* dev) {
	int fd = -1;
	while(true) {
		fd = open(dev, O_RDONLY);
		if(fd >= 0)
			break;
		usleep(100000);
	}
	
	dup2(fd, 0);
	dup2(fd, 1);
	return 0;
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	init_stdio(getenv("STDIO_DEV"));
	welcome();
	usleep(100000);
	while(1) {
		int pid = fork();
		if(pid == 0) {
			init_stdio(getenv("STDIO_DEV"));
			exec("/sbin/login");
		}
		else {
			wait(pid);
		}
	}
	return 0;
}
