#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/sys.h>
#include <ewoksys/syscall.h>
#include <sysinfo.h>

int main(int argc, char* argv[]) {
	sys_info_t sysinfo;
	sys_get_sys_info(&sysinfo);

	if(argc == 1)
		printf("Ewok\n");
	else if(strcmp(argv[1], "-a") == 0)
		printf("Ewok micro-kernel OS, %s, V 0.01\n", sysinfo.machine);
	return 0;
}
