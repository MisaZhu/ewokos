#include "dev.h"
#include <kstring.h>
#include <syscall.h>

void devInit(DeviceT* dev) {
	memset(dev, 0, sizeof(DeviceT));
}

bool devMount(DeviceT* dev, uint32_t index, const char* nodeName, bool isFile) {
	(void)dev;
	int32_t ret = 0;
	/*
	if(isFile)
		ret = syscall3(SYSCALL_VFS_MOUNT_FILE, (int32_t)nodeName, (int32_t)NULL, index);
	else
		ret = syscall3(SYSCALL_VFS_MOUNT_DIR, (int32_t)nodeName, (int32_t)NULL, index);
		*/

	if(ret == 0)
		return true;
	return false;
}
