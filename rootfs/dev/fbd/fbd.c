#include <dev/devserv.h>
#include <unistd.h>
#include <syscall.h>
#include <fbinfo.h>
#include <shm.h>

static FBInfoT _fbInfo;
static int32_t _fbBufID = -1;
static int32_t _fbBufSize = 0;
static void* _fbBuf = NULL;

static int32_t fbMount(uint32_t node, int32_t index) {
	(void)node;
	(void)index;

	if(syscall1(SYSCALL_FB_INFO, (int32_t)&_fbInfo) != 0)
		return -1;
	
	_fbBufSize = 4 *_fbInfo.width * _fbInfo.height;
	if(_fbBufSize == 0)
		return -1;

	_fbBufID = shmalloc(_fbBufSize);
	if(_fbBufID < 0)
		return -1;

	_fbBuf = shmMap(_fbBufID);
	return 0;
}

int32_t fbWrite(uint32_t node, void* buf, uint32_t size, int32_t seek) {
	(void)node;
	(void)seek;
	return syscall2(SYSCALL_FB_WRITE, (int32_t)buf, (int32_t)size);
}

int32_t fbFlush(uint32_t node) {
	(void)node;
	return syscall2(SYSCALL_FB_WRITE, (int32_t)_fbBuf, (int32_t)_fbBufSize);
}

int32_t fbDMA(uint32_t node, uint32_t *size) {
	(void)node;
	*size = _fbBufSize;
	return _fbBufID;
}

void* fbCtrl(uint32_t node, int32_t cmd, void* data, uint32_t size, int32_t* ret) {
	(void)node;
	(void)data;
	(void)size;
	void* p = NULL;

	if(cmd == 0) {//getfbinfo
		p = &_fbInfo;
		*ret = sizeof(FBInfoT);
	}
	return p;
}

void _start() {
	DeviceT dev = {0};
	dev.write = fbWrite;
	dev.mount = fbMount;
	dev.dma = fbDMA;
	dev.ctrl = fbCtrl;
	dev.flush = fbFlush;

	devRun(&dev, "dev.fb", 0, "/dev/fb0", true);
	shmUnmap(_fbBufID);
	exit(0);
}
