#ifndef XEVENT_H
#define XEVENT_H

#include <types.h>

enum {
	X_EV_NONE = 0,
	X_EV_MOUSE,
	X_EV_KEYB,
	X_EV_GUI
};

enum {
	X_EV_MOUSE_MOV = 0,
	X_EV_MOUSE_DOWN,
	X_EV_MOUSE_UP
};

enum {
	X_EV_KEYB_DOWN = 0,
	X_EV_KEYB_UP
};

typedef struct {
	uint8_t type;
	union {
		struct {
			int32_t x;
			int32_t y;
			int32_t state;
			int32_t button;
		} mouse;

		struct {
			int32_t value;
			int32_t state;
		} keyboard;

		struct {
			int32_t value;
			int32_t state;
		} gui;
	} value;
} x_ev_t;

#endif
