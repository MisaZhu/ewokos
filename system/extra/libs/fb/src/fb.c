#include <fb/fb.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/shm.h>
#include <sys/proto.h>
#include <sys/vfs.h>
#include <sys/vdevice.h>

#ifdef __cplusplus 
extern "C" { 
#endif

int fb_size(int fd, int* w, int* h) {
	if(fd < 0)
		return -1;

	proto_t in, out;
	PF->init(&in);
	PF->init(&out);
	if(vfs_fcntl(fd, 0, &in, &out) != 0) { //get fb size
		PF->clear(&out);
		return -1;
	}
	*w = proto_read_int(&out);
	*h = proto_read_int(&out);
	PF->clear(&out);
	return 0;
}

graph_t* fb_graph(int fd) {
	int w, h;
	if(fb_size(fd, &w, &h) != 0 ||
			w <= 0 || h <= 0)
		return NULL;
	return graph_new(NULL, w, h);
}

int fb_dma(int fd, fb_dma_t* dma, int w, int h) {
	if(fd < 0 || dma == NULL || w <= 0 || h <= 0)
		return -1;

	if(dma->w == w && dma->h == h)
		return 0;
	
	if(dma->shm_id > 0) {
		shm_unmap(dma->shm_id);
		dma->buf = NULL;
	}
	
	dma->shm_id = vfs_dma(fd, NULL);
	if(dma->shm_id <= 0) {
		return -1;
	}

	dma->buf = shm_map(dma->shm_id);
	if(dma->buf == NULL) {
		return -1;
	}
	dma->w = w;
	dma->h = h;
	return 0;
}

void fb_close_dma(fb_dma_t* dma) {
	if(dma == NULL || dma->shm_id <= 0)
		return;
	shm_unmap(dma->shm_id);
}

int fb_flush(int fd, fb_dma_t* dma, graph_t* g) {
	if(fd < 0 || dma == NULL || dma->buf == NULL ||
			g == NULL || dma->w != g->w || dma->h != g->h)
		return -1;
	memcpy(dma->buf, g->buffer, g->w*g->h*4);
	return vfs_flush(fd);
}

int fb_set(const char *dev, int w, int h, int bpp) {
	if(bpp != 16 && bpp != 32) {
		bpp = 32;
	}

	proto_t in;
	PF->init(&in)->
		addi(&in, w)->
		addi(&in, h)->
		addi(&in, bpp);

	int res = dev_cntl(dev, 0, &in, NULL);
	PF->clear(&in);
	return res;
}

int fb_dev_info(const char *dev, int *w, int *h, int *bpp) {
	proto_t out;
	PF->init(&out);
	if(dev_cntl(dev, 1, NULL, &out) != 0)
		return -1;

	*w = proto_read_int(&out);
	*h = proto_read_int(&out);
	*bpp = proto_read_int(&out);
	PF->clear(&out);
	return 0;
}

#ifdef __cplusplus
}
#endif

