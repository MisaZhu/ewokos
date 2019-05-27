#ifndef XCLIENT_H
#define XCLIENT_H

#include <x/xcmd.h>
#include <x/xevent.h>
#include <graph/graph.h>

#define X_STYLE_NO_FRAME  0x1
#define X_STYLE_NO_TITLE  0x2
#define X_STYLE_NO_RESIZE 0x4
#define X_STYLE_NO_MOVE   0x8

#define X_STYLE_ALPHA     0x40000000
#define X_STYLE_TOP       0x80000000

enum {
	X_STATE_HIDE = 0,
	X_STATE_NORMAL,
	X_STATE_FULLSCR,
	X_STATE_MAX,
	X_STATE_MIN
};

typedef struct {
	int32_t fd;
	int32_t shm_id;
	int32_t x, y, w, h;
} x_t;

int32_t x_open(const char* dev, x_t* x);

void x_close(x_t* x);

void x_flush(x_t* x);

void x_set_title(x_t* x, const char* title);

void x_set_style(x_t* x, uint32_t style);

void x_set_state(x_t* x, uint32_t state);

void x_move_to(x_t* x, int32_t tx, int32_t ty);

void x_resize_to(x_t* x, int32_t w, int32_t h);

graph_t* x_get_graph(x_t* x);

int32_t x_get_scr_size(const char* dev_name, gsize_t* sz);

int32_t x_get_event(x_t* x, x_ev_t* ev);

#endif
