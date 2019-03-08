#include <dev/devserv.h>
#include <syscall.h>
#include <unistd.h>
#include <pmessage.h>
#include <stdio.h>
#include <stdlib.h>
#include <kstring.h>
#include <kserv.h>
#include <vfs/fs.h>
#include <vfs/vfs.h>
#include <proto.h>

static void doOpen(DeviceT* dev, PackageT* pkg) { 
	ProtoT* proto = protoNew(getPackageData(pkg), pkg->size);
	uint32_t node = (uint32_t)protoReadInt(proto);
	int32_t flags = protoReadInt(proto);
	protoFree(proto);

	if(node == 0) {
		psend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	int32_t ret = 0;
	if(dev->open != NULL)
		ret = dev->open(node, flags);	
	psend(pkg->id, pkg->type, &ret, 4);
}

static void doClose(DeviceT* dev, PackageT* pkg) { 
	uint32_t node = *(uint32_t*)getPackageData(pkg);
	if(node == 0)
		return;

	if(dev->close != NULL)
		dev->close(node);
}

static void doAdd(DeviceT* dev, PackageT* pkg) { 
	ProtoT* proto = protoNew(getPackageData(pkg), pkg->size);
	uint32_t node = (uint32_t)protoReadInt(proto);
	const char* name = protoReadStr(proto);
	protoFree(proto);

	if(node == 0 || name[0] == 0) {
		psend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	int32_t ret = 0;
	if(dev->add != NULL)
		ret = dev->add(node, name);	
	psend(pkg->id, pkg->type, &ret, 4);
}

static void doWrite(DeviceT* dev, PackageT* pkg) { 
	ProtoT* proto = protoNew(getPackageData(pkg), pkg->size);
	uint32_t node = (uint32_t)protoReadInt(proto);
	uint32_t size;
	void* p = protoRead(proto, &size);
	protoFree(proto);

	if(node == 0) {
		psend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	int32_t ret = 0;
	if(dev->write != NULL)
		ret = dev->write(node, p, size);	

	psend(pkg->id, pkg->type, &ret, 4);
}

static void doRead(DeviceT* dev, PackageT* pkg) { 
	ProtoT* proto = protoNew(getPackageData(pkg), pkg->size);
	uint32_t node = (uint32_t)protoReadInt(proto);
	uint32_t size = (uint32_t)protoReadInt(proto);
	uint32_t seek = (uint32_t)protoReadInt(proto);
	
	if(node == 0) {
		psend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	if(size == 0) {
		psend(pkg->id, pkg->type, NULL, 0);
		return;
	}

	char* buf = (char*)malloc(size);
	int32_t ret = 0;
	if(dev->read != NULL)
		ret = dev->read(node, buf, size, seek);	

	if(ret < 0)
		psend(pkg->id, PKG_TYPE_ERR, NULL, 0);
	else
		psend(pkg->id, pkg->type, buf, size);
	free(buf);
}

static void doCtrl(DeviceT* dev, PackageT* pkg) { 
	ProtoT* proto = protoNew(getPackageData(pkg), pkg->size);
	uint32_t node = (uint32_t)protoReadInt(proto);
	uint32_t cmd = (uint32_t)protoReadInt(proto);
	uint32_t size;
	void* p = protoRead(proto, &size);
	protoFree(proto);

	if(node == 0) {
		psend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	p = NULL;
	int32_t ret = 0;
	if(dev->ctrl != NULL)
		p = dev->ctrl(node, cmd, p, size, &ret);	

	if(ret < 0)
		psend(pkg->id, PKG_TYPE_ERR, NULL, 0);
	else
		psend(pkg->id, pkg->type, p, ret);

	if(p != NULL)
		free(p);
}

static void handle(PackageT* pkg, void* p) {
	DeviceT* dev = (DeviceT*)p;
	switch(pkg->type) {
		case FS_OPEN:
			doOpen(dev, pkg);
			break;
		case FS_CLOSE:
			doClose(dev, pkg);
			break;
		case FS_WRITE:
			doWrite(dev, pkg);
			break;
		case FS_READ:
			doRead(dev, pkg);
			break;
		case FS_CTRL:
			doCtrl(dev, pkg);
			break;
		case FS_ADD:
			doAdd(dev, pkg);
			break;
	}
}

static uint32_t devMount(const char* devName, uint32_t index, const char* nodeName, bool file) {
	if(file)
		return (uint32_t)syscall3(SYSCALL_VFS_MOUNT_FILE, (int32_t)nodeName, (int32_t)devName, index);
	return (uint32_t)syscall3(SYSCALL_VFS_MOUNT, (int32_t)nodeName, (int32_t)devName, index);
}

void devRun(DeviceT* dev, const char* devName, uint32_t index, const char* nodeName, bool file) {
	if(kservGetPid(devName) >= 0) {
    printf("Panic: '%s' process has been running already!\n", devName);
		exit(0);
	}

	uint32_t node = devMount(devName, index, nodeName, file);
	if(node == 0)
		return;
	
	if(dev->mount != NULL) {
		if(dev->mount(node, index) != 0)
			return;
	}
	kservRun(devName, handle, dev);
}
