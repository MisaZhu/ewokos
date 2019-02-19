#include "dev.h"
#include <kstring.h>
#include <stdio.h>
#include <syscall.h>
#include <stdlib.h>
#include <kserv/kserv.h>
#include <kserv/fs.h>
#include <pmessage.h>
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

bool devMount(DeviceT* dev, uint32_t index, const char* dstPath, const char* nodeName) {
	int pid = kservGetPid(KSERV_VFS_NAME);
	if(pid < 0)
		return false;

	ProtoT proto;
	protoInit(&proto, NULL, 0);
	protoAddStr(&proto, dev->type);
	protoAddInt(&proto, index);
	protoAddStr(&proto, dstPath);
	protoAddStr(&proto, nodeName);

	PackageT* pkg = preq(pid, FS_MOUNT, proto.data, proto.size, true);
	protoFree(&proto);

	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return false; 
	}	
	free(pkg);
	return true;
}
