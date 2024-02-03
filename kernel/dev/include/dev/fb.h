#ifndef FB_H
#define FB_H

#include <dev/fbinfo.h>
#include <graph/graph.h>

int32_t  fb_init(uint32_t w, uint32_t h, uint8_t dep, fbinfo_t* fbinfo);
void     fb_close(void);
int32_t  fb_init_bsp(uint32_t w, uint32_t h, uint8_t dep,  fbinfo_t* fbinfo);
void     fb_flush_graph_bsp(graph_t* g);
graph_t* fb_fetch_graph_bsp(uint32_t w, uint32_t h);
void     fb_flush32(uint32_t* g32, uint32_t w, uint32_t h, uint8_t rotate);

#endif