#ifndef XCLIENT_H
#define XCLIENT_H

#include <x/xcmd.h>
#include <x/xevent.h>
#include <graph/graph.h>

typedef struct {
	int32_t fd;
	int32_t shm_id;
	int32_t w, h;
} x_t;

int32_t x_open(const char* dev, x_t* x);

void x_close(x_t* x);

void x_flush(x_t* x);

void x_set_title(x_t* x, const char* title);

void x_move_to(x_t* x, int32_t tx, int32_t ty);

void x_resize_to(x_t* x, int32_t w, int32_t h);

graph_t* x_get_graph(x_t* x);

int32_t x_get_scr_size(x_t* x, gsize_t* sz);

int32_t x_get_event(x_t* x, x_ev_t* ev);

#endif
