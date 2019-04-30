#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <syscall.h>
#include <device.h>

int main() {
	int i = 0;
	while(true) {
		printf("%d ", i++);
		usleep(1000000);
	}
	return 0;
}

