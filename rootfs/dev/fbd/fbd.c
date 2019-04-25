#include <dev/devserv.h>
#include <unistd.h>
#include <syscall.h>
#include <fbinfo.h>
#include <shm.h>
#include <device.h>

static fb_info_t _fb_info;
static int32_t _fb_buf_id = -1;
static int32_t _fb_bufSize = 0;
static void* _fb_buf = NULL;

static int32_t fb_mount(uint32_t node, int32_t index) {
	(void)node;
	(void)index;

	if(syscall2(SYSCALL_DEV_INFO, dev_typeid(DEV_FRAME_BUFFER, 0), (int32_t)&_fb_info) != 0)
		return -1;
	
	_fb_bufSize = 4 *_fb_info.width * _fb_info.height;
	if(_fb_bufSize == 0)
		return -1;

	_fb_buf_id = shm_alloc(_fb_bufSize);
	if(_fb_buf_id < 0)
		return -1;

	_fb_buf = shm_map(_fb_buf_id);
	return 0;
}

int32_t fb_write(uint32_t node, void* buf, uint32_t size, int32_t seek) {
	(void)node;
	(void)seek;
	return syscall3(SYSCALL_DEV_CHAR_WRITE, dev_typeid(DEV_FRAME_BUFFER, 0), (int32_t)buf, (int32_t)size);
}

int32_t fb_flush(uint32_t node) {
	(void)node;
	return syscall3(SYSCALL_DEV_CHAR_WRITE, dev_typeid(DEV_FRAME_BUFFER, 0), (int32_t)_fb_buf, (int32_t)_fb_bufSize);
}

int32_t fb_dma(uint32_t node, uint32_t *size) {
	(void)node;
	*size = _fb_bufSize;
	return _fb_buf_id;
}

void* fb_ctrl(uint32_t node, int32_t cmd, void* data, uint32_t size, int32_t* ret) {
	(void)node;
	(void)data;
	(void)size;
	void* p = NULL;

	if(cmd == 0) {//getfbinfo
		p = &_fb_info;
		*ret = sizeof(fb_info_t);
	}
	return p;
}

int main() {
	device_t dev = {0};
	dev.write = fb_write;
	dev.mount = fb_mount;
	dev.dma = fb_dma;
	dev.ctrl = fb_ctrl;
	dev.flush = fb_flush;

	dev_run(&dev, "dev.fb", 0, "/dev/fb0", true);
	shm_unmap(_fb_buf_id);
	return 0;
}
