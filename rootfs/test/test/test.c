#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <syscall.h>
#include <devices.h>

int main() {
	printf("read keyboard: ");
	while(true) {
		char buf[9];
		int32_t res = syscall3(SYSCALL_DEV_READ, dev_typeid(DEV_KEYBOARD, 0), (int32_t)buf, 3);
		if(res > 0) {
			buf[res] = 0;
			printf("%s", buf);
			if(buf[res-1] == '\r')
				break;
		}
	}
	return 0;
}

