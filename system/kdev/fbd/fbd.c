#include <dev.h>
#include <unistd.h>
#include <stdlib.h>
#include <syscall.h>
#include <fbinfo.h>
#include <kstring.h>

static int32_t getFBInfo(FBInfoT* info) {
	return syscall2(SYSCALL_MMIO_INFO, (int32_t)"framebuffer", (int32_t)info);
}

static FBInfoT _fbInfo;

int32_t fbMount(uint32_t node, int32_t index) {
	(void)node;
	(void)index;
	return getFBInfo(&_fbInfo);	
}

int32_t fbWrite(uint32_t node, void* buf, uint32_t size) {
	(void)node;

	uint32_t fbSize = _fbInfo.width * _fbInfo.height * (_fbInfo.depth / 8);
	if(size > fbSize)
		size = fbSize;
	
	if(syscall3(SYSCALL_MMIO_WRITE, _fbInfo.pointer, (int32_t)buf, (int32_t)size) == 0)
		return size;
	return -1;
}

int32_t fbRead(uint32_t node, void* buf, uint32_t size, uint32_t seek) {
	(void)node;
	(void)seek;

	uint32_t fbSize = _fbInfo.width * _fbInfo.height * (_fbInfo.depth / 8);
	if(size > fbSize)
		size = fbSize;
	
	if(syscall3(SYSCALL_MMIO_READ, _fbInfo.pointer, (int32_t)buf, (int32_t)size) == 0)
		return size;
	return -1;
}

void* fbCtrl(uint32_t node, int32_t cmd, void* data, uint32_t size, int32_t* ret) {
	(void)node;
	(void)data;
	(void)size;

	*ret = -1;
	if(cmd == 0) {//get fb info
		*ret = sizeof(FBInfoT);
		void* p = malloc(*ret);
		memcpy(p, &_fbInfo, *ret);
		return p;
	}
	return NULL;
}

void _start() {
	DeviceT dev = {0};
	dev.mount = fbMount;
	dev.write = fbWrite;
	dev.read = fbRead;
	dev.ctrl = fbCtrl;

	devRun(&dev, "dev.fb", 0, "/dev/fb0", true);
	exit(0);
}
