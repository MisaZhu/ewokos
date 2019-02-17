#include "dev.h"
#include <kstring.h>
#include <stdio.h>
#include <syscall.h>
#include <stdlib.h>
#include <kserv/fs.h>
#include <package.h>

void devInit(DeviceT* dev) {
	memset(dev, 0, sizeof(DeviceT));
}

const char* devGetServName(DeviceT* dev) {
	static char ret[128];
	snprintf(ret, 127, "DEV_%s", dev->name);
	return ret;
}

static int _fsPid = -1;

#define CHECK_KSERV_FS \
	if(_fsPid < 0) \
		_fsPid = syscall1(SYSCALL_KSERV_GET, (int)KSERV_FS_NAME); \
	if(_fsPid < 0) \
		return false; 

bool devMount(DeviceT* dev, const char* dstPath, const char* nodeName) {
	CHECK_KSERV_FS
	const char* serv = devGetServName(dev);
	uint32_t servLen = strlen(serv);
	uint32_t pathLen = strlen(dstPath);
	uint32_t nameLen = strlen(nodeName);

	uint32_t sz = servLen+pathLen+nameLen+12;
	char *req = (char*)malloc(sz);
	const char* p = req;

	memcpy(req, &servLen, 4);
	req += 4;
	memcpy(req, serv, servLen);
	req += servLen;
	memcpy(req, &pathLen, 4);
	req += 4;
	memcpy(req, dstPath, pathLen);
	req += pathLen;
	memcpy(req, &nameLen, 4);
	req += 4;
	memcpy(req, nodeName, nameLen);

	PackageT* pkg = preq(_fsPid, FS_MOUNT, p, sz, true);
	free(req);

	if(pkg == NULL)
		return false;

	if(pkg->type == PKG_TYPE_ERR) {
		free(pkg);
		return false; 
	}
	return true;
}
