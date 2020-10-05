#include <fb/fb.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
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

int fb_flush(fb_t* fb) {
	if(fb == NULL || fb->fd < 0)
		return -1;
	return vfs_flush(fb->fd);
}

int fb_close(fb_t* fb) {
	if(fb == NULL || fb->fd < 0)
		return -1;
	if(fb->g != NULL) {
		graph_free(fb->g);
		shm_unmap(fb->dma_id);
	}
	close(fb->fd);
	return 0;
}

int fb_size(fb_t* fb, int* w, int* h) {
	if(fb == NULL || fb->fd < 0)
		return -1;

	proto_t out;
	PF->init(&out);
	if(vfs_fcntl(fb->fd, 0, NULL, &out) != 0) { //get fb size
		PF->clear(&out);
		return -1;
	}
	*w = proto_read_int(&out);
	*h = proto_read_int(&out);
	PF->clear(&out);
	return 0;
}

graph_t* fb_fetch_graph(fb_t* fb) {
	if(fb == NULL || fb->fd < 0)
		return NULL;

	int id, w, h;
	void* gbuf;
	graph_t* g;

	if(fb_size(fb, &w, &h) != 0 ||
			w <= 0 || h <= 0)
		return NULL;

	if(fb->g != NULL && fb->g->w == w && fb->g->h == h)
		return fb->g;
	
	if(fb->g != NULL) {
		graph_free(fb->g);
		shm_unmap(fb->dma_id);
		fb->g = NULL;
		fb->dma_id = 0;
	}

	id = vfs_dma(fb->fd, NULL);
	if(id <= 0) {
		return NULL;
	}

	gbuf = shm_map(id);
	if(gbuf == NULL) {
		return NULL;
	}
	g = graph_new(gbuf, w, h);
	fb->dma_id = id;
	fb->g = g;
	return g;
}

#ifdef __cplusplus
}
#endif

