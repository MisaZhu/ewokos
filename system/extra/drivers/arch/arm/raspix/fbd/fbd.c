#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <sys/syscall.h>
#include <sys/vdevice.h>
#include <sys/shm.h>
#include <arch/arm/bcm283x/framebuffer.h>

typedef struct {
	void* data;
	uint32_t size;
	int32_t shm_id;
} fb_dma_t;

static fbinfo_t* _fbinfo;

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

	if(cmd == 0) { //get fb size
		PF->addi(out, _fbinfo->width)->addi(out, _fbinfo->height)->addi(out, _fbinfo->depth);
	}
	return 0;
}

static int fb_dma_reset(fb_dma_t* dma) {
	if(dma->shm_id > 0) {
		shm_unmap(dma->shm_id);
		dma->shm_id = 0;
	}
	if(_fbinfo->pointer == 0)
		return -1;

  uint32_t size = 4 * _fbinfo->width * _fbinfo->height;
	dma->shm_id = shm_alloc(size, 1);
	if(dma->shm_id <= 0)
		return -1;
	dma->size = size;
	dma->data = shm_map(dma->shm_id);
	if(dma->data == NULL)
		return -1;
	return 0;
}

static int fb_dev_cntl(int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
	(void)from_pid;
	(void)ret;
	fb_dma_t* dma = (fb_dma_t*)p;

	if(cmd == 0) { //set fb size and bpp
		int w = proto_read_int(in);
		int h = proto_read_int(in);
		int bpp = proto_read_int(in);
		if(bcm283x_fb_init(w, h, bpp) != 0)
			return -1;
		_fbinfo = bcm283x_get_fbinfo();
		fb_dma_reset(dma);
	}
	else {
		PF->addi(ret, _fbinfo->width)->addi(ret, _fbinfo->height)->addi(ret, _fbinfo->depth);
	}	
	return 0;
}

static inline void dup16(uint16_t* dst, uint32_t* src, uint32_t w, uint32_t h) {
  register int32_t i, size;
  size = w * h;
  for(i=0; i < size; i++) {
    register uint32_t s = src[i];
    register uint8_t r = (s >> 16) & 0xff;
    register uint8_t g = (s >> 8)  & 0xff;
    register uint8_t b = s & 0xff;
    dst[i] = ((r >> 3) <<11) | ((g >> 3) << 6) | (b >> 3);
  }
}

static int32_t do_flush(fb_dma_t* dma) {
	const void* buf = dma->data;
	uint32_t size = dma->size;
  uint32_t sz = 4 * _fbinfo->width * _fbinfo->height;
  if(size != sz) {
		return -1;
	}

  if(_fbinfo->depth == 32)
    memcpy((void*)_fbinfo->pointer, buf, size);
  else if(_fbinfo->depth == 16)
    dup16((uint16_t*)_fbinfo->pointer, (uint32_t*)buf,  _fbinfo->width, _fbinfo->height);
	else 
		size = 0;
  return (int32_t)size;
}

/*return
0: error;
-1: resized;
>0: size flushed*/
static int fb_flush(int fd, int from_pid, fsinfo_t* info, void* p) {
	(void)fd;
	(void)from_pid;
	(void)info;
	fb_dma_t* dma = (fb_dma_t*)p;

	if(_fbinfo->pointer == 0)
		return 0;

	return do_flush(dma);
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
	fb_dma_t dma;
	dma.shm_id = 0;
	if(bcm283x_fb_init(640, 480, 32) != 0)
		return -1;
	_fbinfo = bcm283x_get_fbinfo();
	
	if(fb_dma_reset(&dma) != 0) {
		return -1;
	}

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "framebuffer");
	dev.dma = fb_dma;
	dev.flush = fb_flush;
	dev.fcntl = fb_fcntl;
	dev.dev_cntl = fb_dev_cntl;

	dev.extra_data = &dma;
	device_run(&dev, mnt_name, FS_TYPE_CHAR);
	shm_unmap(dma.shm_id);
	return 0;
}
