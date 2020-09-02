#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <sys/syscall.h>
#include <sys/vdevice.h>
#include <sys/shm.h>
#include <fbinfo.h>

typedef struct {
	void* data;
	uint32_t size;
	int32_t shm_id;
} fb_dma_t;

static fbinfo_t _fbinfo;

static inline void dup16(uint16_t* dst, uint32_t* src, int32_t w, int32_t h) {
	if(w <= 0 || h <= 0)
		return;

	register int32_t i, size;
	size = w * h;
	for(i=0; i < size; i++) {
		register uint32_t s = src[i];
		register uint8_t b = (s >> 16);
		register uint8_t g = (s >> 8);
		register uint8_t r = s;
		dst[i] = ((r >> 3) <<11) | ((g >> 3) << 6) | (b >> 3);
	}
}

static int fb_write(int fd,
		int from_pid, 
		fsinfo_t* info, 
		const void* buf,
		int size,
		int offset,
		void* p) {

	(void)fd;
	(void)from_pid;
	(void)info;
	(void)offset;
	(void)p;

	if(_fbinfo.pointer == 0)
		return 0;

	int32_t sz = (_fbinfo.depth/8) * _fbinfo.width * _fbinfo.height;
	if(size < sz)
		return 0;
	
	if(_fbinfo.depth == 32) {
		memcpy((void*)_fbinfo.pointer, buf, size);
	}
	else if(_fbinfo.depth == 16) 
		dup16((uint16_t*)_fbinfo.pointer, (uint32_t*)buf, _fbinfo.width, _fbinfo.height);
	return sz;
}	

static int fb_fcntl(int fd, 
		int from_pid,
		fsinfo_t* info, 
		int cmd, 
		proto_t* in, 
		proto_t* out,
		void* p) {

	(void)fd;
	(void)from_pid;
	(void)info;
	(void)in;
	(void)p;

	if(cmd == CNTL_INFO) {
		PF->addi(out, _fbinfo.width)->addi(out, _fbinfo.height);
	}
	return 0;
}

static int fb_flush(int fd, int from_pid, fsinfo_t* info, void* p) {
	(void)fd;
	(void)from_pid;
	(void)info;
	fb_dma_t* dma = (fb_dma_t*)p;

	if(_fbinfo.pointer == 0)
		return 0;

	uint32_t size = dma->size;
	uint32_t sz = (_fbinfo.depth/8) * _fbinfo.width * _fbinfo.height;
	if(size > sz)
		size = sz;

	if(_fbinfo.depth == 32) {
		memcpy((void*)_fbinfo.pointer, dma->data, size);
	}
	else if(_fbinfo.depth == 16) 
		dup16((uint16_t*)_fbinfo.pointer, (uint32_t*)dma->data, _fbinfo.width, _fbinfo.height);
	return 0;
}

static int fb_dma(int fd, int from_pid, fsinfo_t* info, int* size, void* p) {
	(void)fd;
	(void)from_pid;
	(void)info;
	fb_dma_t* dma = (fb_dma_t*)p;
	*size = dma->size;
	return dma->shm_id;
}

int main(int argc, char** argv) {
	const char* mnt_name = argc > 1 ? argv[1]: "/dev/fb0";

	if(syscall1(SYS_FRAMEBUFFER_MAP, (int32_t)&_fbinfo) != 0) {
		kprintf(false, " framebuffer mapping failed!\n");
		return -1;
	}

	uint32_t sz = _fbinfo.width * _fbinfo.height * 4;
	fb_dma_t dma;
	dma.shm_id = shm_alloc(sz, 1);
	if(dma.shm_id <= 0)
		return -1;
	dma.size = sz;
	dma.data = shm_map(dma.shm_id);
	if(dma.data == NULL)
		return -1;

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "framebuffer");
	dev.dma = fb_dma;
	dev.flush = fb_flush;
	dev.write = fb_write;
	dev.fcntl = fb_fcntl;

	dev.extra_data = &dma;
	device_run(&dev, mnt_name, FS_TYPE_CHAR);
	shm_unmap(dma.shm_id);
	return 0;
}
