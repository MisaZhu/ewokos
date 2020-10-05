#ifndef FB_H
#define FB_H

#include <graph/graph.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int fd;
	int dma_id;
	graph_t* g;
} fb_t;

int      fb_open(const char *dev, fb_t* fb);
int      fb_size(fb_t* fb, int* w, int* h);
graph_t* fb_fetch_graph(fb_t* fb);
int      fb_flush(fb_t* fb);
int      fb_close(fb_t* fb);

#ifdef __cplusplus
}
#endif

#endif
