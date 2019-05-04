#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <kstring.h>

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;
	int fds[2];
	if(pipe(fds) != 0) {
		printf("error: open pipe failed!\n");
		return -1;
	}

	int pid = fork();
	if(pid == 0) {
		close(fds[0]);
		int counter = 0;
		while(counter < 100) {
			char s[128];
			snprintf(s, 127, "hello world from pipe. %d", counter);
			while(true) { //non-block
				int res = write(fds[1], s, strlen(s));
				if(res < 0 && errno == EAGAIN)
					continue;
				break;
			}
			counter++;
		}
		close(fds[1]);
	}
	else {
		close(fds[1]);
		char buf[128];
		int res;
		while(true) {
			while(true) { //non-block
				res = read(fds[0], buf, 128);
				if(res < 0 && errno == EAGAIN)
					continue;
				break;
			}
			if(res <= 0)
				break;
			buf[res] = 0;
			printf("[%s]\n", buf);
		}
		close(fds[0]);
	}
	return 0;
}

