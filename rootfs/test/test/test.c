#include <types.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
	int fd = open("/etc/init/welcome", O_RDONLY);
	char buf[6];
	lseek(fd, 6, SEEK_SET);
	read(fd, buf, 5);
	buf[5] = 0;
	printf("[%s]\n", buf);
	close(fd);
	return 0;
}
