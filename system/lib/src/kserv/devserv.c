#include "kserv/devserv.h"
#include "dev.h"
#include <stdio.h>

/*
void doMount(PackageT* pkg, DeviceT* dev) {
}

void doUnmount(PackageT* pkg, DeviceT* dev) {
}

void doOpen(PackageT* pkg, DeviceT* dev) {
}

void doClose(PackageT* pkg, DeviceT* dev) {
}

void doWrite(PackageT* pkg, DeviceT* dev) {
}

void doRead(PackageT* pkg, DeviceT* dev) {
}

void doInfo(PackageT* pkg, DeviceT* dev) {
}

void doSetopt(PackageT* pkg, DeviceT* dev) {
}
*/

void handle(PackageT* pkg, void* p) {
	DeviceT* dev = (DeviceT*)p;

  switch(pkg->type) {
	case DEV_MOUNT:
		//doMount(pkg, dev);
		break;
	case DEV_UNMOUNT:
		//doUnmount(pkg, dev);
		break;
	case DEV_OPEN:
		//doOpen(pkg, dev);
		break;
	case DEV_CLOSE:
		//doClose(pkg, dev);
		break;
	case DEV_WRITE:
		//doWrite(pkg, dev);
		break;
	case DEV_READ:
		//doRead(pkg, dev);
		break;
	case DEV_INFO:
		//doInfo(pkg, dev);
		break;
	case DEV_SETOPT:
		//doSetopt(pkg, dev);
		break;
	}
}

bool devServRun(DeviceT *dev) {
	return kservRun(devGetServName(dev->type), handle, dev);
}
