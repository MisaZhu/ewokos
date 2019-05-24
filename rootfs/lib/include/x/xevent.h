#ifndef XEVENT_H
#define XEVENT_H

#include <types.h>

enum {
	X_EV_NONE = 0,
	X_EV_MOUSE,
	X_EV_KEYB,
	X_EV_WIN
};

enum {
	X_EV_MOUSE_MOVE = 0,
	X_EV_MOUSE_DRAG,
	X_EV_MOUSE_DOWN,
	X_EV_MOUSE_UP
};

enum {
	X_EV_KEYB_DOWN = 0,
	X_EV_KEYB_UP
};

enum {
	X_EV_WIN_CLOSE = 0
};

typedef struct {
	uint8_t type;
	int32_t state;
	union {
		struct {
			int32_t x, y;
			int32_t rx, ry;
			int32_t button;
		} mouse;

		struct {
			int32_t value;
		} keyboard;

		struct {
			int32_t value;
		} window;
	} value;
} x_ev_t;

#define X_EV_MAX 16

typedef struct {
	x_ev_t items[X_EV_MAX];
	uint32_t start;
	uint32_t size;
} x_ev_queue_t;

void x_ev_queue_init(x_ev_queue_t *q);
int32_t x_ev_queue_push(x_ev_queue_t *q, x_ev_t *ev, bool loop);
int32_t x_ev_queue_pop(x_ev_queue_t *q, x_ev_t *ev);

#endif
