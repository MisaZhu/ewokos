#include "dev.h"
#include <kstring.h>
#include <syscall.h>

void devInit(DeviceT* dev) {
	memset(dev, 0, sizeof(DeviceT));
}

bool devMount(uint32_t index, const char* nodeName) {
	if(syscall3(SYSCALL_VFS_MOUNT, (int32_t)nodeName, (int32_t)NULL, index) == 0)
		return true;
	return false;
}
