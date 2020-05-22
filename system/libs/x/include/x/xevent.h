#ifndef XEVENT_H
#define XEVENT_H

#include <stdint.h>

enum {
	XEVT_NONE = 0,
	XEVT_MOUSE,
	XEVT_KEYB,
	XEVT_WIN
};

enum {
	XEVT_MOUSE_MOVE = 0,
	XEVT_MOUSE_DRAG,
	XEVT_MOUSE_DOWN,
	XEVT_MOUSE_UP
};

enum {
	XEVT_KEYB_DOWN = 0,
	XEVT_KEYB_UP
};

enum {
	XEVT_WIN_CLOSE = 0,
	XEVT_WIN_MOVE,
	XEVT_WIN_MOVE_TO,
	XEVT_WIN_MAX,
	XEVT_WIN_FOCUS,
	XEVT_WIN_UNFOCUS
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
			int32_t event;
			int32_t v0;
			int32_t v1;
		} window;
	} value;
} xevent_t;

#endif
