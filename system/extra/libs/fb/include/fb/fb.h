#ifndef FB_H
#define FB_H

#include <graph/graph.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	void* buf;
	int   shm_id;
	int   w;
	int   h;
} fb_dma_t;

int      fb_set(const char *dev, int w, int h, int bpp);
int      fb_size(int fd, int* w, int* h);
int      fb_dev_info(const char* dev, int* w, int* h, int* bpp);
int      fb_flush(int fd, fb_dma_t* dma, graph_t* g);
int      fb_dma(int fd, fb_dma_t* dma, int w, int h);
void     fb_close_dma(fb_dma_t* dma);
graph_t* fb_graph(int fd);

#ifdef __cplusplus
}
#endif

#endif
