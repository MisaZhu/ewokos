#include <dev/devserv.h>
#include <unistd.h>
#include <syscall.h>
#include <fbinfo.h>
#include <shm.h>
#include <device.h>
#include <vfs/fs.h>

static fb_info_t _fb_info;
static int32_t _fb_buf_id = -1;
static int32_t _fb_bufSize = 0;
static void* _fb_buf = NULL;

static int32_t fb_mount(const char* fname, int32_t index) {
	(void)fname;
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

static int32_t fb_write(int32_t pid, int32_t fd, void* buf, uint32_t size, int32_t seek) {
	(void)pid;
	(void)fd;
	(void)seek;
	return syscall3(SYSCALL_DEV_CHAR_WRITE, dev_typeid(DEV_FRAME_BUFFER, 0), (int32_t)buf, (int32_t)size);
}

static int32_t fb_flush(int32_t pid, int32_t fd) {
	(void)pid;
	(void)fd;
	return syscall3(SYSCALL_DEV_CHAR_WRITE, dev_typeid(DEV_FRAME_BUFFER, 0), (int32_t)_fb_buf, (int32_t)_fb_bufSize);
}

static int32_t fb_dma(int32_t pid, int32_t fd, uint32_t *size) {
	(void)pid;
	(void)fd;
	*size = _fb_bufSize;
	return _fb_buf_id;
}

static int32_t fb_ctrl(int32_t pid, int32_t fd, int32_t cmd, proto_t* input, proto_t* out) {
	(void)pid;
	(void)fd;
	(void)input;

	if(cmd == FS_CTRL_INFO) {//getfbinfo
		proto_add(out, &_fb_info, sizeof(fb_info_t));
	}
	return 0;
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	device_t dev = {0};
	dev.write = fb_write;
	dev.mount = fb_mount;
	dev.dma = fb_dma;
	dev.ctrl = fb_ctrl;
	dev.flush = fb_flush;

	dev_run(&dev, argc, argv);
	shm_unmap(_fb_buf_id);
	return 0;
}
