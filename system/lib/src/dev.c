#include "dev.h"
#include <kstring.h>
#include <stdio.h>
#include <syscall.h>
#include <stdlib.h>
#include <kserv/fs.h>
#include <proto.h>

void devInit(DeviceT* dev, const char* type) {
	memset(dev, 0, sizeof(DeviceT));
	if(type != NULL)
		strncpy(dev->type, type, DEV_NAME_MAX);
}

const char* devGetServName(const char* deviceName) {
	static char ret[128];
	snprintf(ret, 127, "dev_%s", deviceName);
	return ret;
}

bool devMount(DeviceT* dev, uint32_t index, const char* nodeName) {
	if(syscall3(SYSCALL_VFS_MOUNT, (int32_t)nodeName, (int32_t)devGetServName(dev->type), index) == 0)
		return true;
	return false;
}
