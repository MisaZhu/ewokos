#ifndef DISPLAY_H
#define DISPLAY_H

#include <graph/graph.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DISPLAY_DIRTY_MAX 4

/* control block for the display framebuffer, living in its own small shm
 * segment (id obtained via DISPLAY_CNTL_GET_CTRL). 'busy' is set by the
 * daemon while a flush is in flight; the dirty rects describe the damaged
 * region of the next flush. */
typedef struct {
	uint8_t busy;
	uint8_t dirty_num;
	uint8_t reserved[2];
	grect_t dirty[DISPLAY_DIRTY_MAX];
} display_ctrl_t;

typedef struct {
	uint32_t display_index;
	int fd;
	void* dma;
	int32_t dma_id; //shm id of the dma buffer, so it can be shared with other procs
	display_ctrl_t* ctrl; //attached ctrl block (its own shm segment)
	graph_t* g;
} display_t;

enum {
	DISPLAY_CNTL_GET_INFO = 0,
	DISPLAY_CNTL_GET_CTRL //get the shm id of the ctrl block
};

enum {
	DISPLAY_DEV_CNTL_GET_INFO = 0,
	DISPLAY_DEV_CNTL_SET_INFO
};

int      display_set(const char *dev, int w, int h, int bpp);
int      display_open(const char *dev, int32_t disp_index, display_t* display);
int      display_dev_info(const char *dev, int* w, int* h, int* bpp);
int      display_info(display_t* display, int* w, int* h, int* bpp);
graph_t* display_fetch_graph(display_t* display);
int      display_flush(display_t* display, bool waiting);
int      display_close(display_t* display);
bool     display_busy(display_t* display);
display_ctrl_t* display_ctrl(display_t* display);
int      display_set_dirty(display_t* display, const grect_t* rects, uint32_t num);

#ifdef __cplusplus
}
#endif

#endif
