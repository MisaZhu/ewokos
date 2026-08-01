#include <fb/fb.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <ewoksys/syscall.h>
#include <ewoksys/vdevice.h>
#include <sys/shm.h>
#include <ewoksys/proto.h>
#include <ewoksys/vfs.h>
#include <ewoksys/core.h>

#ifdef __cplusplus
extern "C" {
#endif

int fb_open(const char *dev, int32_t disp_index, fb_t* fb) {
	if(fb == NULL || dev == NULL)
		return -1;

	memset(fb, 0, sizeof(fb_t));
	fb->dma_id = -1;
	fb->fd = open(dev, O_RDWR);
	if(fb->fd < 0)
		return -1;
	fb->display_index = disp_index;
	return 0;
}

int fb_set(const char *dev, int w, int h, int bpp) {
	if(bpp != 16 && bpp != 32) {
		bpp = 32;
	}

	proto_t in;
	PF->format(&in, "i,i,i", w, h, bpp);

	int res = dev_cntl(dev, FB_DEV_CNTL_SET_INFO, &in, NULL);
	PF->clear(&in);
	return res;
}

int fb_dev_info(const char *dev, int *w, int *h, int *bpp) {
	proto_t out;
	PF->init(&out);
	if(dev_cntl(dev, FB_DEV_CNTL_GET_INFO, NULL, &out) != 0)
		return -1;

	if(w != NULL)
		*w = proto_read_int(&out);
	if(h != NULL)
		*h = proto_read_int(&out);
	if(bpp != NULL)
		*bpp = proto_read_int(&out);
	PF->clear(&out);
	return 0;
}

int fb_info(fb_t* fb, int* w, int* h, int* bpp) {
	if(fb == NULL || fb->fd < 0)
		return -1;

	proto_t out;
	PF->init(&out);
	if(vfs_fcntl(fb->fd, FB_CNTL_GET_INFO, NULL, &out) != 0) { //get fb info
		PF->clear(&out);
		return -1;
	}
	*w = proto_read_int(&out);
	*h = proto_read_int(&out);
	*bpp = proto_read_int(&out);
	PF->clear(&out);
	return 0;
}

int fb_flush(fb_t* fb, bool waiting) {
	if(fb == NULL || fb->fd < 0)
		return -1;
	return vfs_flush(fb->fd, waiting);
}

int fb_close(fb_t* fb) {
	if(fb == NULL || fb->fd < 0)
		return -1;
	if(fb->g != NULL) {
		graph_free(fb->g);
		shmdt(fb->dma);
	}
	close(fb->fd);
	return 0;
}

fb_ctrl_t* fb_ctrl(fb_t* fb) {
	if(fb == NULL || fb->dma == NULL || fb->g == NULL)
		return NULL;
	uint32_t size = fb->g->w * fb->g->h * 4;
	return (fb_ctrl_t*)(((uint8_t*)fb->dma) + size);
}

bool fb_busy(fb_t* fb) {
	if(fb == NULL || fb->fd < 0)
		return false;
	fb_ctrl_t* ctrl = fb_ctrl(fb);
	if(ctrl == NULL)
		return false;
	return ctrl->busy != 0;
}

/*declare the damaged area of the next flush. rects == NULL or num == 0
  means the whole framebuffer. The daemon consumes and clears it on every
  flush, so it has to be set again for each partial frame.*/
int fb_set_dirty(fb_t* fb, const grect_t* rects, uint32_t num) {
	fb_ctrl_t* ctrl = fb_ctrl(fb);
	if(ctrl == NULL)
		return -1;

	if(rects == NULL || num == 0 || num > FB_DIRTY_MAX) {
		ctrl->dirty_num = 0; //too many rects, pushing everything is cheaper
		return 0;
	}

	memcpy(ctrl->dirty, rects, num * sizeof(grect_t));
	ctrl->dirty_num = (uint8_t)num;
	return 0;
}

graph_t* fb_fetch_graph(fb_t* fb) {
	if(fb == NULL || fb->fd < 0)
		return NULL;

	int w, h, bpp;
	int32_t dma_id;
	uint8_t* dma;
	graph_t* g;

	if(fb_info(fb, &w, &h, &bpp) != 0 ||
			w <= 0 || h <= 0)
		return NULL;

	if(fb->g != NULL && fb->g->w == w && fb->g->h == h)
		return fb->g;

	if(fb->g != NULL) {
		graph_free(fb->g);
		shmdt(fb->dma);
		fb->g = NULL;
		fb->dma = NULL;
	}

	dma_id = vfs_dma(fb->fd, NULL);
	if(dma_id == -1) 
		return NULL;
	
	dma = shmat(dma_id, 0, 0);
	if(dma == NULL) 
		return NULL;
	
	g = graph_new((uint32_t*)dma, w, h);
	fb->dma = dma;
	fb->dma_id = dma_id;
	fb->g = g;
	return g;
}

#ifdef __cplusplus
}
#endif
