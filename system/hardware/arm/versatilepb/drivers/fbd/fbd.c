#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <sys/syscall.h>
#include <sys/vdevice.h>
#include <sys/shm.h>
#include <graph/graph.h>
#include <fb/fb.h>
#include <arch/vpb/framebuffer.h>

typedef struct {
	void* data;
	uint32_t size;
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
	if(cmd == FB_CNTL_GET_INFO) { //get fb size
		PF->addi(out, _fbinfo->width)->addi(out, _fbinfo->height)->addi(out, _fbinfo->depth);
	}
	return 0;
}

static int fb_dma_init(fb_dma_t* dma) {
	memset(dma, 0, sizeof(fb_dma_t));
	if(_fbinfo->pointer == 0)
		return -1;

	dma->data = shm_alloc(_fbinfo->size_max+1, 1);
	if(dma->data == NULL)
		return -1;
	dma->size = _fbinfo->size_max;
	return 0;
}

static int fb_dev_cntl(int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
	(void)from_pid;
	(void)ret;
	(void)p;

	if(cmd == FB_DEV_CNTL_SET_INFO) { //set fb size and bpp
		int w = proto_read_int(in);
		int h = proto_read_int(in);
		int bpp = proto_read_int(in);
		if(vpb_fb_init(w, h, bpp) != 0)
			return -1;
		_fbinfo = vpb_get_fbinfo();
	}
	else if(cmd == FB_DEV_CNTL_GET_INFO) {
		PF->addi(ret, _fbinfo->width)->addi(ret, _fbinfo->height)->addi(ret, _fbinfo->depth);
	}	
	return 0;
}

static uint32_t flush_buf(const void* buf, uint32_t size) {
	uint32_t sz = 4 * _fbinfo->width * _fbinfo->height;
	if(size < sz || _fbinfo->depth != 32)
		return -1;
	memcpy((void*)_fbinfo->pointer, buf, sz);
	return size;
}

static int32_t do_flush(fb_dma_t* dma) {
	uint8_t* buf = dma->data;
	uint32_t size = dma->size;
	buf[0] = 1;
	int32_t res = (int32_t)flush_buf(buf+1, size);
	buf[0] = 0;
	return res;
}

/*return
0: error;
-1: resized;
>0: size flushed*/
static int do_fb_flush(int fd, int from_pid, fsinfo_t* info, void* p) {
	(void)fd;
	(void)from_pid;
	(void)info;
	fb_dma_t* dma = (fb_dma_t*)p;
	if(_fbinfo->pointer == 0)
		return 0;

	return do_flush(dma);
}

static void* fb_dma(int fd, int from_pid, fsinfo_t* info, int* size, void* p) {
	(void)fd;
	(void)from_pid;
	(void)info;
	fb_dma_t* dma = (fb_dma_t*)p;
	*size = dma->size;
	return dma->data;
}

static void clear(void) {
	graph_t g;
	graph_init(&g, (uint32_t*)_fbinfo->pointer, _fbinfo->width, _fbinfo->height);
	graph_clear(&g, 0xff000000);
}

int main(int argc, char** argv) {
	const char* mnt_name = argc > 1 ? argv[1]: "/dev/fb0";
	int w = 640;
	int h = 480;
	if(argc > 3) {
		w = atoi(argv[2]);
		h = atoi(argv[3]);
	}

	fb_dma_t dma;
	dma.data = NULL;
	if(vpb_fb_init(w, h, 32) != 0)
		return -1;
	_fbinfo = vpb_get_fbinfo();
	
	if(fb_dma_init(&dma) != 0) {
		return -1;
	}
	clear();

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "framebuffer");
	dev.dma = fb_dma;
	dev.flush = do_fb_flush;
	dev.fcntl = fb_fcntl;
	dev.dev_cntl = fb_dev_cntl;

	dev.extra_data = &dma;
	device_run(&dev, mnt_name, FS_TYPE_CHAR);
	shm_unmap(dma.data);
	return 0;
}
