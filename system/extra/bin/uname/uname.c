#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/syscall.h>
#include <sysinfo.h>

int main(int argc, char* argv[]) {
	sysinfo_t sysinfo;
	syscall1(SYS_GET_SYSINFO, (int32_t)&sysinfo);

	if(argc == 1)
		printf("Ewok\n");
	else if(strcmp(argv[1], "-a") == 0)
		printf("Ewok micro-kernel OS, %s, V 0.01\n", sysinfo.machine);
	return 0;
}
