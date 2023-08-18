#include <fb/fb.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/vdevice.h>
#include <sys/shm.h>
#include <sys/proto.h>
#include <sys/vfs.h>

#ifdef __cplusplus
extern "C" {
#endif

int fb_open(const char *dev, fb_t* fb) {
	if(fb == NULL || dev == NULL)
		return -1;

	memset(fb, 0, sizeof(fb_t));
	fb->fd = open(dev, O_RDWR);
	if(fb->fd < 0)
		return -1;
	return 0;
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

int fb_info(fb_t* fb, int* w, int* h, int* bpp) {
	if(fb == NULL || fb->fd < 0)
		return -1;

	proto_t out;
	PF->init(&out);
	if(vfs_fcntl(fb->fd, 0, NULL, &out) != 0) { //get fb info
		PF->clear(&out);
		return -1;
	}
	*w = proto_read_int(&out);
	*h = proto_read_int(&out);
	*bpp = proto_read_int(&out);
	PF->clear(&out);
	return 0;
}

int fb_flush(fb_t* fb) {
	if(fb == NULL || fb->fd < 0)
		return -1;
	return vfs_flush(fb->fd, true);
}

int fb_close(fb_t* fb) {
	if(fb == NULL || fb->fd < 0)
		return -1;
	if(fb->g != NULL) {
		graph_free(fb->g);
		shm_unmap(fb->dma);
	}
	close(fb->fd);
	return 0;
}

graph_t* fb_fetch_graph(fb_t* fb) {
	if(fb == NULL || fb->fd < 0)
		return NULL;

	int w, h, bpp;
	void* dma;
	graph_t* g;

	if(fb_info(fb, &w, &h, &bpp) != 0 ||
			w <= 0 || h <= 0)
		return NULL;

	if(fb->g != NULL && fb->g->w == w && fb->g->h == h)
		return fb->g;

	if(fb->g != NULL) {
		graph_free(fb->g);
		shm_unmap(fb->dma);
		fb->g = NULL;
		fb->dma = NULL;
	}

	dma = vfs_dma(fb->fd, NULL);
	if(dma == NULL) {
		return NULL;
	}
	dma = shm_map(dma);
	if(dma == NULL) {
		return NULL;
	}
	g = graph_new(dma, w, h);
	fb->dma = dma;
	fb->g = g;
	return g;
}

#ifdef __cplusplus
}
#endif
