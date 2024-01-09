#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/syscall.h>
#include <sysinfo.h>

static int sysinfo_read(int fd,
		int from_pid,
		uint32_t node,
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

#define STR_MAX 128
	if(size < STR_MAX)
		return -1;

	sys_info_t sysinfo;
	syscall1(SYS_GET_SYS_INFO, (int32_t)&sysinfo);
	char* str = (char*)buf;
	snprintf(str, STR_MAX-1, 
			"machine: %s\ncores: %d\nphy_mem_size: %d\nmmio_base: 0x%x\n", 
			sysinfo.machine, sysinfo.cores, sysinfo.phy_mem_size, sysinfo.mmio);
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
