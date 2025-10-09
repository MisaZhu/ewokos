#ifndef FB_H
#define FB_H

#include <graph/graph.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint32_t display_index;
	int fd;
	void* dma;
	graph_t* g;
} fb_t;

enum {
	FB_CNTL_GET_INFO = 0
};

enum {
	FB_DEV_CNTL_GET_INFO = 0,
	FB_DEV_CNTL_SET_INFO
};

int      fb_set(const char *dev, int w, int h, int bpp);
int      fb_open(const char *dev, int32_t disp_index, fb_t* fb);
int      fb_dev_info(const char *dev, int* w, int* h, int* bpp);
int      fb_info(fb_t* fb, int* w, int* h, int* bpp);
graph_t* fb_fetch_graph(fb_t* fb);
int      fb_flush(fb_t* fb, bool waiting);
int      fb_close(fb_t* fb);
bool     fb_busy(fb_t* fb);

#ifdef __cplusplus
}
#endif

#endif
