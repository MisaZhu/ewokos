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
	int32_t dma_id; //shm id of the dma buffer, so it can be shared with other procs
	graph_t* g;
} fb_t;

#define FB_DIRTY_MAX 4

/* control block sitting right behind the pixel data inside the fb dma
 * share-memory. 'busy' stays at offset 0 so it keeps the layout of the
 * original single busy byte. */
typedef struct {
	uint8_t busy;
	uint8_t dirty_num;
	uint8_t reserved[2];
	grect_t dirty[FB_DIRTY_MAX];
} fb_ctrl_t;

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
fb_ctrl_t* fb_ctrl(fb_t* fb);
int      fb_set_dirty(fb_t* fb, const grect_t* rects, uint32_t num);

#ifdef __cplusplus
}
#endif

#endif
