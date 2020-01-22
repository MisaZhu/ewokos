#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <string.h>
#include <fsinfo.h>

static const char* get_cmd(char* cmd) {
	char* p = cmd;
	while(*p != 0) {
		if(*p == ' ') {
			*p = 0;
			break;
		}
		p++;
	}
	return cmd;
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	printf("  MOUNT_POINT              PID    DRIVER\n");
	int32_t i;
	for(i=0; i<FS_MOUNT_MAX; i++) {
		mount_t mnt;
		if(syscall2(SYS_VFS_GET_MOUNT_BY_ID, i, (int32_t)&mnt) != 0)
			continue;

		char cmd[128];
		syscall3(SYS_PROC_GET_CMD, mnt.pid, (int32_t)cmd, 127);
		printf("  %24s %6d %s\n", 
				mnt.org_name,
				mnt.pid,
				get_cmd(cmd));
	}
	return 0;
}
