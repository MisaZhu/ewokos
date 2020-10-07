#ifndef FB_H
#define FB_H

#include <graph/graph.h>

#ifdef __cplusplus
extern "C" {
#endif

int      fb_set(const char *dev, int w, int h, int bpp);
int      fb_size(int fd, int* w, int* h);
int      fb_dev_info(const char* dev, int* w, int* h, int* bpp);
int      fb_flush(int fd, graph_t* g);
graph_t* fb_graph(int fd);

#ifdef __cplusplus
}
#endif

#endif
