#include <dev/devserv.h>
#include <kserv.h>
#include <unistd.h>
#include <ipc.h>
#include <stdlib.h>
#include <kstring.h>
#include <vfs/vfs.h>
#include <vfs/fs.h>
#include <proto.h>
#include <stdio.h>

static void doOpen(DeviceT* dev, PackageT* pkg) { 
	ProtoT* proto = protoNew(getPackageData(pkg), pkg->size);
	uint32_t node = (uint32_t)protoReadInt(proto);
	int32_t flags = protoReadInt(proto);
	protoFree(proto);

	if(node == 0) {
		ipcSend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	int32_t ret = 0;
	if(dev->open != NULL)
		ret = dev->open(node, flags);	
	ipcSend(pkg->id, pkg->type, &ret, 4);
}

static void doClose(DeviceT* dev, PackageT* pkg) { 
	uint32_t node = *(uint32_t*)getPackageData(pkg);
	if(node == 0) {
		ipcSend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	if(dev->close != NULL)
		dev->close(node);
	ipcSend(pkg->id, pkg->type, NULL, 0);
}

static void doDMA(DeviceT* dev, PackageT* pkg) { 
	uint32_t node = *(uint32_t*)getPackageData(pkg);

	if(node == 0) {
		ipcSend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	int32_t dma = 0;
	uint32_t size = 0;
	if(dev->dma != NULL) {
		dma = dev->dma(node, &size);
	}
		
	ProtoT* proto = protoNew(NULL, 0);
	protoAddInt(proto, dma);
	protoAddInt(proto, (int32_t)size);

	ipcSend(pkg->id, pkg->type, proto->data, proto->size);
	protoFree(proto);
}

static void doFlush(DeviceT* dev, PackageT* pkg) { 
	uint32_t node = *(uint32_t*)getPackageData(pkg);
	if(node == 0) {
		ipcSend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	if(dev->flush != NULL)
		dev->flush(node);
	ipcSend(pkg->id, pkg->type, NULL, 0);
}

static void doAdd(DeviceT* dev, PackageT* pkg) { 
	ProtoT* proto = protoNew(getPackageData(pkg), pkg->size);
	uint32_t node = (uint32_t)protoReadInt(proto);
	const char* name = protoReadStr(proto);
	protoFree(proto);

	if(node == 0 || name[0] == 0) {
		ipcSend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	int32_t ret = 0;
	if(dev->add != NULL)
		ret = dev->add(node, name);	
	ipcSend(pkg->id, pkg->type, &ret, 4);
}

static void doWrite(DeviceT* dev, PackageT* pkg) { 
	ProtoT* proto = protoNew(getPackageData(pkg), pkg->size);
	uint32_t node = (uint32_t)protoReadInt(proto);
	uint32_t size;
	void* p = protoRead(proto, &size);
	protoFree(proto);

	if(node == 0) {
		ipcSend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	int32_t ret = 0;
	if(dev->write != NULL)
		ret = dev->write(node, p, size);	

	ipcSend(pkg->id, pkg->type, &ret, 4);
}

static void doRead(DeviceT* dev, PackageT* pkg) { 
	ProtoT* proto = protoNew(getPackageData(pkg), pkg->size);
	uint32_t node = (uint32_t)protoReadInt(proto);
	uint32_t size = (uint32_t)protoReadInt(proto);
	uint32_t seek = (uint32_t)protoReadInt(proto);
	
	if(node == 0) {
		ipcSend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	if(size == 0) {
		ipcSend(pkg->id, pkg->type, NULL, 0);
		return;
	}

	char* buf = (char*)malloc(size);
	int32_t ret = 0;
	if(dev->read != NULL)
		ret = dev->read(node, buf, size, seek);	

	if(ret < 0)
		ipcSend(pkg->id, PKG_TYPE_ERR, NULL, 0);
	else
		ipcSend(pkg->id, pkg->type, buf, size);
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
		ipcSend(pkg->id, PKG_TYPE_ERR, NULL, 0);
		return;
	}

	p = NULL;
	int32_t ret = -1;
	if(dev->ctrl != NULL)
		p = dev->ctrl(node, cmd, p, size, &ret);	

	if(ret < 0)
		ipcSend(pkg->id, PKG_TYPE_ERR, NULL, 0);
	else
		ipcSend(pkg->id, pkg->type, p, ret);
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
		case FS_DMA:
			doDMA(dev, pkg);
			break;
		case FS_FLUSH:
			doFlush(dev, pkg);
			break;
		case FS_ADD:
			doAdd(dev, pkg);
			break;
	}
}

void devRun(DeviceT* dev, const char* devName, uint32_t index, const char* nodeName, bool file) {
	if(kservGetPid(devName) >= 0) {
		printf("%s device service has been running!\n", devName);
		exit(0);
	}

	uint32_t node = vfsMount(nodeName, devName, index, file);
	if(node == 0)
		return;
	
	if(dev->mount != NULL) {
		if(dev->mount(node, index) != 0)
			return;
	}
	printf("(%s mounted to vfs:%s)\n", devName, nodeName);
	kservRun(devName, handle, dev);
}
