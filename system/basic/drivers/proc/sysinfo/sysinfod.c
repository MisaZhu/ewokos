#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/syscall.h>
#include <ewoksys/proc.h>
#include <sysinfo.h>

static int sysinfo_read(int fd,
		int from_pid,
		fsinfo_t* node,
		void* buf,
		int size,
		int offset,
		void* p) {

	(void)fd;
	(void)from_pid;
	(void)node;
	(void)offset;
	(void)p;

	if(offset > 0)
		return 0;

#define STR_MAX 256
	if(size < STR_MAX)
		return -1;

	sys_info_t sysinfo;
	syscall1(SYS_GET_SYS_INFO, (int32_t)&sysinfo);
	char* str = (char*)buf;
	snprintf(str, STR_MAX-1, 
			"machine: %s\n"
			"cores: %d\n"
			"phy_mem_size: %d\n"
			"max proc num: %d\n"
			"max thread per_proc: %d\n"
			"max files per_proc: %d\n"
			"mmio_base: 0x%x\n",
			sysinfo.machine,
			sysinfo.cores,
			sysinfo.phy_mem_size,
			MAX_PROC_NUM,
			MAX_THREAD_NUM_PER_PROC,
			MAX_OPEN_FILE_PER_PROC,
			sysinfo.mmio);
	return strlen(str);
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/proc/sysinfo";

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "sysinfo");
	dev.read = sysinfo_read;

	device_run(&dev, mnt_point, FS_TYPE_FILE, 0444);
	return 0;
}
