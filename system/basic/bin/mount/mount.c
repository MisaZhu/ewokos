#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ewoksys/syscall.h>
#include <string.h>
#include <ewoksys/vfs.h>

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

	int32_t i;
	for(i=0; i<FS_MOUNT_MAX; i++) {
		mount_t mnt;
		if(vfs_get_mount_by_id(i, &mnt) != 0)
			continue;

		char cmd[128];
		syscall3(SYS_PROC_GET_CMD, (ewokos_addr_t)mnt.pid, (ewokos_addr_t)cmd, 127);
		printf("%-24s %-6d %s\n", 
				mnt.org_name,
				mnt.pid,
				get_cmd(cmd));
	}
	return 0;
}
