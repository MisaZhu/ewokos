#ifndef GRAPH_FB_H
#define GRAPH_FB_H

#include <graph/graph.h>

#ifdef __cplusplus
extern "C" {
#endif

graph_t* graph_from_fb(int fd, int *dma_id);

#ifdef __cplusplus
}
#endif

#endif
