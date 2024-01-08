#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/syscall.h>
#include <sysinfo.h>

static int sys_state_read(int fd,
		int from_pid,
		fsinfo_t* info,
		void* buf,
		int size,
		int offset,
		void* p) {

	(void)fd;
	(void)from_pid;
	(void)info;
	(void)offset;
	(void)p;

	if(offset > 0)
		return 0;

#define STR_MAX 128
	if(size < STR_MAX)
		return -1;

	sys_state_t state;
	syscall1(SYS_GET_SYS_STATE, (int32_t)&state);
	snprintf((char*)buf, STR_MAX-1, 
			"run_sec: %d\nfree_mem: %d\nshared_mem: %d\n", 
			state.kernel_sec, state.mem.free, state.mem.shared);
	return strlen((char*)buf);
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/proc/state";

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "state");
	dev.read = sys_state_read;

	device_run(&dev, mnt_point, FS_TYPE_FILE, 0444);
	return 0;
}
