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
			tstr_t* name;
			if(mnt.node_old == 0)
				name = tstr_new("/", malloc, free);
			else
				name = vfs_full_name_by_node(mnt.node_old);
			if(name != NULL) {
				printf("  %24s %16s %6d  %6d\n", CS(name), mnt.dev_name, mnt.dev_index, mnt.dev_serv_pid);
				tstr_free(name);
			}
		}
		i++;
	}
	return 0;
}
