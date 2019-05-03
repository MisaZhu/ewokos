#include <unistd.h>
#include <stdio.h>
#include <vfs/fs.h>
#include <stdlib.h>
#include <kstring.h>

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	printf("  MOUNT_POINT              DRIVER           DEV_ID  PID\n");
	uint32_t i = 0;
	while(true) {
		mount_t mnt;
		if(vfs_mount_by_index(i, &mnt) != 0)
			break;

		if(mnt.dev_serv_pid > 0) {
			char name[FULL_NAME_MAX];
			if(mnt.node_old == 0)
				strcpy(name, "/");
			else
				vfs_node_full_name(mnt.node_old, name, FULL_NAME_MAX);
			printf("  %24s %16s %6d  %6d\n", name, mnt.dev_name, mnt.dev_index, mnt.dev_serv_pid, name);
		}
		i++;
	}
	return 0;
}
