#include <unistd.h>
#include <stdio.h>
#include <syscall.h>

int main() {
	int32_t i=0;
	while(true) {
		char name[64], value[128];
		if(syscall3(SYSCALL_GET_ENV_NAME, i, (int32_t)name, 63) != 0 || name[0] == 0)
			break;
		if(syscall3(SYSCALL_GET_ENV_VALUE, i, (int32_t)value, 127) != 0)
			break;
		printf("%s=%s\n", name, value);
		i++;
	}
	return 0;
}
